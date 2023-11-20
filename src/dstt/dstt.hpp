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
    // static void ingress(OctetString &stream, uint16_t  tsi_seconds_msb, uint32_t tsi_second_lsb, uint32_t tsi_nanoseconds);
    static double egress(OctetString &stream, int messageType);
};
