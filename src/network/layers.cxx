#include "layers.h"
#include <cstdint>
#include <format>
#include <stdexcept>
#include <SystemUtils.h>


namespace {
    struct MemDecoder {
    public:
        MemDecoder(uint8_t* start, size_t maxBytes)
        : curPtr(start), boundPtr(start + maxBytes) {
            if (curPtr == nullptr) {
                throw std::runtime_error("start pointer cannot be nullptr");
            }
        }

        MemDecoder& add_u8(uint8_t& dst) {
            if (curPtr >= boundPtr) {
                throw std::runtime_error("memory decoder out of bound");
            }

            memcpy(&dst, curPtr, 1);
            curPtr += 1;
            return *this;
        }

        MemDecoder& add_u16(uint16_t& dst) {
            if (curPtr >= boundPtr) {
                throw std::runtime_error("memory decoder out of bound");
            }

            memcpy(&dst, curPtr, 2);
            dst = pcpp::netToHost16(dst);
            curPtr += 2;
            return *this;
        }

        MemDecoder& add_u32(uint32_t& dst) {
            if (curPtr >= boundPtr) {
                throw std::runtime_error("memory decoder out of bound");
            }

            memcpy(&dst, curPtr, 4);
            dst = pcpp::netToHost32(dst);
            curPtr += 4;
            return *this;
        }

        uint8_t* end() {
            if (curPtr != boundPtr) {
                throw std::runtime_error("current ptr != bounded ptr, indicating wrong memory layout performed");
            }
            return curPtr;
        }

    private:
        uint8_t* curPtr { nullptr };
        uint8_t* boundPtr { nullptr };
    };
}

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

    IPv4Address::IPv4Address(std::span<uint8_t, 4> rawData) {
        for (int i = 0; i < 4; i++) {
            address[i] = rawData[i];
        }
    }

    std::string IPv4Address::toString() const {
        return std::format(
            "{}.{}.{}.{}",
            address[0], address[1],
            address[2], address[3]
        );
    }

    IPv6Address::IPv6Address(std::span<uint8_t, 16> rawData) {
        for (int i = 0; i < 8; i++) {
            memcpy(&address[i], rawData.data() + sizeof(uint16_t) * i, sizeof(uint16_t));
            address[i] = pcpp::netToHost16(address[i]);
        }
    }

    std::string IPv6Address::toString() const {
        return std::format(
            "{:04X}:{:04X}:{:04X}:{:04X}:{:04X}:{:04X}:{:04X}:{:04X}",
            address[0], address[1], address[2], address[3],
            address[4], address[5], address[6], address[7]
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
            type = Type::EthernetII;

            nextLayerType = ident;
            // Remove HEAD (6 + 6 + 2 bytes) and CRC (4 bytes)
            nextLayerRawData = std::span<uint8_t>(rawData.begin() + 14, rawData.end() - 4);
        }
        else if (ident <= 1500) {
            /* IEEE 802.3 */
            type = Type::IEEE_802_3;

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
            throw std::runtime_error("invalid Link Layer raw data, in data = (0 + 12, 2 bytes)");
        }
    }

    bool LinkLayer::isValid() const {
        return type != Type::Invalid;
    }

    NetworkLayer LinkLayer::getNextLayer() {
        return NetworkLayer { nextLayerType, nextLayerRawData };
    }

    NetworkLayer::NetworkLayer(uint16_t type, std::span<uint8_t> rawData)
    : rawData(rawData)
    {
        switch (static_cast<Type>(type)) {
            case Type::ARP:
            case Type::RARP: asARPLike(); break;
            case Type::IPv4: asIPv4(); break;
            case Type::IPv6: asIPv6(); break;
            default: {
                this->type = Type::Invalid;
                return;
            }
        }
        this->type = static_cast<Type>(type);
    }

    bool NetworkLayer::isValid() const {
        return type != Type::Invalid;
    }

    TransportLayer NetworkLayer::getNextLayer() {
        switch (type) {
            case Type::IPv4: return TransportLayer { static_cast<uint8_t>(IPv4Info.protocol), IPv4Info.data };
            case Type::IPv6: return TransportLayer { static_cast<uint8_t>(IPv6Info.protocol), IPv6Info.data };
            default: break;
        }
        return TransportLayer {};
    }

    void NetworkLayer::asARPLike() {
        if (rawData.size() < 28) {
            // Invalid ARP/RARP msg, as raw-data is less than 24 bytes
            type = Type::Invalid;
            return;
        }

        auto& info = ARPLikeInfo;
        uint8_t* cur = 
            MemDecoder(rawData.data(), 8)
                .add_u16(info.hardwareType)
                .add_u16(info.protocolType)
                .add_u8(info.hardwareLength)
                .add_u8(info.protocolLength)
                .add_u16(info.operation)
            .end();

        info.srcMACAddr = MACAddress { std::span<uint8_t, 6>(cur, 6) };
            cur += 6;
        info.srcIPAddr = IPv4Address { std::span<uint8_t, 4>(cur, 4) };
            cur += 4;
        info.dstMACAddr = MACAddress { std::span<uint8_t, 6>(cur, 6) };
            cur += 6;
        info.dstIPAddr = IPv4Address { std::span<uint8_t, 4>(cur, 4) };
    }

    void NetworkLayer::asIPv4() {
        auto& info = IPv4Info;

        uint8_t  t0; // version (4-bits) + headerLength(4-bits)
        uint16_t t1; // flags (3-bits) + fragmentOffset(13-bits)

        uint8_t* cur = 
            MemDecoder(rawData.data(), 12)
                .add_u8(t0)
                .add_u8(info.typeOfService)
                .add_u16(info.totalLength)
                .add_u16(info.identification)
                .add_u16(t1)
                .add_u8(info.timeToLive)
                .add_u8(info.protocol)
                .add_u16(info.headerChecksum)
            .end();

        info.version = (t0 & 0xF0) >> 4;
        info.headerLength = t0 & 0x0F;
        info.flags = (t1 & 0xE0000) >> 13;
        info.fragmentOffset = t1 & 0x1FFF;

        info.srcIPAddr = IPv4Address { std::span<uint8_t, 4>(cur, 4) };
            cur += 4;
        info.dstIPAddr = IPv4Address { std::span<uint8_t, 4>(cur, 4) };
            cur += 4;
        if (info.headerLength > 5) {
            info.options = std::span<uint8_t>(cur, static_cast<size_t>(info.headerLength - 5) * 4);
            cur += static_cast<size_t>(info.headerLength - 5) * 4;
        }
        else if (info.headerLength < 5) {
            // IPv4's header is always longer than 20 bytes.
            type = Type::Invalid;
            return;
        }
        info.data = std::span<uint8_t>(cur, rawData.size() - static_cast<size_t>(info.headerLength) * 4);
    }

    void NetworkLayer::asIPv6() {
        auto& info = IPv6Info;
        uint32_t t0;

        uint8_t* cur =
            MemDecoder(rawData.data(), 8)
                .add_u32(t0)
                .add_u16(info.payloadLength)
                .add_u8(info.nextHeader)
                .add_u8(info.hopLimit)
            .end();
        
        info.version = (t0 & 0xF0000000) >> 28;
        info.trafficClass = (t0 & 0x0FF00000) >> 20;
        info.flowLabel = t0 & 0x000FFFFF;

        info.srcIPAddr = IPv6Address { std::span<uint8_t, 16>(cur, 16) };
            cur += 16;
        info.dstIPAddr = IPv6Address { std::span<uint8_t, 16>(cur, 16) };
            cur += 16;

        uint8_t nextHdr = info.nextHeader;
        while (cur < (rawData.data() + rawData.size())) {
            if (nextHdr == 1 || nextHdr == 6 || nextHdr == 17 || nextHdr == 58) {
                /* ICMP = 1, TCP = 6, UDP = 17, ICMPv6 = 58 */
                info.protocol = nextHdr;
                info.data = std::span<uint8_t>(cur, rawData.data() + rawData.size() - cur);
                break;
            }
            else if (nextHdr == 0 || nextHdr == 43 || nextHdr == 60) {
                /* Hop-by-Hop Options, Routing, Destination Options */
                uint8_t hdrLen;

                MemDecoder(cur, rawData.data() + rawData.size() - cur)
                    .add_u8(nextHdr)
                    .add_u8(hdrLen);

                cur += static_cast<size_t>(hdrLen + 1) * 8;
            }
            else {
                // FIXME: Not supported options will be treated as invalid packet.
                type = Type::Invalid;
                break;
            }
        }
    }

    TransportLayer::TransportLayer(uint8_t type, std::span<uint8_t> rawData)
    : rawData(rawData)
    {
        switch (static_cast<Type>(type)) {
            case Type::TCP: asTCP(); break;
            case Type::UDP: asUDP(); break;
            case Type::ICMP: asICMP(); break;
            case Type::ICMPv6: asICMPv6(); break;
            default: {
                this->type = Type::Invalid;
                return;
            }
        }
        this->type = static_cast<Type>(type);
    }

    bool TransportLayer::isValid() const {
        return type != Type::Invalid;
    }

    ApplicationLayer TransportLayer::getNextLayer() {
        switch (type) {
            case Type::TCP: return ApplicationLayer { TCPInfo.data };
            case Type::UDP: return ApplicationLayer { UDPInfo.data };
            default: break;
        }
        return ApplicationLayer {};
    }

    void TransportLayer::asTCP() {
        auto& info = TCPInfo;

        uint8_t t0, t1;
        MemDecoder(rawData.data(), 20)
            .add_u16(info.srcPort)
            .add_u16(info.dstPort)
            .add_u32(info.seqNum)
            .add_u32(info.ackNum)
            .add_u8(t0)
            .add_u8(t1)
            .add_u16(info.window)
            .add_u16(info.checksum)
            .add_u16(info.urgentPtr)
        .end();

        info.dataOffset = (t0 >> 4) * 4;
        if (info.dataOffset > rawData.size()) {
            type = Type::Invalid;
            return;
        }

        uint16_t flags = (t0 << 8) | t1;
        info.NS  = flags & (1 << 8);
        info.CWR = flags & (1 << 7);
        info.ECE = flags & (1 << 6);
        info.URG = flags & (1 << 5);
        info.ACK = flags & (1 << 4);
        info.PSH = flags & (1 << 3);
        info.RST = flags & (1 << 2);
        info.SYN = flags & (1 << 1);
        info.FIN = flags & (1 << 0);

        info.options = std::span<uint8_t>(rawData.data() + 20, rawData.data() + info.dataOffset);
        info.data = std::span<uint8_t>(rawData.data() + info.dataOffset, rawData.data() + rawData.size());
    }

    void TransportLayer::asUDP() {
        auto& info = UDPInfo;

        uint8_t* cur =
            MemDecoder(rawData.data(), 8)
                .add_u16(info.srcPort)
                .add_u16(info.dstPort)
                .add_u16(info.length)
                .add_u16(info.checksum)
            .end();

        info.data = std::span<uint8_t>(cur, rawData.data() + rawData.size());
    }

    void TransportLayer::asICMP() {
        auto& info = ICMPInfo;

        uint8_t* cur = 
            MemDecoder(rawData.data(), 4)
                .add_u8(info.type)
                .add_u8(info.code)
                .add_u16(info.checksum)
            .end();

        info.data = std::span<uint8_t>(cur, rawData.data() + rawData.size());
    }

    void TransportLayer::asICMPv6() {
        auto& info = ICMPv6Info;

        uint8_t* cur = 
            MemDecoder(rawData.data(), 4)
                .add_u8(info.type)
                .add_u8(info.code)
                .add_u16(info.checksum)
            .end();
    }

    ApplicationLayer::ApplicationLayer(std::span<uint8_t> rawData)
    : rawData(rawData), type(Type::Unknown) {}

    bool ApplicationLayer::isvalid() const {
        return type != Type::Invalid;
    }
}