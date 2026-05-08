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

    struct IPv6Address {
        IPv6Address() = default;
        IPv6Address(std::span<uint8_t, 16> rawData);
        IPv6Address(const IPv6Address&) = default;

        [[nodiscard]]
        std::string toString() const;

        std::array<uint16_t, 8> address {};
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
            } ARPLikeInfo {};

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
                uint8_t version;    // 4 bits
                uint8_t trafficClass;
                uint32_t flowLabel; // 20 bits
                uint16_t payloadLength;
                uint8_t nextHeader;
                uint8_t hopLimit;
                IPv6Address srcIPAddr;
                IPv6Address dstIPAddr;
                uint8_t protocol;
                std::span<uint8_t> data;
            } IPv6Info;
        };
    };

    struct TransportLayer {
    public:
        TransportLayer() = default;
        TransportLayer(uint8_t type, std::span<uint8_t> rawData);

        [[nodiscard]]
        bool isValid() const;

        [[nodiscard]]
        ApplicationLayer getNextLayer();
    private:
        void asTCP();
        void asUDP();
        void asICMP();

    public:
        enum class Type : uint8_t {
            TCP  = 6,
            UDP  = 17,
            ICMP = 1,
            Invalid
        } type = Type::Invalid;

        std::span<uint8_t> rawData;

        union {
            struct TCPInfoT {
                uint16_t srcPort;
                uint16_t dstPort;
                uint32_t seqNum;
                uint32_t ackNum;
                uint8_t dataOffset;
                bool ACK;
                bool SYN;
                bool FIN;
                bool RST;
                bool PSH;
                bool URG;
                uint16_t window;
                uint16_t checksum;
                uint16_t urgentPtr;
                std::span<uint8_t> options;
                std::span<uint8_t> data;
            } TCPInfo {};

            struct UDPInfoT {
                uint16_t srcPort;
                uint16_t dstPort;
                uint16_t length;
                uint16_t checksum;
                std::span<uint8_t> data;
            } UDPInfo;

            struct ICMPInfoT {
                uint8_t type;
                uint8_t code;
                uint16_t checksum;
                std::span<uint8_t> data;
            } ICMPInfo;
        };
    };

    struct ApplicationLayer {
    public:
        ApplicationLayer() = default;
        ApplicationLayer(std::span<uint8_t> rawData);

        [[nodiscard]]
        bool isvalid() const;

    public:
        enum class Type : uint8_t {
            HTTP,
            HTTPS,
            Invalid
        } type = Type::Invalid;

        std::span<uint8_t> rawData;
    };
}