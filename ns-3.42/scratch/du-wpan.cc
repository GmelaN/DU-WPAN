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

#define PAN_COUNT 10    // PAN network count
#define NODE_COUNT 20   // node in each PAN count


class PANNetwork: public Object
{
    public:
        static int totalPanId;
        static TypeId GetTypeId(void) {
            static TypeId tid = TypeId("PANNetwork")
                .SetParent<Object>()  // Object 상속 설정
                .SetGroupName("Network")
                .AddConstructor<PANNetwork>();  // 생성자 등록
            return tid;
        }

        PANNetwork(int nodeCount)
        {
            // setup helpers
            this->mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            this->mobility.SetPositionAllocator("ns3::GridPositionAllocator",
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

            this->channel = channel;

            // install mobiilty
            nodes.Create(nodeCount);

            for(int i = 0; i < nodeCount; i++)
            {
                Ptr<Node> node = this->nodes.Get(i);
            }
        }

        PANNetwork()
        {
            // setup helpers
            this->mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            this->mobility.SetPositionAllocator("ns3::GridPositionAllocator",
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

            this->channel = channel;

            // install mobiilty
            nodes.Create(10);
            for(int i = 0; i < 10; i++)
            {
                Ptr<Node> node = this->nodes.Get(i);
            }
        }

        // callback methods
        static void McpsDataConfirmCallback(McpsDataConfirmParams params)
        {
            NS_LOG_UNCOND(Simulator::Now() << "\t" << params.m_status << ": MCPS-DATA confirmed.");
        }

        static void McpsDataIndicationCallback(McpsDataIndicationParams params, Ptr<Packet> packet)
        {
            NS_LOG_UNCOND(Simulator::Now() << "\tfrom " << params.m_srcAddr << "\tto " << params.m_dstAddr << ": MCPS-DATA.indication");
        }

        // getter, setter
        NodeContainer GetNodes()
        {
            return nodes;
        }

        NetDeviceContainer GetDevices() // must used after Install()
        {
            return devices;
        }

        Ptr<Channel> GetChannel()
        {
            return channel;
        }

        void SetChannel(Ptr<SingleModelSpectrumChannel> channel)
        {
            this->channel = channel;
            this->helper.SetChannel(channel);
        }

        // install devices, mobility
        void Install()
        {
            this->mobility.Install(this->nodes);
            this->devices = this->helper.Install(this->nodes);
            this->helper.CreateAssociatedPan(this->devices, totalPanId++);
        }

        void InstallCallbacks()
            {
                for(auto device = this->devices.Begin(); device < this->devices.End(); device++)
                {
                    Ptr<LrWpanNetDevice> dev = DynamicCast<LrWpanNetDevice>((*device));

                    // 각 콜백을 static 메서드로 설정
                    dev->GetMac()->SetMcpsDataConfirmCallback(MakeCallback(&PANNetwork::McpsDataConfirmCallback));
                    dev->GetMac()->SetMcpsDataIndicationCallback(MakeCallback(&PANNetwork::McpsDataIndicationCallback));
                }
            }

    private:
        NodeContainer nodes;
        NetDeviceContainer devices;

        Ptr<SingleModelSpectrumChannel> channel;

        LrWpanHelper helper;
        MobilityHelper mobility;
};

int PANNetwork::totalPanId = 0;

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
    LogComponentEnable("LrWpanMac", LOG_DEBUG);
    LogComponentEnable("SingleModelSpectrumChannel", LOG_DEBUG);

    vector<Ptr<PANNetwork>> panNetworks;

    for(int i = 0; i < PAN_COUNT; i++)
    {
        Ptr<PANNetwork> network = CreateObject<PANNetwork>(NODE_COUNT);
        panNetworks.push_back(network);
    }

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    for(vector<Ptr<PANNetwork>>::iterator panNetwork = panNetworks.begin(); panNetwork < panNetworks.end(); panNetwork++)
    {
        (*panNetwork)->Install();

        // 각 PANNetwork에서의 첫 번째 노드가 해당 PAN의 코디네이터임
        Ptr<LrWpanNetDevice> coordinatorNetDevice = DynamicCast<LrWpanNetDevice>((*panNetwork)->GetDevices().Get(0));
        Mac64Address coordinatorAddr = coordinatorNetDevice->GetMac()->GetExtendedAddress();

        McpsDataRequestParams params;
        params.m_dstExtAddr = coordinatorAddr;
        params.m_dstAddrMode = EXT_ADDR;
        params.m_txOptions = TX_OPTION_NONE;
        params.m_msduHandle = 0;

        for(NetDeviceContainer::Iterator device = ++((*panNetwork)->GetDevices().Begin()); device < (*panNetwork)->GetDevices().End(); device++)
        {
            Ptr<NetDevice> netDevice = *device;
            Ptr<LrWpanNetDevice> lrWpanNetDevice = DynamicCast<LrWpanNetDevice>(netDevice);
            Ptr<Packet> packet = Create<Packet>(50);

            Simulator::ScheduleWithContext(
                0,
                Seconds(5),
                &LrWpanMac::McpsDataRequest,
                lrWpanNetDevice->GetMac(),
                params,
                packet
            );
        }
        (*panNetwork)->InstallCallbacks();
    }

    // Ptr<LrWpanNetDevice> dev = DynamicCast<LrWpanNetDevice>((*panNetworks.at(0)).GetDevices().Get(0));
    // McpsDataRequestParams params;
    // params.m_dstAddr = Mac16Address("00:03");
    // params.m_txOptions = TX_OPTION_NONE;
    // params.m_msduHandle = 0;


    // Simulator::ScheduleWithContext(
    //     0,
    //     Seconds(5),
    //     &LrWpanMac::McpsDataRequest,
    //     dev->GetMac(),
    //     params,
    //     packet
    // );

    Simulator::Schedule(
        Seconds(1),
        &DebugCallback
    );

    Simulator::Stop(Seconds(1500));
    Simulator::Run();


    Simulator::Destroy();

    return 0;
}
