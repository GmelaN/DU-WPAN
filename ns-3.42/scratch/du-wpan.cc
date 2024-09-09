/*
 * Copyright (c) 2024 Gyeongsang National University, South Korea.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Jo Seoung Hyeon <gmelan@gnu.ac.kr>
 */

/*
 * This example demonstrates the use of DU-WPAN.
 * 
 */

#include <ns3/core-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/propagation-module.h>
#include <ns3/spectrum-module.h>

#include <vector>
#include <iostream>

using namespace std;
using namespace ns3;
using namespace ns3::lrwpan;

#define PAN_COUNT 10 // PAN network count
#define NODE_COUNT 20 // node in each PAN count

class PANNetwork
{
    public:
    PANNetwork(int nodeCount)
    {
        nodes.Create(nodeCount);
        for(int i = 0; i < nodeCount; i++)
        {
            Ptr<Node> node = this->nodes.Get(i);
            Ptr<LrWpanNetDevice> device = CreateObject<LrWpanNetDevice>();
            node->AddDevice(device);
            devices.Add(device);
        }
    }

    NetDeviceContainer GetDevices()
    {
        return devices;
    }

    NodeContainer GetNodes()
    {
        return nodes;
    }

    // SingleModelSpectrumChannel GetChannel()
    // {
    //     return channel;
    // }

    // void SetChannel(SingleModelSpectrumChannel channel)
    // {
    //     this->channel = channel;
    // }

    private:
        NetDeviceContainer devices;
        NodeContainer nodes;
        // SingleModelSpectrumChannel channel;
};

static void
McpsDataConfirm(Ptr<LrWpanNetDevice> device, McpsDataConfirmParams params)
{
    NS_LOG_UNCOND(Simulator::Now() << "\t" << device->GetAddress() << " sent data");
}

static void
McpsDataIndication(Ptr<LrWpanNetDevice> device, McpsDataIndicationParams params, Ptr<Packet> packet)
{
    NS_LOG_UNCOND(Simulator::Now() << "\t" << device->GetAddress() << " got data, packet size: " << packet->GetSize());
}

static void
DebugCallback()
{
    int i = 0;
    i++;
    NS_LOG_UNCOND("ddd" << i);
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));

    vector<PANNetwork> panNetworks;

    for(int i = 0; i < PAN_COUNT; i++)
    {
        panNetworks.push_back(PANNetwork(NODE_COUNT));
    }

    MobilityHelper mobility;
    LrWpanHelper lrWpanHelper;

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(30.0),
                                  "DeltaY",
                                  DoubleValue(30.0),
                                  "GridWidth",
                                  UintegerValue(20),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    lrWpanHelper.SetChannel(channel);
    
    int i = 0;
    for(vector<PANNetwork>::iterator device = panNetworks.begin(); device < panNetworks.end(); device++)
    {
        mobility.Install(device->GetNodes());
        lrWpanHelper.CreateAssociatedPan(device->GetDevices(), ++i);
    }

    lrWpanHelper.EnableLogComponents();

    // Ptr<LrWpanNetDevice> dev = DynamicCast<LrWpanNetDevice>(panNetworks.at(0).GetDevices().Get(0));
    
    // McpsDataRequestParams params;
    // params.m_dstAddr = Mac16Address("00:03");
    // params.m_txOptions = TX_OPTION_NONE;
    // params.m_msduHandle = 0;

    // Ptr<Packet> packet = Create<Packet>(50);

    // Simulator::ScheduleWithContext(
    //     0,
    //     Seconds(5),
    //     &LrWpanMac::McpsDataRequest,
    //     dev->GetMac(),
    //     params,
    //     packet
    // );

    Simulator::Stop(Seconds(1500));
    Simulator::Run();

    // Callback cb = MakeCallback(&DebugCallback);

    Simulator::Schedule(
        Seconds(1),
        &DebugCallback
    );

    Simulator::Destroy();

    return 0;
}
