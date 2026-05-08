#pragma once
#include <vector>
#include <string>
#include <RawPacket.h>

#include "layers.h"

namespace WireCat::Network {
    struct Packet {
        Packet(pcpp::RawPacket* raw);

        std::string srcAddress { "Unknown" };
        std::string dstAddress { "Unknown" };
        std::string timeStamp;
        std::string packetType { "Unknown" };
        size_t frameLength;
        
        LinkLayer linkLayer;
        NetworkLayer networkLayer;
        TransportLayer transportLayer;
        ApplicationLayer applicationLayer;

        std::vector<uint8_t> rawData;
    };
}