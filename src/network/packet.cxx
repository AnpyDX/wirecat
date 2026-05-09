#include "packet.h"
#include <format>
#include <RawPacket.h>

namespace WireCat::Network {
    Packet::Packet(pcpp::RawPacket* raw)
    {
        // Copy raw data into current packet, as `raw` will be destroy then by pcap.
        rawData = std::vector<uint8_t>(raw->getRawData(), raw->getRawData() + raw->getRawDataLen());

        auto ts = raw->getPacketTimeStamp();
        timeStamp = std::format("{}(s) @{}(ns)", ts.tv_sec, ts.tv_nsec);
        frameLength = raw->getFrameLength();

        // NOTE: Link layer only supports Ethernet II & IEEE 802.3
        //       protocol. Packets with other protocol will be 
        //       left unparsed, or `Unknow` in display.
        if (raw->getLinkLayerType() != pcpp::LINKTYPE_ETHERNET) {
            return;
        }

        linkLayer = LinkLayer(std::span<uint8_t>(rawData.begin(), frameLength));
        networkLayer = linkLayer.getNextLayer();
        transportLayer = networkLayer.getNextLayer();
        applicationLayer = transportLayer.getNextLayer();
        
        switch (networkLayer.type) {
            case NetworkLayer::Type::IPv4:
                srcAddress = networkLayer.IPv4Info.srcIPAddr.toString();
                dstAddress = networkLayer.IPv4Info.dstIPAddr.toString();
                break;
            case NetworkLayer::Type::IPv6:
                srcAddress = networkLayer.IPv6Info.srcIPAddr.toString();
                dstAddress = networkLayer.IPv6Info.dstIPAddr.toString();
                break;
            default: {
                if (linkLayer.isValid()) {
                    srcAddress = linkLayer.srcAddress.toString();
                    dstAddress = linkLayer.dstAddress.toString();
                }
            }
        }

        if (transportLayer.isValid()) {
            switch (transportLayer.type) {
                case TransportLayer::Type::TCP: packetType = "TCP"; break;
                case TransportLayer::Type::UDP: packetType = "UDP"; break;
                case TransportLayer::Type::ICMP: packetType = "ICMP"; break;
                case TransportLayer::Type::ICMPv6: packetType = "ICMPv6"; break;
                default: break;
            }
        }
        else if (networkLayer.isValid()) {
            switch (networkLayer.type) {
                case NetworkLayer::Type::ARP: packetType = "ARP"; break;
                case NetworkLayer::Type::RARP: packetType = "RARP"; break;
                default: break;
            }
        }
        else {
            // TODO: add application support...
        }
    }
}