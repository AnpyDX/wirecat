#pragma once
#include <span>
#include <array>
#include <string>
#include <cstdint>
#include <optional>

namespace WireCat::Network {
    struct MACAddress {
        MACAddress() = default;
        MACAddress(std::span<uint8_t, 6> rawData);
        MACAddress(const MACAddress&) = default;

        [[nodiscard]]
        std::string toString() const;

        std::array<uint8_t, 6> address {};
    };

    struct IPv4Address {};
    struct IPv6Address {};
    struct IPAddress {
        union {
            IPv4Address addrV4;
            IPv6Address addrV6;
        };
    };

    struct NetworkLayer;
    struct TransportLayer;

    /**
     * @brief Link Layer Representation.
     * @note  Impl `IEEE 802.3` & `Ethernet II`.
     */
    struct LinkLayer {
    public:
        LinkLayer(std::span<uint8_t> rawData);

        std::optional<NetworkLayer> getNextLayer();
    public:
        enum class Type : uint8_t {
            IEEE_802_3, EthernetII
        } type;

        std::span<uint8_t> rawData;

        /* 802.3 MAC */
        MACAddress srcAddress;
        MACAddress dstAddress;
        uint16_t length;

        /* 802.2 LLC */
        uint8_t DSAP;
        uint8_t SSAP;
        uint8_t CNTL;

        /* 802.2 SNAP */
        uint32_t orgCode;
        uint16_t nextLayerType;

        std::span<uint8_t> nextLayerRawData;
    };

    struct NetworkLayer {
        enum class Type : uint8_t {
            RARP, ARP, IPv4, IPv6
        };

        std::span<uint8_t> rawData;
    };

    struct TransportLayer {
        enum class Type : uint8_t {
            TCP, UDP, ICMP
        };

        std::span<uint8_t> rawData;
    };

    struct ApplicationLayer {
        enum class Type : uint8_t {
            HTTP, HTTPS
        };

        std::span<uint8_t> rawData;
    };
}