//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//
#include <utils/octet_string.hpp>

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
