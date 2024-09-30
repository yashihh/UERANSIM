//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//
#include <utils/octet_string.hpp>
#include <unordered_map>

class Dstt
{
  public:
    Dstt();
    virtual ~Dstt();

  public:
    static void ingress(OctetString &stream);
    static double egress(OctetString &stream, int messageType);
    static void PMIC_show_dstt_capability(OctetString &content);
    static OctetString DecodePMIC(int msg_type, OctetString &content, int *response_header);
};
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
#define NETWORK_IPv4      0b00000000
#define	NETWORK_IPv6      0b00000001
#define	NETWORK_Ethernet  0b00000010

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

// PTP Instance List
#define PTP_profile                                    0x0001
#define Transport_type                                 0x0002
#define Grandmaster_enabled                            0x0003
#define DefaultDS_clockIdentity                        0x0006
#define DefaultDS_clockQuality_clockClass              0x0007
#define DefaultDS_clockQuality_clockAccuracy           0x0008
#define DefaultDS_clockQuality_offsetScaledLogVariance 0x0009
#define DefaultDS_priority1                            0x000A
#define DefaultDS_priority2                            0x000B
#define DefaultDS_domainNumber                         0x000C
#define DefaultDS_sdoId                                0x000D
#define DefaultDS_instanceEnable                       0x000E
#define DefaultDS_instanceType                         0x0010
#define PortDS_PortIdentity                            0x0011
#define PortDS_PortState                               0x0012
#define PortDS_LogMinDelayReqInterval                  0x0013
#define PortDS_LogAnnounceInterval                     0x0014
#define PortDS_LogSyncInterval                         0x0016
#define PortDS_DelayMechanism                          0x0017
#define PortDS_LogMinPdelayReqInterval                 0x0018
#define PortDS_VersionNumber                           0x0019
#define PortDS_MinorVersionNumber                      0x001A
#define PortDS_DelayAsymmetry                          0x001B
#define PortDS_PortEnable                              0x001C
