#include <mutex>
#include <format>
#include <iostream>
#include <stdexcept>

#include <imgui.h>
#include <imgui_hex.h>
#include <imgui_internal.h>

#include "application.h"
#include "network/layers.h"
#include "widgets/text.h"
#include "widgets/menu.h"
#include "widgets/fold.h"
#include "widgets/table.h"
#include "widgets/widget.h"
#include "widgets/window.h"
#include "widgets/button.h"
#include "widgets/separator.h"

#include "network/packet.h"

#include "PcapLiveDevice.h"
#include "PcapLiveDeviceList.h"


#define WIRECAT_BUILD_STR "v1.0 2026.5 build"
constexpr int DEFAULT_WIDTH = 1500;
constexpr int DEFAULT_HEIGHT = 900;

using namespace WireCat::Network;
using namespace WireCat::Widgets;

class App: public WireCat::Application {
public:
    App(): Application("WireCat", DEFAULT_WIDTH, DEFAULT_HEIGHT) {
        /* Get all live network devices */
        networkDeviceList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();
        if (networkDeviceList.empty()) {
            throw std::runtime_error("failed to find available live network device");
        }

        /* Construct UI widgets */
        mainBar.add(
            Menu("WireCat")
                .add(MenuItem("新建分析", "New analyze", [this]() {
                    if (currentDevice != nullptr) {
                        // Stop current capture and close used device.
                        while(currentDevice->captureActive()) {
                            currentDevice->stopCapture();
                        }
                        while (currentDevice->isOpened()) {
                            currentDevice->close();
                        }
                        currentDevice = nullptr;
                        
                        // Clear all captured packets to release memory.
                        {
                            std::lock_guard lock(capturedPacketsMutex);
                            capturedPackets.clear();
                            capturedPackets.shrink_to_fit();
                        }
                        
                        packetListTable->rebind(capturedPackets);
                    }

                    // Reset MainWorkSpace.
                    showMainWorkSpace = false;
                    previewFoldGroup->unselectAll();
                    previewFoldGroup->setHidden(true);

                    // Show device selection UI.
                    deviceListWindow.setHidden(false);
                }))
                .add(MenuItem("退出", "Exit", [this]() {
                    glfwSetWindowShouldClose(window, true);
                }))
                .add(Separator())
                .add(Text(WIRECAT_BUILD_STR).setColor(1.0, 1.0, 1.0, 0.8).into())
            .into()
        );

        deviceListWindow = 
            Window("##DeviceList")
                .setTitleBar(false)
                .setMovable(false)
                .setResizable(false)
                .setFixedSize(700, 400)
                .add(SeparatorText("选择目标设备以开始分析"))
                .add(
                    Table<pcpp::PcapLiveDevice*>(networkDeviceList)
                        .setFixedOuterSize(0, 300)
                        .addColumn("描述信息", [](const pcpp::PcapLiveDevice* dev, int) {
                            return dev->getDesc();
                        })
                        .addColumn("IP 地址", [](const pcpp::PcapLiveDevice* dev, int) {
                            for (auto& addr : dev->getIPAddresses()) {
                                return addr.toString();
                            }
                            return std::string("null");
                        })
                    .into(), &deviceListTable
                )
                .add(
                    Once([]() {
                        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 120, 0));
                        ImGui::SameLine();
                    })
                )
                .add(
                    Button("确认", [this]() {
                        if (deviceListTable->getSelected().has_value()) {
                            size_t deviceIdx = deviceListTable->getSelected().value();
                            currentDevice = networkDeviceList[deviceIdx];

                            deviceListWindow.setHidden(true);
                            showMainWorkSpace = true;
                            
                            if (!currentDevice->open()) {
                                throw std::runtime_error(std::format("failed to open device {}", currentDevice->getDesc()));
                            }
                            currentDevice->startCapture([&](pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) {
                                auto packetList = reinterpret_cast<std::vector<Packet>*>(cookie);
                                {
                                    std::lock_guard lock(capturedPacketsMutex);
                                    packetList->emplace_back(packet);
                                }
                            }, &capturedPackets);
                        }
                        else {
                            ImGui::OpenPopup("Info##deviceListPopup");
                        }
                    })
                    .setFixedSize(100, 0)
                    .into()
                )
                .add(
                    Once([]() {
                        if (ImGui::BeginPopupModal("Info##deviceListPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("提示：请选择一个有效设备。");
                            ImGui::Dummy(ImVec2(70, 0));
                            ImGui::SameLine();
                            if (ImGui::Button("关闭")) { ImGui::CloseCurrentPopup(); }
                            ImGui::EndPopup();
                        }
                    })
                )
            .into();

        packetListWindow = 
            Window("抓取列表##packetList")
                .setTitleBar(false)
                .add(
                    Once([]() {
                        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x / 2 - 60, 0));
                        ImGui::SameLine();
                    })
                )
                .add(
                    Button("Stop", [this]() {
                        auto device = networkDeviceList[deviceListTable->getSelected().value()];
                        while (device->captureActive()) {
                            device->stopCapture();
                        }
                    })
                    .setFixedSize(60, 30)
                    .into()
                )
                .add(
                    Once([this]() {
                        ImGui::SameLine();
                        ImGui::Text("已抓取：%lld", capturedPackets.size());
                    })
                )
                .add(
                    Table<Packet>(capturedPackets)
                        .addColumn("No.", [](const Packet&, int row) {
                            return std::to_string(row);
                        })
                        .addColumn("时间戳", [](const Packet& pkt, int) {
                            return pkt.timeStamp;
                        })
                        .addColumn("起始地址", [](const Packet& pkt, int) {
                            return pkt.srcAddress;
                        })
                        .addColumn("目标地址", [](const Packet& pkt, int) {
                            return pkt.dstAddress;
                        })
                        .addColumn("报文类型", [](const Packet& pkt, int) {
                            return pkt.packetType;
                        })
                    .into(), &packetListTable
                )
            .into();
        
        packetPreviewWindow = 
            Window("详细信息##packetPreview")
                .setTitleBar(false)
                .add(
                    FoldGroup()
                        .setHidden(true)
                        .add(
                            Fold("Raw Packet")
                                .add(Once([this]() {
                                    const auto& pkt = capturedPackets[packetListTable->getSelected().value()];
                                    ImGui::Text("Time Stamp: %s", pkt.timeStamp.c_str());
                                    ImGui::Text("Raw Packet Length: %lld bytes", pkt.rawData.size());
                                    ImGui::Text("Frame Length: %lld bytes", pkt.frameLength);
                                }))
                            .into(), &frameInfoFold)
                        .add(
                            Fold("Link")
                                .add(Once([this]() {
                                    const auto& pkt = capturedPackets[packetListTable->getSelected().value()];
                                    if (pkt.linkLayer.isValid()) {
                                        auto& layer = pkt.linkLayer;
                                        if (layer.type == LinkLayer::Type::EthernetII) {
                                            ImGui::Text("协议类型: Ethernet II");
                                            ImGui::Text("> 起始 MAC: %s", layer.srcAddress.toString().c_str());
                                            ImGui::Text("> 目标 MAC: %s", layer.dstAddress.toString().c_str());
                                        }
                                        else {
                                            ImGui::Text("协议类型: IEEE 802.3");
                                            ImGui::Text("> 起始 MAC: %s", layer.srcAddress.toString().c_str());
                                            ImGui::Text("> 目标 MAC: %s", layer.dstAddress.toString().c_str());
                                            ImGui::Text("> LLC: DSAP=%#x, SSAP=%#x, CNTL=%#x", layer.DSAP, layer.SSAP, layer.CNTL);
                                            ImGui::Text("> SNAP: ORG Code=%#x", layer.orgCode);
                                        }
                                        ImGui::Text("> Type: %#x", layer.nextLayerType);
                                    }
                                    else {
                                        ImGui::Text("Unknown");
                                    }
                                }))
                            .into(), &linkLayerInfoFold)
                        .add(
                            Fold("Network / IP")
                                .add(Once([this]() {
                                    const auto& pkt = capturedPackets[packetListTable->getSelected().value()];
                                    if (pkt.networkLayer.isValid()) {
                                        auto& layer = pkt.networkLayer;
                                        switch (layer.type) {
                                            case NetworkLayer::Type::IPv4: {
                                                auto& info = layer.IPv4Info;
                                                ImGui::Text("协议类型: IPv4");
                                                ImGui::Text("> 服务类型: %u", info.typeOfService);
                                                ImGui::Text("> 报文总长: %u", info.totalLength);
                                                ImGui::Text("> 标识符: %u", info.identification);
                                                ImGui::Text("> TTL: %u", info.timeToLive);
                                                ImGui::Text("> 上层协议: %u", info.protocol);
                                                ImGui::Text("> 起始 IP: %s", info.srcIPAddr.toString().c_str());
                                                ImGui::Text("> 目的 IP: %s", info.dstIPAddr.toString().c_str());
                                                break;
                                            }
                                            case NetworkLayer::Type::IPv6: {
                                                auto& info = layer.IPv6Info;
                                                ImGui::Text("协议类型: IPv6");
                                                ImGui::Text("> 流类别: %#x", info.trafficClass);
                                                ImGui::Text("> 流标签: %#x", info.flowLabel);
                                                ImGui::Text("> 有效载荷长度: %u", info.payloadLength);
                                                ImGui::Text("> 跳数限制: %u", info.hopLimit);
                                                ImGui::Text("> 起始 IP: %s", info.srcIPAddr.toString().c_str());
                                                ImGui::Text("> 目的 IP: %s", info.dstIPAddr.toString().c_str());
                                                break;
                                            }
                                            case NetworkLayer::Type::ARP:
                                            case NetworkLayer::Type::RARP: {
                                                auto& info = layer.ARPLikeInfo;
                                                ImGui::Text("协议类型: %s", layer.type == NetworkLayer::Type::ARP ? "ARP" : "RARP");
                                                ImGui::Text("> 硬件类型: %#x", info.hardwareType);
                                                ImGui::Text("> 协议类型: %#x", info.protocolType);
                                                ImGui::Text("> OP: %s", [info]() -> const char* {
                                                    uint16_t op = info.operation;
                                                    switch (op) {
                                                        case 1: return "ARP 请求";
                                                        case 2: return "ARP 应答";
                                                        case 3: return "RARP 请求";
                                                        case 4: return "RARP 应答";
                                                        default: return "unkown";
                                                    }
                                                }());
                                                ImGui::Text("> 起始 MAC: %s", info.srcMACAddr.toString().c_str());
                                                ImGui::Text("> 起始 IP: %s", info.srcIPAddr.toString().c_str());
                                                ImGui::Text("> 目标 MAC: %s", info.dstMACAddr.toString().c_str());
                                                ImGui::Text("> 目标 IP: %s", info.dstIPAddr.toString().c_str());
                                                break;
                                            }
                                            default: break;
                                        }
                                    }
                                }))
                            .into(), &networkLayerInfoFold)
                        .add(
                            Fold("Transport / ICMP")
                                .add(Once([this]() {
                                    const auto& pkt = capturedPackets[packetListTable->getSelected().value()];
                                    if (pkt.transportLayer.isValid()) {
                                        auto& layer = pkt.transportLayer;
                                        switch (layer.type) {
                                            case TransportLayer::Type::TCP: {
                                                auto& info = layer.TCPInfo;
                                                ImGui::Text("协议类型: TCP");
                                                ImGui::Text("> 起始端口: %u", info.srcPort);
                                                ImGui::Text("> 目标端口: %u", info.dstPort);
                                                ImGui::Text("> 序列号: %u", info.seqNum);
                                                ImGui::Text("> 确认号: %u", info.ackNum);
                                                ImGui::Text(
                                                    "> NS = %u, CWR = %u, ECE = %u, URG = %u, ACK = %u",
                                                    info.NS, info.CWR, info.ECE, info.URG, info.ACK
                                                );
                                                ImGui::Text(
                                                    "> PSH = %u, RST = %u, SYN = %u, FIN = %u",
                                                    info.PSH, info.RST, info.SYN, info.FIN
                                                );
                                                ImGui::Text("> 窗口大小: %u", info.window);
                                                ImGui::Text("> 校验和: %#x", info.checksum);
                                                ImGui::Text("> 紧急指针: %u", info.urgentPtr);
                                                break;
                                            }
                                            case TransportLayer::Type::UDP: {
                                                auto& info = layer.UDPInfo;
                                                ImGui::Text("协议类型: UDP");
                                                ImGui::Text("> 起始端口: %u", info.srcPort);
                                                ImGui::Text("> 目标端口: %u", info.dstPort);
                                                ImGui::Text("> 上层报文长度: %u", info.length);
                                                ImGui::Text("> 校验和: %#x", info.checksum);
                                                break;
                                            }
                                            case TransportLayer::Type::ICMP: {
                                                auto& info = layer.ICMPInfo;
                                                ImGui::Text("协议类型: ICMP");
                                                ImGui::Text("> 类型: %s", std::format("{} (code = {})", getICMPDesc(info.type), info.code).c_str());
                                                ImGui::Text("> 原始信息: type = %u, code = %u", info.type, info.code);
                                                ImGui::Text("> 校验和: %#x", info.checksum);
                                                ImGui::Text("> 数据长度: %lld", info.data.size());
                                                break;
                                            }
                                            case TransportLayer::Type::ICMPv6: {
                                                auto& info = layer.ICMPInfo;
                                                ImGui::Text("协议类型: ICMPv6");
                                                ImGui::Text("> 类型: %s", std::format("{} (code = {})", getICMPv6Desc(info.type), info.code).c_str());
                                                ImGui::Text("> 原始信息: type = %u, code = %u", info.type, info.code);
                                                ImGui::Text("> 校验和: %#x", info.checksum);
                                                ImGui::Text("> 数据长度: %lld", info.data.size());
                                            }
                                            default: break;
                                        }
                                    }
                                }))
                            .into(), &transportLayerInfoFold)
                        .add(
                            Fold("Application")
                                .add(Once([this]() {
                                    const auto& pkt = capturedPackets[packetListTable->getSelected().value()];
                                    if (pkt.applicationLayer.isValid()) {
                                        auto& layer = pkt.applicationLayer;
                                        switch (layer.type) {
                                            case ApplicationLayer::Type::DHCP: {
                                                auto& info = layer.DHCPInfo;
                                                ImGui::Text("协议类型: DHCP");
                                                ImGui::Text("> OP = %#x", info.op);
                                                ImGui::Text("> 硬件类型 = %#x, 硬件地址长度 = %#x", info.htype, info.hlen);
                                                ImGui::Text("> 报文中继次数 (Hops): %u", info.hops);
                                                ImGui::Text("> 事务ID: %u", info.xid);
                                                ImGui::Text("> secs: %u 秒", info.secs);
                                                ImGui::Text("> 客户端地址 (Ci Address): %s", info.ciaddr.toString().c_str());
                                                ImGui::Text("> 分配地址   (Yi Address): %s", info.yiaddr.toString().c_str());
                                                ImGui::Text("> Si Address: %s", info.siaddr.toString().c_str());
                                                ImGui::Text("> Gi Address: %s", info.giaddr.toString().c_str());
                                                ImGui::Text("> 客户端 MAC 地址: %s", info.chaddr.toString().c_str());  
                                                break;
                                            }
                                            case ApplicationLayer::Type::DHCPv6: {
                                                auto& info = layer.DHCPv6Info;
                                                ImGui::Text("协议类型: DHCPv6");
                                                ImGui::Text("> 消息类型: %#x", info.msgType);
                                                break;
                                            }
                                            case ApplicationLayer::Type::HTTP: {
                                                ImGui::Text("协议类型: HTTP");
                                                ImGui::Text("Check the hex editor ->");
                                                break;
                                            }
                                            case ApplicationLayer::Type::HTTPS: {
                                                ImGui::Text("协议类型: HTTPS");
                                                ImGui::Text("Check the hex editor ->");
                                                break;
                                            }
                                            default: break;
                                        }
                                    }
                                }))
                            .into(), &applicationLayerInfoFold)
                    .into(), &previewFoldGroup
                )
            .into();
        packetHexEditorWindow = 
            Window("原始内容##packetHexEditor")
                .setTitleBar(false)
                .add(
                    Once([this]() {
                        if (packetListTable && packetListTable->getSelected().has_value()) {
                            auto& pkt = capturedPackets[packetListTable->getSelected().value()];
                            if (previewFoldGroup->getSelected() == 1) {
                                // Link Layer
                                if (pkt.linkLayer.isValid()) {
                                    hexEditState.Bytes = (void*)pkt.linkLayer.rawData.data();
                                    hexEditState.MaxBytes = static_cast<int>(pkt.linkLayer.rawData.size());
                                }
                            }
                            else if (previewFoldGroup->getSelected() == 2) {
                                // Network Layer
                                if (pkt.networkLayer.isValid()) {
                                    hexEditState.Bytes = (void*)pkt.networkLayer.rawData.data();
                                    hexEditState.MaxBytes = static_cast<int>(pkt.networkLayer.rawData.size());
                                }
                            }
                            else if (previewFoldGroup->getSelected() == 3) {
                                // Transport Layer
                                if (pkt.transportLayer.isValid()) {
                                    hexEditState.Bytes = (void*)pkt.transportLayer.rawData.data();
                                    hexEditState.MaxBytes = static_cast<int>(pkt.transportLayer.rawData.size());
                                }
                            }
                            else if (previewFoldGroup->getSelected() == 4) {
                                // Application Layer
                                if (pkt.applicationLayer.isValid()) {
                                    hexEditState.Bytes = (void*)pkt.applicationLayer.rawData.data();
                                    hexEditState.MaxBytes = static_cast<int>(pkt.applicationLayer.rawData.size());
                                }
                            }
                            else {
                                hexEditState.Bytes = (void*)pkt.rawData.data();
                                hexEditState.MaxBytes = static_cast<int>(pkt.rawData.size());
                            }
                            
                            if (hexEditState.Bytes != nullptr && hexEditState.MaxBytes != 0) {
                                ImGui::BeginHexEditor("##HexEditor", &hexEditState);
                                ImGui::EndHexEditor();
                            }
                        }
                    })
                )
            .into();
    }

    ~App() {
        if (currentDevice) {
            while (currentDevice->captureActive()) {
                currentDevice->stopCapture();
            }
            while (currentDevice->isOpened()) {
                currentDevice->close();
            }
            currentDevice = nullptr;
        }
    }

    void renderUI() override {
        mainBar.draw();

        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();

        deviceListWindow.setFixedPos(
            mainViewport->GetWorkCenter().x - deviceListWindow.size.value().x / 2, 
            mainViewport->GetWorkCenter().y - deviceListWindow.size.value().y / 2
        );
        deviceListWindow.draw();

        mainWorkSpaceID = ImGui::GetID("MainWorkSpace");

        if (showMainWorkSpace) {
            if (ImGui::DockBuilderGetNode(mainWorkSpaceID) == nullptr) {
                ImGui::DockBuilderAddNode(mainWorkSpaceID, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(mainWorkSpaceID, mainViewport->Size);
                ImGuiID dockIDDown = 0;
                ImGuiID dockIDMain = mainWorkSpaceID;
                ImGui::DockBuilderSplitNode(dockIDMain, ImGuiDir_Down, 0.3f, &dockIDDown, &dockIDMain);
                ImGuiID dockIDDownLeft = 0;
                ImGuiID dockIDDownRight = 0;
                ImGui::DockBuilderSplitNode(dockIDDown, ImGuiDir_Left, 0.6f, &dockIDDownLeft, &dockIDDownRight);
                ImGui::DockBuilderDockWindow("抓取列表##packetList", dockIDMain);
                ImGui::DockBuilderDockWindow("详细信息##packetPreview", dockIDDownLeft);
                ImGui::DockBuilderDockWindow("原始内容##packetHexEditor", dockIDDownRight);
                ImGui::DockBuilderFinish(mainWorkSpaceID);
            }

            ImGui::DockSpaceOverViewport(mainWorkSpaceID, mainViewport, ImGuiDockNodeFlags_PassthruCentralNode);

            if (packetListTable && packetListTable->getSelected().has_value()) {
                previewFoldGroup->setHidden(false);
            }

            {
                std::lock_guard lock(capturedPacketsMutex);
                packetListWindow.draw();
                packetPreviewWindow.draw();
                packetHexEditorWindow.draw();
            }
        }
    }

    static std::string asBinaryStr(uint32_t num, size_t len) {
        std::string result = "";

        for (size_t i = len - 1; i >= 0; i--) {
            result += (num & (1 << i)) ? "1" : "0";
        }

        return result;
    }

    static const char* getICMPDesc(uint8_t type) {
        switch (type) {
            case 0: return "Echo 响应";
            case 1: return "目的不可达";
            case 5: return "重定向报文";
            case 8: return "Echo 请求";
            case 9: return "路由器通知";
            case 10: return "路由器请求";
            case 11: return "ICMP 超时";
            case 12: return "参数问题";
            case 13: return "时间戳请求";
            case 14: return "时间戳应答";
            case 15: return "信息请求报文";
            case 16: return "信息应答报文";
            default: return "未知 ICMP 报文类型 (?)";
        }
    }

    static const char* getICMPv6Desc(uint8_t type) {
        switch (type) {
            case 1: return "目的不可达";
            case 2: return "数据包过大";
            case 3: return "时间超时";
            case 4: return "参数错误";
            case 128: return "Echo 请求";
            case 129: return "Echo 应答";
            case 130: return "组播侦听器查询";
            case 131: return "多播侦听器报告";
            case 132: return "多播侦听器完成";
            case 133: return "路由器请求报文";
            case 134: return "路由器通告报文";
            case 135: return "邻居请求报文";
            case 136: return "邻居通告报文";
            case 137: return "重定向报文";
            case 143: return "组播侦听器报告 (v2)";
            default: return "未知 ICMPv6 报文类型 (?)";
        }
    }

private:
    MainBar mainBar;
    Window deviceListWindow;
    Table<pcpp::PcapLiveDevice*>* deviceListTable = nullptr;

    bool showMainWorkSpace = false;
    ImGuiID mainWorkSpaceID;
    Window packetListWindow;
    Window packetPreviewWindow;
    Window packetHexEditorWindow;
    ImGuiHexEditorState hexEditState {};
    Table<Packet>* packetListTable = nullptr;
    FoldGroup* previewFoldGroup = nullptr;
    Fold* frameInfoFold = nullptr;
    Fold* linkLayerInfoFold = nullptr;
    Fold* networkLayerInfoFold = nullptr;
    Fold* transportLayerInfoFold = nullptr;
    Fold* applicationLayerInfoFold = nullptr;

    pcpp::PcapLiveDevice* currentDevice = nullptr;
    std::vector<pcpp::PcapLiveDevice*> networkDeviceList {};
    
    std::mutex capturedPacketsMutex {};
    std::vector<Packet> capturedPackets {};
};

int main() {
    try {
        App app {};
        app.launch();
        return EXIT_SUCCESS;
    }
    catch (const std::exception& err) {
        std::cout << std::format("[error] {}!\n", err.what());
    }
    return EXIT_FAILURE;
}