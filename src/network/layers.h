#pragma once
#include <span>
#include <array>
#include <string>
#include <cstdint>

namespace WireCat::Network {
    struct MACAddress {
        MACAddress() = default;
        MACAddress(std::span<uint8_t, 6> rawData);
        MACAddress(const MACAddress&) = default;

        [[nodiscard]]
        std::string toString() const;

        std::array<uint8_t, 6> address {};
    };

    struct IPv4Address {
        IPv4Address() = default;
        IPv4Address(std::span<uint8_t, 4> rawData);
        IPv4Address(const IPv4Address&) = default;

        [[nodiscard]]
        std::string toString() const;

        std::array<uint8_t, 4> address {};
    };
    struct IPv6Address {};
    struct IPAddress {
        union {
            IPv4Address addrV4;
            IPv6Address addrV6;
        };
    };

    struct NetworkLayer;
    struct TransportLayer;
    struct ApplicationLayer;

    /**
     * @brief Link Layer Representation.
     * @note  Impl `IEEE 802.3` & `Ethernet II`.
     */
    struct LinkLayer {
    public:
        LinkLayer() = default;
        LinkLayer(std::span<uint8_t> rawData);

        [[nodiscard]]
        bool isValid() const;

        [[nodiscard]]
        NetworkLayer getNextLayer();

    public:
        enum class Type : uint8_t {
            IEEE_802_3,
            EthernetII,
            Invalid
        } type = Type::Invalid;

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
    public:
        NetworkLayer() = default;
        NetworkLayer(uint16_t type, std::span<uint8_t> rawData);

        [[nodiscard]]
        bool isValid() const;

        [[nodiscard]]
        TransportLayer getNextLayer();

    private:
        void asARPLike();
        void asIPv4();
        void asIPv6();

    public:
        enum class Type : uint16_t {
            ARP  = 0x0806,
            RARP = 0x0835,
            IPv4 = 0x0800,
            IPv6 = 0x08DD,
            Invalid
        } type = Type::Invalid;

        std::span<uint8_t> rawData;

        union {
            /* ARP or RARP */
            struct ARPLikeInfoT {
                uint16_t hardwareType;
                uint16_t protocolType;
                uint8_t hardwareLength;
                uint8_t protocolLength;
                uint16_t operation;
                MACAddress srcMACAddr;
                IPv4Address srcIPAddr;
                MACAddress dstMACAddr;
                IPv4Address dstIPAddr;
            } ARPLikeInfo;

            /* IPv4 */
            struct IPv4InfoT {
                uint8_t version;      // 4 bits
                uint8_t headerLength; // 4 bits
                uint8_t typeOfService;
                uint16_t totalLength;
                uint16_t identification;
                uint8_t flags;           // 3 bits
                uint16_t fragmentOffset; // 13 bits
                uint8_t timeToLive;
                uint8_t protocol;
                uint16_t headerChecksum;
                IPv4Address srcIPAddr;
                IPv4Address dstIPAddr;
                std::span<uint8_t> options;
                std::span<uint8_t> data;
            } IPv4Info;

            /* IPv6 */
            struct IPv6InfoT {
                uint8_t version;
            } IPv6Info;
        };
    };

    struct TransportLayer {
    public:
        TransportLayer() = default;
        TransportLayer(std::span<uint8_t> rawData);

        [[nodiscard]]
        bool isValid();

        [[nodiscard]]
        ApplicationLayer getNextLayer();
    public:
        enum class Type : uint8_t {
            TCP, UDP, ICMP, Invalid
        } type = Type::Invalid;

        std::span<uint8_t> rawData;
    };

    struct ApplicationLayer {
    public:
        ApplicationLayer() = default;
        ApplicationLayer(std::span<uint8_t> rawData);

        [[nodiscard]]
        bool isvalid();

    public:
        enum class Type : uint8_t {
            HTTP, HTTPS, Invalid
        } type = Type::Invalid;

        std::span<uint8_t> rawData;
    };
}