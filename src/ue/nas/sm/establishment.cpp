//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "sm.hpp"
#include <algorithm>
#include <lib/nas/proto_conf.hpp>
#include <lib/nas/utils.hpp>
#include <ue/app/task.hpp>
#include <ue/nas/mm/mm.hpp>
#include <dstt/dstt.hpp>
#include <iostream>

namespace nr::ue
{

static nas::IE5gSmCapability MakeSmCapability()
{
    nas::IE5gSmCapability cap{};
    cap.rqos = nas::EReflectiveQoS::NOT_SUPPORTED;
    cap.mh6pdu = nas::EMultiHomedIPv6PduSession::NOT_SUPPORTED;
    cap.tpmic = nas::ETransferOfPortManagementInformationContainers::SUPPORTED;
    return cap;
}

static nas::IEIntegrityProtectionMaximumDataRate MakeIntegrityMaxRate(const IntegrityMaxDataRateConfig &config)
{
    nas::IEIntegrityProtectionMaximumDataRate res{};
    res.maxRateDownlink = nas::EMaximumDataRatePerUeForUserPlaneIntegrityProtectionForDownlink::SIXTY_FOUR_KBPS;
    res.maxRateUplink = nas::EMaximumDataRatePerUeForUserPlaneIntegrityProtectionForUplink::SIXTY_FOUR_KBPS;
    if (config.downlinkFull)
        res.maxRateDownlink = nas::EMaximumDataRatePerUeForUserPlaneIntegrityProtectionForDownlink::FULL_DATA_RATE;
    if (config.uplinkFull)
        res.maxRateUplink = nas::EMaximumDataRatePerUeForUserPlaneIntegrityProtectionForUplink::FULL_DATA_RATE;
    return res;
}

void NasSm::sendEstablishmentRequest(const SessionConfig &config)
{
    m_logger->debug("Sending PDU Session Establishment Request");

    /* Control the protocol state */
    if (m_mm->m_rmState != ERmState::RM_REGISTERED)
    {
        m_logger->err("PDU session establishment could not be triggered, UE is not registered");
        return;
    }

    if (m_mm->m_mmSubState == EMmSubState::MM_REGISTERED_NON_ALLOWED_SERVICE && !m_mm->hasEmergency() && !m_mm->isHighPriority())
    {
        m_logger->err("PDU session establishment could not be triggered, non allowed service condition");
        return;
    }

    /* Control the received config */
    if (config.type != nas::EPduSessionType::IPV4)
    {
        m_logger->err("PDU session type [%s] is not supported", nas::utils::EnumToString(config.type));
        return;
    }
    if (m_mm->m_rmState == ERmState::RM_REGISTERED && m_mm->m_registeredForEmergency && !config.isEmergency)
    {
        m_logger->err("Non-emergency PDU session cannot be requested, UE is registered for emergency only");
        return;
    }
    if (config.isEmergency && anyEmergencySession())
    {
        m_logger->err(
            "Emergency PDU session cannot be requested, another emergency session already established or establishing");
        return;
    }

    /* Allocate PSI */
    int psi = allocatePduSessionId(config);
    if (psi == 0)
        return;

    /* Allocate PTI */
    int pti = allocateProcedureTransactionId();
    if (pti == 0)
    {
        freePduSessionId(psi);
        return;
    }

    /* Set relevant fields of the PS description */
    auto &ps = m_pduSessions[psi];
    ps->psState = EPsState::ACTIVE_PENDING;
    ps->sessionType = config.type;
    ps->apn = config.apn;
    ps->sNssai = config.sNssai;
    ps->isEmergency = config.isEmergency;
    ps->authorizedQoSRules = {};
    ps->sessionAmbr = {};
    ps->authorizedQoSFlowDescriptions = {};
    ps->pduAddress = {};
    ps->uplinkPending = false;

    /* Make PCO */
    nas::ProtocolConfigurationOptions opt{};
    opt.additionalParams.push_back(std::make_unique<nas::ProtocolConfigurationItem>(
        nas::EProtocolConfigId::CONT_ID_UP_IP_ADDRESS_ALLOCATION_VIA_NAS_SIGNALLING, true, OctetString::Empty()));
    opt.additionalParams.push_back(std::make_unique<nas::ProtocolConfigurationItem>(
        nas::EProtocolConfigId::CONT_ID_DOWN_DNS_SERVER_IPV4_ADDRESS, true, OctetString::Empty()));

    nas::IEExtendedProtocolConfigurationOptions iePco{};
    iePco.configurationProtocol = nas::EConfigurationProtocol::PPP;
    iePco.extension = true;
    iePco.options = opt.encode();

    /* Prepare the establishment request message */
    auto req = std::make_unique<nas::PduSessionEstablishmentRequest>();
    req->pti = pti;
    req->pduSessionId = psi;
    req->integrityProtectionMaximumDataRate = MakeIntegrityMaxRate(m_base->config->integrityMaxRate);
    req->pduSessionType = nas::IEPduSessionType{};
    req->pduSessionType->pduSessionType = nas::EPduSessionType::IPV4;
    req->sscMode = nas::IESscMode{};
    req->sscMode->sscMode = nas::ESscMode::SSC_MODE_1;
    req->extendedProtocolConfigurationOptions = std::move(iePco);
    req->smCapability = MakeSmCapability();
    if(config.type == nas::EPduSessionType::ETHERNET){
        //mac address
    	req->pduSessionType->pduSessionType = nas::EPduSessionType::ETHERNET;
    	req->macAddress = nas::IEMacAddress{};
        std::string add = config.mac.value();
        std::string temp = "";
        for(size_t i=0 ; i<add.size();++i){
            if(add[i] == ':')continue;
            temp += add[i];
        }
        req->macAddress->mac = temp;
	    req->macAddress->macAddress.appendUtf8(temp);
    }
    if (req->smCapability->tpmic == nas::ETransferOfPortManagementInformationContainers::SUPPORTED){
        m_logger->debug("smCapability support Transfer of Port Management Information Containers");

        //UE-DS-TT Residence Time for IEEE TSN network and TSCAI (QoS purpose)
        req->residence_time = nas::IEResidenceTime{};
        // TODO: update incorrect residence time
        req->residence_time->residence_time.appendOctet8((int64_t)95492);
        
        //Port management information container
        //may contain multiple message inside the container
        req->port_manage = nas::IEPortManagementInformationContainer{};
        req->port_manage->encode_header_type[0] = true;
        Dstt::PMIC_show_dstt_capability(req->port_manage->container.port_management_capability);
    }
    /* Set relevant fields of the PT, and start T3580 */
    auto &pt = m_procedureTransactions[pti];
    pt.state = EPtState::PENDING;
    pt.timer = newTransactionTimer(3580);
    pt.message = std::move(req);
    pt.psi = psi;

    /* Send SM message */
    sendSmMessage(psi, *pt.message);
}

void NasSm::receiveEstablishmentAccept(const nas::PduSessionEstablishmentAccept &msg)
{
    m_logger->debug("PDU Session Establishment Accept received");

    if (!checkPtiAndPsi(msg))
        return;

    freeProcedureTransactionId(msg.pti);

    auto &pduSession = m_pduSessions[msg.pduSessionId];
    if (pduSession->psState != EPsState::ACTIVE_PENDING)
    {
        m_logger->err("PS establishment accept received without being requested");
        sendSmCause(nas::ESmCause::MESSAGE_TYPE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE, msg.pti, msg.pduSessionId);
        return;
    }

    if (msg.smCause.has_value())
    {
        m_logger->warn("SM cause received in PduSessionEstablishmentAccept [%s]",
                       nas::utils::EnumToString(msg.smCause->value));
    }

    pduSession->psState = EPsState::ACTIVE;
    pduSession->authorizedQoSRules = nas::utils::DeepCopyIe(msg.authorizedQoSRules);
    pduSession->sessionAmbr = nas::utils::DeepCopyIe(msg.sessionAmbr);
    pduSession->sessionType = msg.selectedPduSessionType.pduSessionType;

    if (msg.authorizedQoSFlowDescriptions.has_value())
        pduSession->authorizedQoSFlowDescriptions = nas::utils::DeepCopyIe(*msg.authorizedQoSFlowDescriptions);
    else
        pduSession->authorizedQoSFlowDescriptions = {};

    if (msg.pduAddress.has_value())
        pduSession->pduAddress = nas::utils::DeepCopyIe(*msg.pduAddress);
    else
        pduSession->pduAddress = {};

    auto statusUpdate = std::make_unique<NmUeStatusUpdate>(NmUeStatusUpdate::SESSION_ESTABLISHMENT);
    statusUpdate->pduSession = pduSession;
    m_base->appTask->push(std::move(statusUpdate));

    m_logger->info("PDU Session establishment is successful PSI[%d]", pduSession->psi);
}

void NasSm::receiveEstablishmentReject(const nas::PduSessionEstablishmentReject &msg)
{
    m_logger->err("PDU Session Establishment Reject received [%s]", nas::utils::EnumToString(msg.smCause.value));

    if (!checkPtiAndPsi(msg))
        return;

    freeProcedureTransactionId(msg.pti);

    auto &pduSession = m_pduSessions[msg.pduSessionId];

    if (pduSession->psState != EPsState::ACTIVE_PENDING)
    {
        m_logger->err("PS establishment reject received without being requested");
        sendSmCause(nas::ESmCause::MESSAGE_TYPE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE, msg.pti, msg.pduSessionId);
        return;
    }

    pduSession->psState = EPsState::INACTIVE;

    if (pduSession->isEmergency)
    {
        // This not much important and no need for now
        // TODO: inform the upper layers of the failure of the procedure
    }
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

void NasSm::receiveModificationCommand(const nas::PduSessionModificationCommand &msg){
    m_logger->debug("PDU Session modification command received");
    auto tmp =  nas::utils::DeepCopyIe(*msg.port_manage);
    m_logger->debug("request message type : [%d]", tmp.service_msg_type);
    m_logger->debug("request iei : [%d]", tmp.iei);
    m_logger->debug("request length : [%d]", tmp.l);
    m_logger->debug("msg length : [%d]", tmp.container.l);
    auto ack = std::make_unique<nas::PduSessionModificationComplete>();
    ack->pduSessionId = msg.pduSessionId;
    ack->pti = msg.pti;
    ack->port_manage = nas::IEPortManagementInformationContainer{};
    int response_header[3] = {0};
    OctetString full_message = Dstt::DecodePMIC(tmp.service_msg_type, tmp.container.port_management_list, response_header);
    int index = 0;
    ack->port_manage->encode_header_type[0] = false;
    
    /* Port management capability 24.539 9.3 */
    if(response_header[0] > 0){ 
        ack->port_manage->container.port_management_capability = full_message.subCopy(index, response_header[0]);
        index += response_header[0];
        ack->port_manage->encode_header_type[1] = true;
    }

    /* Port status 24.539 9.4 */
    if(response_header[1] > 1){
        ack->port_manage->container.port_status = full_message.subCopy(index, response_header[1]);
        index += response_header[1];
        ack->port_manage->encode_header_type[2] = true;
        m_logger->debug("below show port_status in UE");
        m_logger->debug("%s", pkt_hex_dump(ack->port_manage->container.port_status.toHexString()).c_str());
    }
    /* Port update result 24.539 9.5 */
    if(response_header[2] > 1){
        ack->port_manage->container.port_update_result = full_message.subCopy(index, response_header[2]);
        ack->port_manage->encode_header_type[3] = true;
        m_logger->debug("below show port_update_status in UE");
        m_logger->debug("%s", pkt_hex_dump(ack->port_manage->container.port_update_result.toHexString()).c_str());
    }

    
    auto &pt = m_procedureTransactions[msg.pti];
    pt.state = EPtState::PENDING;
    pt.timer = newTransactionTimer(3591);
    pt.message = std::move(ack);
    pt.psi = msg.pduSessionId;

    /* Send SM message */
    sendSmMessage(msg.pduSessionId, *pt.message);
}

} // namespace nr::ue