#define PTP

#define PTP_SYNC                    0x0 ///< Sync
#define PTP_DELAY_REQ               0x1 ///< Delay_Req
#define PTP_PDELAY_REQ              0x2 ///< Pdelay_Req
#define PTP_PDELAY_RESP             0x3 ///< Pdelay_Resp
#define PTP_FOLLOW_UP               0x8 ///< Follow_Up
#define PTP_DELAY_RESP              0x9 ///< Delay_Resp
#define PTP_PDELAY_RESP_FOLLOW_UP   0xA ///< Pdelay_Resp_Follow_Up
#define PTP_ANNOUNCE                0xB ///< Announce
#define PTP_SIGNALING               0xC ///< Signaling
#define PTP_MANAGEMENT              0xD ///< Management

#define PTP_EVENT_PORT              0x013f
#define PTP_GENERAL_PORT            0x0140
