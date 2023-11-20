#include "dstt.hpp"
#include "utils/common.hpp"
#include "netinet/in.h"
#include "ue/tun/ptp.hpp"

Dstt::Dstt(){}

Dstt::~Dstt(){}

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
    // int16_t tsi_seconds_msb = stream.get2I(len - 10);
    int32_t tsi_seconds_lsb = stream.get4I(len - 8);
    int64_t tsi_fraction = stream.get4I(len - 4);
    int64_t tse = utils::CurrentTimeMillis();
    int32_t tse_second = (tse / 1e3);
    int32_t tse_fraction = tse % int(1e3) * 1e6;

    int32_t residence_time_second = tse_second - tsi_seconds_lsb; 
    int32_t residence_time_fraction = tse_fraction - tsi_fraction;
    // printf("[DSTT] residence_time_second: [%ld] residence_time_fraction: [%ld] \n",  residence_time_second, residence_time_fraction);

    int64_t residence_time = (residence_time_second + (residence_time_fraction / 1e9)) * 1e9;

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
    
    return residence_time_second + (residence_time_fraction / 1e9);
}

