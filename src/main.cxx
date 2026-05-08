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
                            return pkt.dstAddress;
                        })
                        .addColumn("目标地址", [](const Packet& pkt, int) {
                            return pkt.srcAddress;
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
                                                ImGui::Text("协议类型: IPv4");
                                                ImGui::Text("> 服务类型: %d", layer.IPv4Info.typeOfService);
                                                ImGui::Text("> 报文总长: %d", layer.IPv4Info.totalLength);
                                                ImGui::Text("> 标识符: %d", layer.IPv4Info.identification);
                                                ImGui::Text("> TTL: %d", layer.IPv4Info.timeToLive);
                                                ImGui::Text("> 上层协议: %d", layer.IPv4Info.protocol);
                                                ImGui::Text("> 起始 IP: %s", layer.IPv4Info.srcIPAddr.toString().c_str());
                                                ImGui::Text("> 目的 IP: %s", layer.IPv4Info.dstIPAddr.toString().c_str());
                                                break;
                                            }
                                            case NetworkLayer::Type::IPv6: {
                                                ImGui::Text("协议类型: IPv6");
                                                ImGui::Text("> 流类别: %#x", layer.IPv6Info.trafficClass);
                                                ImGui::Text("> 流标签: %#x", layer.IPv6Info.flowLabel);
                                                ImGui::Text("> 有效载荷长度: %d", layer.IPv6Info.payloadLength);
                                                ImGui::Text("> 跳数限制: %d", layer.IPv6Info.hopLimit);
                                                ImGui::Text("> 起始 IP: %s", layer.IPv6Info.srcIPAddr.toString().c_str());
                                                ImGui::Text("> 目的 IP: %s", layer.IPv6Info.dstIPAddr.toString().c_str());
                                                break;
                                            }
                                            case NetworkLayer::Type::ARP:
                                                ImGui::Text("协议类型: ARP");
                                            case NetworkLayer::Type::RARP: {
                                                ImGui::Text("协议类型: RARP");
                                                ImGui::Text("> ");
                                                break;
                                            }
                                            default: break;
                                        }
                                    }
                                }))
                            .into(), &networkLayerInfoFold)
                        .add(
                            Fold("Transport")
                                .add(Once([this]() {
                                    const auto& pkt = capturedPackets[packetListTable->getSelected().value()];
                                    if (pkt.transportLayer.isValid()) {
                                        auto& layer = pkt.transportLayer;
                                        switch (layer.type) {
                                            case TransportLayer::Type::TCP:
                                            case TransportLayer::Type::UDP:
                                            case TransportLayer::Type::ICMP:
                                            default: break;
                                        }
                                    }
                                }))
                            .into(), &transportLayerInfoFold)
                        .add(
                            Fold("Application")
                                .add(Once([this]() {
                                    ImGui::Text("test");
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
                                if (pkt.applicationLayer.isvalid()) {
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