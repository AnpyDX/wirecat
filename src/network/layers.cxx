#include "layers.h"
#include <format>
#include <stdexcept>
#include <SystemUtils.h>

namespace WireCat::Network {
    MACAddress::MACAddress(std::span<uint8_t, 6> rawData) {
        for (int i = 0; i < 6; i++) {
            address[i] = rawData[i];
        }
    }

    std::string MACAddress::toString() const {
        return std::format(
            "{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}",
            address[0], address[1], address[2],
            address[3], address[4], address[5]
        );
    }

    LinkLayer::LinkLayer(std::span<uint8_t> rawData)
    : rawData(rawData)
    {
        dstAddress = MACAddress { std::span<uint8_t, 6>(rawData.begin(), 6) };
        srcAddress = MACAddress { std::span<uint8_t, 6>(rawData.begin() + 6, 6) };

        uint16_t ident;
        memcpy(&ident, rawData.data() + 12, 2);
        ident = pcpp::netToHost16(ident);

        if (ident >= 1536) {
            /* Ethernet II */
            nextLayerType = ident;
            // Remove HEAD (6 + 6 + 2 bytes) and CRC (4 bytes)
            nextLayerRawData = std::span<uint8_t>(rawData.begin() + 14, rawData.end() - 4);
        }
        else if (ident <= 1500) {
            /* IEEE 802.3 */
            length = ident;
            DSAP = *(rawData.data() + 14);
            SSAP = *(rawData.data() + 15);
            CNTL = *(rawData.data() + 16);

            orgCode = 0;
            memcpy(&orgCode, rawData.data() + 17, 3);
            orgCode = pcpp::netToHost32(orgCode);

            memcpy(&nextLayerType, rawData.data() + 20, 2);
            nextLayerType = pcpp::netToHost16(nextLayerType);

            // Remove HEAD (14 + 3 + 5 bytes) and CRC (4 bytes)
            nextLayerRawData = std::span<uint8_t>(rawData.begin() + 22, rawData.end() - 4);
        }
        else {
            throw std::runtime_error("invalid Link Layer raw data, in offset = (0 + 12, 2)");
        }
    }
}