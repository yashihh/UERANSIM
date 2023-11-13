#include <utils/octet_string.hpp>

struct Ethhdr
{
    OctetString h_dest;
    OctetString h_source;
    uid_t   eth_proc;
};
