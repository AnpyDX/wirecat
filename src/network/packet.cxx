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
        // networkLayer = linkLayer->getNextLayer();
        // transportLayer = networkLayer->getNextLayer();
        // srcIP = networkLayer.srcAddress.toString(), dstIP = networkLayer.dstAddress.toString();
        // packetType = ...
    }
}