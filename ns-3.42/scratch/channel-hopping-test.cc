/*
 *   Coordinator              End Device
 *       N0   <----------------  N1
 *      (dev0)                 (dev1)
 *
 * This example demonstrate the usage of the MAC primitives involved in
 * direct transmissions for the beacon enabled mode of IEEE 802.15.4-2011.
 * A single packet is sent from an end device to the coordinator during the CAP
 * of the first incoming superframe.
 *
 * This example do not demonstrate a full protocol stack usage.
 * For full protocol stack usage refer to 6lowpan examples.
 */

#include <ns3/constant-position-mobility-model.h>
#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/callback.h>

#include <iostream>

#define CHANNEL_HOPPING

using namespace ns3;
using namespace ns3::lrwpan;

void
BeaconIndication(MlmeBeaconNotifyIndicationParams params)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " secs | Received BEACON packet of size ");
}

void
DataIndication(McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << " secs | Received DATA packet of size " << p->GetSize());
}

void
TransEndIndication(McpsDataConfirmParams params)
{
    // In the case of transmissions with the Ack flag activated, the transaction is only
    // successful if the Ack was received.
    if (params.m_status == MacStatus::SUCCESS)
    {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " Transmission successfully sent");
    }
}

void
DataIndicationCoordinator(McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s Coordinator Received DATA packet (size " << p->GetSize() << " bytes)");
}

void
StartConfirm(MlmeStartConfirmParams params)
{
    if (params.m_status == MacStatus::SUCCESS)
    {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " Beacon status SUCCESSFUL");
    }
}


#ifdef CHANNEL_HOPPING
void
changeChannel(Ptr<Node> node, Ptr<SpectrumChannel> targetChannel)
{
    Ptr<LrWpanNetDevice> netDevice = DynamicCast<LrWpanNetDevice>(node->GetDevice(0));
    netDevice->SetChannel(targetChannel);
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " device " << node->GetId() << "'s channel has been changed to: "<< targetChannel->GetId());

    return;
}
#endif

#define NODE_COUNT 10
#define CHANNEL_COUNT 3

typedef struct PANNetworkStruct
{
    Ptr<Node> coordinator;
    Ptr<Node> nodes[NODE_COUNT];
    Ptr<SingleModelSpectrumChannel> channels[CHANNEL_COUNT];
} PANNetwork;

PANNetwork* setup()
{
    PANNetwork* network;

    // setup channels
    for(int i = 0; i < CHANNEL_COUNT; i++)
    {
        network->channels[i] = CreateObject<SingleModelSpectrumChannel>();
        network->channels[i]->AddPropagationLossModel(CreateObject<LogDistancePropagationLossModel>());
        network->channels[i]->SetPropagationDelayModel(CreateObject<PropagationDelayModel>());
    }

    Ptr<LrWpanNetDevice> coordinatorNetDevice, nodeNetDevice;

    // setup coordinator
    network->coordinator = CreateObject<Node>();
    coordinatorNetDevice = CreateObject<LrWpanNetDevice>();

    coordinatorNetDevice->SetChannel(network->channels[0]);
    coordinatorNetDevice->SetAddress(Mac16Address("00:01"));

    Ptr<ConstantPositionMobilityModel> mobility =
        CreateObject<ConstantPositionMobilityModel>();
    mobility->SetPosition(Vector(0,0,0));
    coordinatorNetDevice->GetPhy()->SetMobility(mobility);

    network->coordinator->AddDevice(coordinatorNetDevice);

    // setup nodes
    for(int i = 0; i < NODE_COUNT; i++)
    {
        network->nodes[i] = CreateObject<Node>();
        nodeNetDevice = CreateObject<LrWpanNetDevice>();
        nodeNetDevice->SetAddress(Mac16Address((uint16_t) i + 2));
        nodeNetDevice->SetChannel(network->channels[0]);
        network->nodes[i]->AddDevice(nodeNetDevice);
    }

    return network;
}

void
hookMcpsDataConfirmCallbacks(PANNetwork* network, McpsDataConfirmCallback cb,  void *(callback)(McpsDataConfirmParams))
{
    Ptr<LrWpanNetDevice> netDevice = DynamicCast<LrWpanNetDevice>(network->coordinator->GetDevice(0));
    netDevice->GetMac()->SetMcpsDataConfirmCallback(callback);

    return;
}

void
hookMlmeStartConfirmCallbacks(PANNetwork* network, MlmeStartConfirmCallback cb,  void *(callback)(MlmeStartConfirmParams))
{
    Ptr<LrWpanNetDevice> netDevice = DynamicCast<LrWpanNetDevice>(network->coordinator->GetDevice(0));
    netDevice->GetMac()->SetMlmeStartConfirmCallback(callback);

    return;
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_FUNC);
    LogComponentEnable("LrWpanMac", LOG_LEVEL_INFO);
    LogComponentEnable("LrWpanCsmaCa", LOG_LEVEL_INFO);

    /////// MAC layer Callbacks hooks/////////////

    // McpsDataConfirmCallback cb1;
    // cb1 = MakeCallback(&TransEndIndication);
    // dev1->GetMac()->SetMcpsDataConfirmCallback(cb1);

    // MlmeBeaconNotifyIndicationCallback cb3;
    // cb3 = MakeCallback(&BeaconIndication);
    // dev1->GetMac()->SetMlmeBeaconNotifyIndicationCallback(cb3);

    // McpsDataIndicationCallback cb4;
    // cb4 = MakeCallback(&DataIndication);
    // dev1->GetMac()->SetMcpsDataIndicationCallback(cb4);

    // McpsDataIndicationCallback cb5;
    // cb5 = MakeCallback(&DataIndicationCoordinator);
    // dev0->GetMac()->SetMcpsDataIndicationCallback(cb5);

    //////////// Manual device association ////////////////////
    // Note: We manually associate the devices to a PAN coordinator
    //       (i.e. bootstrap is not used);
    //       The PAN COORDINATOR does not need to associate or set its
    //       PAN Id or its own coordinator id, these are set
    //       by the MLME-start.request primitive when used.

    dev1->GetMac()->SetPanId(5);
    dev1->GetMac()->SetAssociatedCoor(Mac16Address("00:01"));

    ///////////////////// Start transmitting beacons from coordinator ////////////////////////

    MlmeStartRequestParams params;
    params.m_panCoor = true;
    params.m_PanId = 5;
    params.m_bcnOrd = 14;
    params.m_sfrmOrd = 6;
    Simulator::ScheduleWithContext(1,
                                   Seconds(2.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   dev0->GetMac(),
                                   params);

    ///////////////////// Transmission of data Packets from end device //////////////////////

    Ptr<Packet> p1 = Create<Packet>(5);
    McpsDataRequestParams params2;
    params2.m_dstPanId = 5;
    params2.m_srcAddrMode = SHORT_ADDR;
    params2.m_dstAddrMode = SHORT_ADDR;
    params2.m_dstAddr = Mac16Address("00:01");
    params2.m_msduHandle = 0;
    params2.m_txOptions = TX_OPTION_NONE;

    Simulator::ScheduleWithContext(1,
                                   Seconds(2.5),
                                   &LrWpanMac::McpsDataRequest,
                                   dev1->GetMac(),
                                   params2,
                                   p1);
    
    #ifdef CHANNEL_HOPPING
    Ptr<SingleModelSpectrumChannel> anotherChannel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> anotherPropModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> anotherDelayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    anotherChannel->AddPropagationLossModel(anotherPropModel);
    anotherChannel->SetPropagationDelayModel(anotherDelayModel);

    Simulator::ScheduleWithContext(
        2,
        Seconds(3.1),
        &changeChannel,
        n0,
        anotherChannel
    );
    #endif

    Simulator::ScheduleWithContext(3,
                                Seconds(4),
                                &LrWpanMac::McpsDataRequest,
                                dev1->GetMac(),
                                params2,
                                p1);

    #ifdef CHANNEL_HOPPING
    Simulator::ScheduleWithContext(
        4,
        Seconds(4.1),
        &changeChannel,
        n0,
        channel
    );
    #endif

    Simulator::ScheduleWithContext(5,
                            Seconds(5),
                            &LrWpanMac::McpsDataRequest,
                            dev1->GetMac(),
                            params2,
                            p1);

    Simulator::Stop(Seconds(600));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
