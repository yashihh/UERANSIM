#include "dstt.hpp"
#include "utils/common.hpp"
#include "netinet/in.h"
#include "ue/tun/ptp.hpp"
#include <chrono>
#include <cstdint>
#include <cmath>


#define TLV_ORGANIZATION_EXTENSION			0x0003

#define SupportedPTPInstanceTypes           0x00E2
#define SupportedTransportTypes             0x00E3
#define SupportedDelayMechanisms            0x00E4
#define PTPGrandmasterCapable               0x00E5
#define gPTPGrandmasterCapable              0x00E6
#define SupportedPTPProfiles                0x00E7
#define NumberOfSupportedPTPInstances       0x00E8
#define PTPInstanceList                     0x00E9

// Supported PTP Instance Types
#define	OrdinaryClock        0x00
#define	BoundaryClock        0x01
#define	P2PTransparentClock  0x02
#define	E2ETransparentClock  0x03

// Supported transport types
#define IPv4      0b00000000
#define	IPv6      0b00000001
#define	Ethernet  0b00000010

// Supported PTP delay mechanisms
#define	E2E          0x01
#define	P2P          0x02
#define	COMMON_P2P   0x03
#define	SPECIAL      0x04
#define	NO_MECHANISM 0xFE

// Supported PTP profile
#define	SMPTE                0b00000000
#define	IEEE8021AS           0b00000001
#define	E2EDefault           0b00000010 // Default delay request-response profile
#define	P2PDefault           0b00000011 // Default delay peer-to-peer delay profile
#define	HighAccuracyDefault  0b00000100 // High Accuracy Delay Request-Response Default PTP profile


Dstt::Dstt(){}

Dstt::~Dstt(){}

void Dstt::ingress(OctetString &stream){
    uint16_t empty = 0;

    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::chrono::nanoseconds nanoSeconds = currentTime.time_since_epoch();
    auto tsi = nanoSeconds.count();
    int32_t tsi_second = (tsi / 1e9);
    int32_t tsi_fraction = (tsi % int(1e9));
    // printf("[DSTT] [%lld]tsi: [%ld, %ld]\n",tsi, tsi_second, tsi_fraction);
    
    // TLV type
    stream.appendOctet2(TLV_ORGANIZATION_EXTENSION);
    
    // length of TLV
    stream.appendOctet2(20);

    // Organizationally unique identifier(OUI) : 62 ~ 64 (:ethernet -14)
    stream.appendOctet3(stream.get3(48));

    // Organization subtype
    stream.appendOctet3(1);

    // ingress time(80 bits)
    stream.appendOctet2(empty);
    stream.appendOctet4(htonl(tsi_second));
    stream.appendOctet4(htonl(tsi_fraction));
    
}


// add TLV extention to the suffix
double Dstt::egress(OctetString &stream, int messageType){
    int len = stream.length();
    int64_t CorrectionField = 0;
    // uint64_t NRR_Nvalue= 0;
    // uint64_t NRR_Dvalue = 0;
    // double NRRValue = 0;
    // get NRR value
    // if(messageType == PTP_FOLLOW_UP){
    //     if( len < 67) return 0;
    //     // replace 5GS residence time to correction field : -61 ~ -68
    //     for(int i=len-36, j=7; i<=len-29; ++i, --j){
    //         // NRR_Nvalue += stream.data()[i] << (j*8);
    //         stream.data()[i] = 0;
    //     }
    //     for(int i=len-28, j=7; i<=len-21; ++i, --j){
    //         NRR_Dvalue += stream.data()[i] << (j*8);
    //         stream.data()[i] = 0;
    //     }
    //     // NRRValue = (double)NRR_Nvalue/(double)NRR_Dvalue;
    // }

   
    // compute 5GS residence time
    int32_t tsi_seconds_lsb = stream.get4I(len - 8);
    int32_t tsi_fraction = stream.get4I(len - 4);
    int64_t tsi = tsi_seconds_lsb*1e9 + tsi_fraction;

    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::chrono::nanoseconds nanoSeconds = currentTime.time_since_epoch();
    auto tse = nanoSeconds.count();

    int64_t residence_time = tse - tsi;
    // printf("[DSTT] resident: [%ld]\n", residence_time);

    //residence_time /= 1.1;

    // remove suffix part
    stream = stream.subCopy(0, len-20); // last 20 bytes
    len = stream.length();
    if(messageType == PTP_FOLLOW_UP){
        if( len < 67) return 0;
        // replace 5GS residence time to correction field : -61 ~ -68
        CorrectionField = stream.get8L(36); // 36-43 bytes
        octet8 t((CorrectionField>>16) + residence_time);
        // residence_time += (CorrectionField>>16);
        // residence_time *= 0.998;
        // octet8 t(residence_time);
        // residence_time = 0;
        // octet8 t(residence_time);
        
        for(int i=36, j=0; i<=41; ++i, ++j){
            stream.data()[i] = t[j+2];
            /*
            if(j < 4)
                stream.data()[i] = sec[j % 4];
            else
                stream.data()[i] = frac[j % 4];
            */
        }
        // TODO: Fix Checksum
        // checksum set to zero
        stream.data()[26] = 0x00;
        stream.data()[27] = 0x00;

    }else{
        // TODO: other type
        if( len < 45) return 0;
        // replace 5GS residence time to correction field : -39 ~ -46
        CorrectionField = stream.get8L(len-46);
        octet8 t((CorrectionField>>16) + residence_time);
        // residence_time += (CorrectionField>>16);
        // residence_time *= 0.998;
        // octet8 t(residence_time);
        // residence_time = 0;
        // octet8 t(residence_time);
        for(int i=len-46, j=0; i<=len-41; ++i, ++j){
            stream.data()[i] = t[j+2];
            /*
            if(j < 4)
                stream.data()[i] = sec[j % 4];
            else
                stream.data()[i] = frac[j % 4];
            */
        }
    }
    
    return residence_time/1e9;
}

void Dstt::PMIC_show_dstt_capability(OctetString &content){
    // PORT MANAGEMENT CAPABILITY body
    // capability list

    // Time Synchronization Information:
    // 1. Supported PTP instance types 
    content.appendOctet2(SupportedPTPInstanceTypes);
    // 2. Supported transport types
    content.appendOctet2(SupportedTransportTypes);
    // 3. Supported delay mechanisms
    content.appendOctet2(SupportedDelayMechanisms);
    // 4. PTP grandmaster capable
    content.appendOctet2(PTPGrandmasterCapable);
    // 5. gPTP grandmaster capable
    content.appendOctet2(gPTPGrandmasterCapable);
    // 6. Supported PTP profiles
    content.appendOctet2(SupportedPTPProfiles);
    // 7. Number of supported PTP instances	
    content.appendOctet2(NumberOfSupportedPTPInstances);
    // 8. PTP instance list
    content.appendOctet2(PTPInstanceList);

}

OctetString Dstt::DecodePMIC(int msg_type, OctetString &content, int *response_header){
    Dstt dstt;
    OctetString ack_content;
    if(msg_type == 1)   // MANAGE PORT COMMAND 
    {
        bool read_or_not = false, show_or_not = false, set_or_not = false;
        int capability, num_success_read = 0, num_unsuccess_read = 0;
        OctetString port_capability;
        OctetString success_read;
        OctetString unsuccess_read;
        for(int index = 0; index < content.length();){
            int operation = content.getI(index++);
            switch (operation)
            {
            case 1: // ack DSTT port capability
                PMIC_show_dstt_capability(port_capability);
                show_or_not = true;
                break;
            case 2: // read DSTT port parameter
                capability = content.get2I(index);
                if(capability == SupportedPTPInstanceTypes){
                    success_read.appendOctet2(capability);
                    success_read.appendOctet2(1);
                    success_read.appendOctet(E2ETransparentClock);
                    num_success_read++;
                }
                else if(capability == SupportedTransportTypes){
                    success_read.appendOctet2(capability);
                    success_read.appendOctet2(1);
                    success_read.appendOctet(IPv4);
                    num_success_read++;
                }
                else if(capability == SupportedDelayMechanisms){
                    success_read.appendOctet2(capability);
                    success_read.appendOctet2(1);
                    success_read.appendOctet(E2E);
                    num_success_read++;
                }
                else if(capability == PTPGrandmasterCapable){
                    success_read.appendOctet2(capability);
                    success_read.appendOctet2(1);
                    success_read.appendOctet(false);
                    num_success_read++;
                }
                else if(capability == gPTPGrandmasterCapable){
                    success_read.appendOctet2(capability);
                    success_read.appendOctet2(1);
                    success_read.appendOctet(false);
                    num_success_read++;
                }
                else if(capability == SupportedPTPProfiles){
                    success_read.appendOctet2(capability);
                    success_read.appendOctet2(1);
                    success_read.appendOctet(E2EDefault);
                    num_success_read++;
                }
                // else if(capability == NumberOfSupportedPTPInstances){
                //     success_read.appendOctet2(capability);
                //     success_read.appendOctet2(1);
                //     success_read.appendOctet(IPv4);
                //     num_success_read++;
                // }
                else{ // DSTT don't support
                    unsuccess_read.appendOctet2(capability);
                    unsuccess_read.appendOctet(0b00000001);
                    num_unsuccess_read++;
                }
                index+=2;
                read_or_not = true;
                break;
            case 3: // set DSTT port parameter
                set_or_not = true;
                break;
            default:
                break;
            }
        }

        if(show_or_not){
            ack_content.append(port_capability);
            response_header[0] = port_capability.length();
        }
        if(read_or_not){
            ack_content.appendOctet(num_success_read);
            ack_content.append(success_read);
            ack_content.appendOctet(num_unsuccess_read);
            ack_content.append(unsuccess_read);
            response_header[1] = success_read.length() + unsuccess_read.length() + 2;
        }
        if(set_or_not){
            
        }
    }
    else if(msg_type == 4){}  // PORT MANAGEMENT NOTIFY ACK
    return ack_content;
}