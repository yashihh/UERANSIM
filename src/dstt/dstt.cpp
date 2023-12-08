#include "dstt.hpp"
#include "utils/common.hpp"
#include "netinet/in.h"
#include "ue/tun/ptp.hpp"
#include <chrono>
#include "ptp_suffix.h"

Dstt::Dstt(){}

Dstt::~Dstt(){}

// TODO: add TLV extention to the suffix
static void ingress(OctetString &stream, int64_t tsi){
    uint16_t empty = 0;
    int len = stream.length();

    // TLV type
    stream.appendOctet2(TLV_ORGANIZATION_EXTENSION);
    
    // length of TLV
    stream.appendOctet2(sizeof(ptp_suffix));

    // Organizationally unique identifier(OUI) : -32 ~ -34
    stream.appendOctet3(stream.get3(len-34));

    // Organization subtype
    stream.appendOctet3(1);

    // ingress time(80 bits)
    stream.appendOctet4(htonl(tsi_second));
    stream.appendOctet2(empty);
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
        for(int i=36, j=0; i<=43; ++i, ++j){
            stream.data()[i] = t[j+2];
            /*
            if(j < 4)
                stream.data()[i] = sec[j % 4];
            else
                stream.data()[i] = frac[j % 4];
            */
        }
    }else{
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

