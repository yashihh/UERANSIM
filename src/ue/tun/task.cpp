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

namespace nr::ue
{

ue::TunTask::TunTask(TaskBase *base, int psi, int fd) : m_base{base}, m_psi{psi}, m_fd{fd}, m_receiver{}
{
    m_logger = m_base->logBase->makeUniqueLogger(m_base->config->getLoggerPrefix() + "tun");

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
        ssize_t res = ::write(m_fd, w.data.data(), w.data.length());
        int udpPort = w.data.get2I(20);
        int messageType = -1;
        /* send ptp message to dstt */
        if( udpPort == PTP_EVENT_PORT || udpPort == PTP_GENERAL_PORT){
            messageType = msg_type(w.data);
        }
        if( messageType == PTP_FOLLOW_UP){
            //m_logger->debug("downlink ptpType = [%d]",messageType);
            // double t;
            // Dstt dstt_downlink;
            // t = dstt_downlink.egress(w.data, messageType);
        }
        m_logger->info("%s", pkt_hex_dump(w.data.toHexString()).c_str());
        
        
        if (res < 0)
            push(NmError(GetErrorMessage("TUN device could not write")));
        else if (res != w.data.length())
            push(NmError(GetErrorMessage("TUN device partially written")));
        break;
    }
    case NtsMessageType::UE_TUN_TO_APP: {
        m_base->appTask->push(std::move(msg));
        break;
    }
    default:
        break;
    }
}

} // namespace nr::ue