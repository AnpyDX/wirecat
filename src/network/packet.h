#pragma once
#include <vector>
#include <string>
#include <optional>
#include <RawPacket.h>

#include "layers.h"

namespace WireCat::Network {
    struct Packet {
        Packet(pcpp::RawPacket* raw);

        std::string srcIP { "*.*.*.*" };
        std::string dstIP { "*.*.*.*" };
        std::string timeStamp;
        std::string packetType { "Unknown" };
        size_t frameLength;
        
        std::optional<LinkLayer> linkLayer;
        std::optional<NetworkLayer> networkLayer;
        std::optional<TransportLayer> transportLayer;
        std::optional<ApplicationLayer> applicationLayer;

        std::vector<uint8_t> rawData;
    };
}