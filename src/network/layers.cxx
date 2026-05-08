#include "layers.h"
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

        uint8_t* current() {
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
            address[2], address[4]
        );
    }

    IPv6Address::IPv6Address(std::span<uint8_t, 128> rawData) {
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

    void NetworkLayer::asARPLike() {
        if (rawData.size() < 28) {
            throw std::runtime_error("invalid ARP/RARP msg, as raw-data is less than 24 bytes");
        }

        auto& info = ARPLikeInfo;
        uint8_t* cur = 
            MemDecoder(rawData.data(), 8)
                .add_u16(info.hardwareType)
                .add_u16(info.protocolType)
                .add_u8(info.hardwareLength)
                .add_u8(info.protocolLength)
                .add_u16(info.operation)
            .current();

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
            .current();

        info.version = t0 & 0b00000000;
        info.headerLength = t0 & 0x10;
        info.flags = 
    }

    void NetworkLayer::asIPv6() {

    }
}