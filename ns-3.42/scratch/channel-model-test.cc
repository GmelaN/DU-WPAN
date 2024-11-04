/*  
 *  
 */

#include <ns3/constant-position-mobility-model.h>
#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>

#include <ns3/multi-model-spectrum-channel.h>

#include <ns3/wifi-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/mobility-helper.h>

#include <iostream>
#include <ns3/ble-module.h>



using namespace ns3;
using namespace ns3::lrwpan;

void
BeaconIndication(lrwpan::MlmeBeaconNotifyIndicationParams params)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " secs | Received BEACON packet of size ");
}

void
DataIndication(lrwpan::McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << " secs | Received DATA packet of size " << p->GetSize());
    p->Print(std::cout);
}

void
TransEndIndication(lrwpan::McpsDataConfirmParams params)
{
    // In the case of transmissions with the Ack flag activated, the transaction is only
    // successful if the Ack was received.
    if(params.m_status == MacStatus::SUCCESS)
    {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " Transmission successfully sent");
    }
}

void
StartConfirm(lrwpan::MlmeStartConfirmParams params)
{
    if(params.m_status == MacStatus::SUCCESS)
    {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " Beacon status SUCCESSFUL");
    }
}

std::vector<NodeContainer>
setupWifi(Ptr<SpectrumChannel> channel, double startTime, double stopTime, double interval, int packetSize) {
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(1);
    NodeContainer wifiApNode;
    wifiApNode.Create(20);

    SpectrumWifiPhyHelper phy = SpectrumWifiPhyHelper(1);
    FrequencyRange r;
    r.minFrequency = 2400;
    r.maxFrequency = 2420;
    phy.AddPhyToFreqRangeMapping(1, r);
    phy.SetChannel(channel);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(-5.0),
                                    "MinY", DoubleValue(-5.0),
                                    "DeltaX", DoubleValue(5.0),
                                    "DeltaY", DoubleValue(5.0),
                                    "GridWidth", UintegerValue(2),
                                    "LayoutType", StringValue("RowFirst"));     
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevices);

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApp = echoServer.Install(wifiApNode.Get(0)); 
    serverApp.Start(Seconds(startTime));
    serverApp.Stop(Seconds(stopTime));

    UdpEchoClientHelper echoClient(apInterface.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(0));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(interval)));
    echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

    for(uint32_t i = 1; i < wifiStaNodes.GetN(); i++) {     
        ApplicationContainer clientApp = echoClient.Install(wifiStaNodes.Get(i));
        clientApp.Start(Seconds(startTime));
        clientApp.Stop(Seconds(stopTime));
    }
    std::vector<NodeContainer> result;
    result.push_back(wifiApNode);
    result.push_back(wifiStaNodes);

    return result;
}


void setupBLE(Ptr<SpectrumChannel> channel, double startTime, double duration, double interval, int packetSize) {
    int nNodes = 2;

    BleHelper helper;
    helper.SetChannel(channel);
    NodeContainer bleNodes;
    bleNodes.Create(nNodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                    "X", DoubleValue(0.0),
                                    "Y", DoubleValue(0.0));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(bleNodes);

    Ptr<UniformRandomVariable> randT = CreateObject<UniformRandomVariable>();
    int length = 10;

    Ptr<ListPositionAllocator> nodePositionList = 
      CreateObject<ListPositionAllocator>();
    for (int nodePositionsAssigned = 0; 
        nodePositionsAssigned < nNodes; nodePositionsAssigned++)
    {
      double x, y, z;
      x = randT->GetInteger(0,length);
      y = randT->GetInteger(0,length);
      z = randT->GetInteger(0,length);
      nodePositionList->Add (Vector (x, y, z));
    }

    mobility.SetPositionAllocator (nodePositionList);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install(bleNodes);

    NetDeviceContainer bleNetDevices;
    bleNetDevices = helper.Install(bleNodes);

    for (int nodeI = 0; nodeI < nNodes; nodeI++)
    {
        std::stringstream stream;
        stream << std::hex << nodeI+1;
        std::string s( stream.str());
        while (s.size() < 4)
            s.insert(0,1,'0');
        s.insert(2,1,':');
        char const * buffer = s.c_str();
        DynamicCast<BleNetDevice>(
            bleNetDevices.Get(nodeI))->SetAddress (Mac16Address (buffer)
        );
    }

    bool scheduled = true;
    uint32_t nbConnInterval = 3200; // connection interval, max 3200

    helper.CreateAllLinks(bleNetDevices, scheduled, nbConnInterval);

    ApplicationContainer apps = helper.GenerateTraffic (
        randT, bleNodes, packetSize, startTime, duration, interval
    );
}


int
main(int argc, char* argv[])
{    
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    LogComponentEnable("LrWpanMac", LOG_LEVEL_DEBUG);
    LogComponentEnable("LrWpanPhy", LOG_LOGIC);
    // LogComponentEnable("SpectrumWifiPhy", LOG_INFO);
    LogComponentEnable("MultiModelSpectrumChannel", LOG_LOGIC);
    // LogComponentEnable("LrWpanNetDevice", LOG_FUNCTION);
    // LogComponentEnable("LrWpanCsmaCa", LOG_LOGIC);
    // LogComponentEnable("BlePhy", LOG_LEVEL_INFO);
    LogComponentEnable("BleLinkController", LOG_INFO);
    // LogComponentEnable("BleNetDevice", LOG_ALL);
    PacketMetadata::Enable();

    Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel>();
    channel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->SetPropagationDelayModel(delayModel);

    // setupWifi(channel, 0.0, 5.0, 0.5, 2048);
    setupBLE(channel, 0.0, 100.0, 0.001, 80);

    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<Node> n2 = CreateObject<Node>();
    Ptr<Node> n3 = CreateObject<Node>();

    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev3 = CreateObject<LrWpanNetDevice>();

    dev0->SetAddress(Mac16Address("00:01"));
    dev1->SetAddress(Mac16Address("00:02"));
    dev2->SetAddress(Mac16Address("00:03"));
    dev3->SetAddress(Mac16Address("00:04"));

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);
    dev2->SetChannel(channel);
    dev3->SetChannel(channel);

    n0->AddDevice(dev0);
    n1->AddDevice(dev1);
    n2->AddDevice(dev2);
    n3->AddDevice(dev3);

    Ptr<ConstantPositionMobilityModel> sender0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);

    Ptr<ConstantPositionMobilityModel> sender1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender1Mobility->SetPosition(Vector(0, 10, 0)); // 10 m distance
    dev1->GetPhy()->SetMobility(sender1Mobility);

    Ptr<ConstantPositionMobilityModel> sender2Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender2Mobility->SetPosition(Vector(0, -10, 0)); // 10 m distance
    dev2->GetPhy()->SetMobility(sender2Mobility);

    Ptr<ConstantPositionMobilityModel> sender3Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender3Mobility->SetPosition(Vector(0, -1, 0)); // 10 m distance
    dev3->GetPhy()->SetMobility(sender3Mobility);

    /////// MAC layer Callbacks hooks/////////////
    lrwpan::MlmeStartConfirmCallback cb0;
    cb0 = MakeCallback(&StartConfirm);
    dev0->GetMac()->SetMlmeStartConfirmCallback(cb0);
    dev1->GetMac()->SetMlmeStartConfirmCallback(cb0);
    dev2->GetMac()->SetMlmeStartConfirmCallback(cb0);
    dev3->GetMac()->SetMlmeStartConfirmCallback(cb0);

    lrwpan::McpsDataConfirmCallback cb1;
    cb1 = MakeCallback(&TransEndIndication);
    dev0->GetMac()->SetMcpsDataConfirmCallback(cb1);
    dev1->GetMac()->SetMcpsDataConfirmCallback(cb1);
    dev2->GetMac()->SetMcpsDataConfirmCallback(cb1);
    dev3->GetMac()->SetMcpsDataConfirmCallback(cb1);

    lrwpan::MlmeBeaconNotifyIndicationCallback cb3;
    cb3 = MakeCallback(&BeaconIndication);
    dev0->GetMac()->SetMlmeBeaconNotifyIndicationCallback(cb3);
    dev1->GetMac()->SetMlmeBeaconNotifyIndicationCallback(cb3);
    dev2->GetMac()->SetMlmeBeaconNotifyIndicationCallback(cb3);
    dev3->GetMac()->SetMlmeBeaconNotifyIndicationCallback(cb3);

    lrwpan::McpsDataIndicationCallback cb4;
    cb4 = MakeCallback(&DataIndication);
    dev0->GetMac()->SetMcpsDataIndicationCallback(cb4);
    dev1->GetMac()->SetMcpsDataIndicationCallback(cb4);
    dev2->GetMac()->SetMcpsDataIndicationCallback(cb4);
    dev3->GetMac()->SetMcpsDataIndicationCallback(cb4);

    dev0->GetMac()->SetPanId(5);
    dev1->GetMac()->SetPanId(5);
    dev2->GetMac()->SetPanId(6);
    dev3->GetMac()->SetPanId(6);

    dev1->GetMac()->SetAssociatedCoor(Mac16Address("00:01"));
    dev3->GetMac()->SetAssociatedCoor(Mac16Address("00:04"));

    MlmeStartRequestParams params;
    params.m_panCoor = true;
    params.m_PanId = 5;
    params.m_bcnOrd = 15;
    params.m_sfrmOrd = 15;
    params.m_logCh = 11;
    Simulator::ScheduleWithContext(1,
                                   Seconds(0.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   dev0->GetMac(),
                                   params);

    MlmeStartRequestParams params1;
    params1.m_panCoor = false;
    params1.m_logCh = 11;
    Simulator::ScheduleWithContext(2,
                                   Seconds(0.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   dev1->GetMac(),
                                   params1);

    // MlmeStartRequestParams params2;
    // params2.m_panCoor = true;
    // params2.m_PanId = 6;
    // params2.m_bcnOrd = 15;
    // params2.m_sfrmOrd = 15;
    // params2.m_logCh = 26;
    // Simulator::ScheduleWithContext(1,
    //                                Seconds(0.0),
    //                                &LrWpanMac::MlmeStartRequest,
    //                                dev2->GetMac(),
    //                                params2);
    
    // MlmeStartRequestParams params3;
    // params3.m_panCoor = false;
    // params3.m_logCh = 26;
    // Simulator::ScheduleWithContext(1,
    //                                Seconds(0.0),
    //                                &LrWpanMac::MlmeStartRequest,
    //                                dev3->GetMac(),
    //                                params3);

    ///////////////////// Transmission of data Packets from end device //////////////////////

    lrwpan::McpsDataRequestParams params4;
    params4.m_dstPanId = 5;
    params4.m_srcAddrMode = lrwpan::SHORT_ADDR;
    params4.m_dstAddrMode = lrwpan::SHORT_ADDR;
    params4.m_dstAddr = Mac16Address("00:01");
    params4.m_msduHandle = 0;
    params4.m_txOptions = lrwpan::TX_OPTION_NONE;

    for(int i = 0; i < 20; i++) {
        Ptr<Packet> packet = Create<Packet>(50);
        packet->EnablePrinting();

        Simulator::ScheduleWithContext(1,
                                        Seconds(0.3 + i * 0.1),
                                        &LrWpanMac::McpsDataRequest,
                                        dev1->GetMac(),
                                        params4,
                                        packet);
    }

    // lrwpan::McpsDataRequestParams params5;
    // params5.m_dstPanId = 6;
    // params5.m_srcAddrMode = lrwpan::SHORT_ADDR;
    // params5.m_dstAddrMode = lrwpan::SHORT_ADDR;
    // params5.m_dstAddr = Mac16Address("00:03");
    // params5.m_msduHandle = 0;
    // params5.m_txOptions = lrwpan::TX_OPTION_NONE;

    // for(int i = 0; i < 20; i++) {
    //     Ptr<Packet> packet = Create<Packet>(100);
    //     packet->EnablePrinting();

    //     Simulator::ScheduleWithContext(1,
    //                                     Seconds(0.3 + i * 0.1),
    //                                     &LrWpanMac::McpsDataRequest,
    //                                     dev3->GetMac(),
    //                                     params5,
    //                                     packet);
    // }


    Simulator::Stop(Seconds(3));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
