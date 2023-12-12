//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"
#include <cstring>
#include <ue/app/task.hpp>
#include <ue/nts.hpp>
#include <unistd.h>
#include <utils/libc_error.hpp>
#include <utils/scoped_thread.hpp>
#include "ptp.hpp"
#include "dstt/dstt.hpp"
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>


// TODO: May be reduced to MTU 1500
#define RECEIVER_BUFFER_SIZE 8000

struct ReceiverArgs
{
    int fd{};
    int psi{};
    NtsTask *targetTask{};
};

static std::string GetErrorMessage(const std::string &cause)
{
    std::string what = cause;

    int errNo = errno;
    if (errNo != 0)
        what += " (" + std::string{strerror(errNo)} + ")";

    return what;
}

static std::unique_ptr<nr::ue::NmUeTunToApp> NmError(std::string &&error)
{
    auto m = std::make_unique<nr::ue::NmUeTunToApp>(nr::ue::NmUeTunToApp::TUN_ERROR);
    m->error = std::move(error);
    return m;
}

static void ReceiverThread(ReceiverArgs *args)
{
    int fd = args->fd;
    int psi = args->psi;
    NtsTask *targetTask = args->targetTask;

    delete args;

    uint8_t buffer[RECEIVER_BUFFER_SIZE];

    while (true)
    {
        ssize_t n = ::read(fd, buffer, RECEIVER_BUFFER_SIZE);
        if (n < 0)
        {
            targetTask->push(NmError(GetErrorMessage("TUN device could not read")));
            return; // Abort receiver thread
        }

        if (n > 0)
        {
            auto m = std::make_unique<nr::ue::NmUeTunToApp>(nr::ue::NmUeTunToApp::DATA_PDU_DELIVERY);
            m->psi = psi;
            m->data = OctetString::FromArray(buffer, static_cast<size_t>(n));
            targetTask->push(std::move(m));
        }
    }
}

static int msg_type(OctetString &m_data){
    return m_data.getI(28) & 0x0f ;
}

std::string pkt_hex_dump(std::string data){
    std::string str = "packet hex dump:\n";
    int cnt = 0;
    int byte = 0;
    for (size_t i = 0; i < data.size(); i++){
        str += data[i];
        cnt += 1;
        if ( cnt == 2 ){
            str += " ";
            cnt = 0;
            byte += 1;
            if( byte % 16 == 0 && i != 0){
                str += "\n";
                byte = 0;
            }
        }
    }
    return str;
}

struct sockaddr_in dest_addr;
struct sockaddr_ll sll;

namespace nr::ue
{

ue::TunTask::TunTask(TaskBase *base, int psi, int fd) : m_base{base}, m_psi{psi}, m_fd{fd}, m_receiver{}
{
    m_logger = m_base->logBase->makeUniqueLogger(m_base->config->getLoggerPrefix() + "tun");
    /* Forwarding to target port  */
    char ifName[IFNAMSIZ];
    struct ifreq req;
    /* TODO： DSTT to port number */
    strcpy(ifName, "enp0s10");
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
        perror("socket");
    }
    memset(&req, 0, sizeof(struct ifreq));
    strncpy(req.ifr_name, ifName, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFINDEX, &req) < 0)
        perror("SIOCGIFINDEX");
    sll.sll_ifindex = req.ifr_ifindex;
    sll.sll_family = AF_PACKET;
    // sll.sll_halen = ETHER_ADDR_LEN;
}

void TunTask::onStart()
{
    auto *receiverArgs = new ReceiverArgs();
    receiverArgs->fd = m_fd;
    receiverArgs->targetTask = this;
    receiverArgs->psi = m_psi;
    m_receiver =
        new ScopedThread([](void *args) { ReceiverThread(reinterpret_cast<ReceiverArgs *>(args)); }, receiverArgs);
}

void TunTask::onQuit()
{
    delete m_receiver;
    ::close(m_fd);
}

void TunTask::onLoop()
{
    auto msg = take();
    if (!msg)
        return;

    switch (msg->msgType)
    {
    case NtsMessageType::UE_APP_TO_TUN: {
        auto &w = dynamic_cast<NmAppToTun &>(*msg);
        int udpPort = w.data.get2I(20);
        int messageType = -1;
        /* send ptp message to dstt */
        if( udpPort == PTP_EVENT_PORT || udpPort == PTP_GENERAL_PORT){
            messageType = msg_type(w.data);

            if( messageType == PTP_FOLLOW_UP){
                double t;
                Dstt dstt_downlink;
                t = dstt_downlink.egress(w.data, messageType);
                // m_logger->debug("residence_time:  [%lf]", t);
            }
            /*add ethernet msg*/
            uint8_t ether_msg[14] = { 
                0x01, 0x00, 0x5e, 0x00, 0x01, 0x81,  //Dst Mac
                0x08, 0x00, 0x27, 0x9d, 0x65, 0x39,  //Src Mac
                0x08, 0x00 //ether type
            };
            OctetString msg = OctetString::FromArray(ether_msg, sizeof(ether_msg));
            w.data = OctetString::Concat(msg, w.data);
            if (sendto(sockfd, w.data.data(), w.data.length(), 0, (struct sockaddr*)&sll, sizeof(struct sockaddr_ll)) < 0)
                printf("Send failed\n");
        }
        // m_logger->info("%s", pkt_hex_dump(w.data.toHexString()).c_str());
        break;
    }
    case NtsMessageType::UE_TUN_TO_APP: {
        auto &w = dynamic_cast<NmUeTunToApp &>(*msg);
        int udpPort = w.data.get2I(20);
        int messageType = -1;
        /* send ptp message to dstt */
        if( udpPort == PTP_EVENT_PORT || udpPort == PTP_GENERAL_PORT){
            messageType = msg_type(w.data);

            if( messageType == PTP_DELAY_REQ){
                Dstt dstt_uplink;
                dstt_uplink.ingress(w.data);
                // m_logger->info("%s", pkt_hex_dump(w.data.toHexString()).c_str());
            }
        }
        m_base->appTask->push(std::move(msg));
        break;
    }
    default:
        break;
    }
}

} // namespace nr::ue