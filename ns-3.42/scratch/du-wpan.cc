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

// using namespace std;
using namespace ns3;
using namespace ns3::lrwpan;

#define PAN_COUNT 100     // PAN network count
#define NODE_COUNT 50   // node in each PAN count

#define SLOT_LENGTH 50 // ms

#define PACKET_SIZE 20  // packet size

int totalRequestedTX = 0;
int totalTriedTX = 0;
int totalSuccessfulRX = 0;

void printResult()
{
    NS_LOG_UNCOND("\n\ntotal Requested TX: "<< totalRequestedTX << "\ttotal Tried TX: " << totalTriedTX << "\ttotal Successful RX: " << totalSuccessfulRX << "\tratio: " << (double) totalSuccessfulRX * 100 / totalTriedTX << "%");
}

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

        PANNetwork()
        {

            Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>();
            random->SetAttribute("Min", DoubleValue(0));
            random->SetAttribute("Max", DoubleValue(5));

            // setup helpers
            this->mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            this->mobility.SetPositionAllocator(
                "ns3::RandomDiscPositionAllocator",
                "X",
                DoubleValue(totalPanId * 20), // x축 시작 좌표
                "Y",
                DoubleValue(totalPanId * 20), // y축 시작 좌표
                "Rho",
                PointerValue(random) // 반경
            );

            Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
            Ptr<LogDistancePropagationLossModel> propModel =
                CreateObject<LogDistancePropagationLossModel>();
            Ptr<ConstantSpeedPropagationDelayModel> delayModel =
                CreateObject<ConstantSpeedPropagationDelayModel>();

            channel->AddPropagationLossModel(propModel);
            channel->SetPropagationDelayModel(delayModel);

            this->channel = channel;

            // install mobiilty
            nodes.Create(NODE_COUNT);

            for(int i = 0; i < NODE_COUNT; i++)
            {
                Ptr<Node> node = this->nodes.Get(i);
            }
            this->networkId = totalPanId++;
        }

        // callback methods
        static void McpsDataConfirmCallback(McpsDataConfirmParams params)
        {
            totalTriedTX++;
            // NS_LOG_UNCOND(Simulator::Now().As(Time::S) << "\t" << params.m_status << ": MCPS-DATA confirmed, data successfully sent.");
        }

        static void McpsDataIndicationCallback(McpsDataIndicationParams params, Ptr<Packet> packet)
        {
            totalSuccessfulRX++;
            // NS_LOG_UNCOND(Simulator::Now().As(Time::S) << "\tdata from " << params.m_srcExtAddr << " successfully received, MCPS-DATA.indication issued.");
        }

        static void BeaconIndicationCallback(MlmeBeaconNotifyIndicationParams params)
        {
            // NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " secs | Received BEACON packet of size ");
        }

        // getter, setter
        NodeContainer GetNodes()
        {
            return this->nodes;
        }

        NetDeviceContainer GetDevices() // must used after Install()
        {
            return this->devices;
        }

        Ptr<Channel> GetChannel()
        {
            return this->channel;
        }

        int GetNetworkId()
        {
            return this->networkId;
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
            this->helper.CreateAssociatedPan(this->devices, this->networkId);
        }

        void InstallCallbacks()
        {
            NS_LOG_UNCOND("Installing callbacks...(ID: " << this->networkId << ")");
            for(uint32_t i = 0; i < this->devices.GetN(); i++) // first device is coordinator
            {
                Ptr<NetDevice> device = this->devices.Get(i);
                Ptr<LrWpanNetDevice> dev = DynamicCast<LrWpanNetDevice>(device);

                // 각 콜백을 static 메서드로 설정
                dev->GetMac()->SetMcpsDataConfirmCallback(MakeCallback(&PANNetwork::McpsDataConfirmCallback));
                dev->GetMac()->SetMcpsDataIndicationCallback(MakeCallback(&PANNetwork::McpsDataIndicationCallback));
                dev->GetMac()->SetMlmeBeaconNotifyIndicationCallback(MakeCallback(&PANNetwork::BeaconIndicationCallback));
            }
        }

        void Start()
        {
            NS_LOG_UNCOND(Simulator::Now().As(Time::S) << "\tScheduling MLME-START.request...(ID: " << this->networkId << ")");

            // 각 네트워크의 가장 첫 번째 디바이스가 코디네이터임
            Ptr<LrWpanNetDevice> coordinatorNetDevice = DynamicCast<LrWpanNetDevice>(*(this->GetDevices().Begin()));
            // NS_LOG_UNCOND("coordinator address is: " << coordinatorNetDevice->GetAddress());

            // MCPS-DATA.request 파라미터
            MlmeStartRequestParams params;
            params.m_PanId = this->networkId;
            params.m_panCoor = true;
            // params.m_bcnOrd = 14;
            // params.m_sfrmOrd = 6;
            params.m_logCh = 11; // 11~26

            Time jitter = MilliSeconds((50 * this->GetNetworkId()));

            Simulator::ScheduleWithContext(
                this->networkId,
                jitter,
                &LrWpanMac::MlmeStartRequest,
                coordinatorNetDevice->GetMac(),
                params
            );
        }

        void SendData()
        {
            // NS_LOG_UNCOND(Simulator::Now().As(Time::S) << "\tScheduling MCPS-DATA.request...(ID: " << this->networkId << ")");

            // 각 네트워크의 가장 첫 번째 디바이스가 코디네이터임
            Ptr<LrWpanNetDevice> coordinatorNetDevice = DynamicCast<LrWpanNetDevice>(*(this->GetDevices().Begin()));
            Mac64Address coordinatorAddr = coordinatorNetDevice->GetMac()->GetExtendedAddress();

            // MCPS-DATA.request 파라미터
            McpsDataRequestParams params;
            params.m_srcAddrMode = EXT_ADDR;
            params.m_dstExtAddr = coordinatorAddr;
            params.m_dstAddrMode = EXT_ADDR;
            params.m_txOptions = TX_OPTION_NONE;
            params.m_msduHandle = 0;

            for(uint32_t i = 1; i < this->GetDevices().GetN(); i++) // first device is coordinator
            {
                Ptr<NetDevice> netDevice = this->GetDevices().Get(i);
                Ptr<LrWpanNetDevice> lrWpanNetDevice = DynamicCast<LrWpanNetDevice>(netDevice);

                // NS_LOG_UNCOND("device address is: " << lrWpanNetDevice->GetAddress());

                Ptr<Packet> packet = Create<Packet>(PACKET_SIZE);

                Time delay = MilliSeconds(SLOT_LENGTH * i);

                Simulator::ScheduleWithContext(
                    this->networkId + i,
                    Simulator::Now() + delay,
                    &LrWpanMac::McpsDataRequest,
                    lrWpanNetDevice->GetMac(),
                    params,
                    packet
                );
                totalRequestedTX++;
            }

            Simulator::Schedule(
                Simulator::Now() + MilliSeconds(SLOT_LENGTH),
                MakeEvent(&PANNetwork::SendData, this)
            );
        }

    private:
        int networkId;
        NodeContainer nodes;
        NetDeviceContainer devices;

        Ptr<SingleModelSpectrumChannel> channel;

        LrWpanHelper helper;
        MobilityHelper mobility;
};

int PANNetwork::totalPanId = 0;

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    LogComponentEnable("LrWpanMac", LOG_ERROR);
    // LogComponentEnable("SingleModelSpectrumChannel", LOG_FUNCTION);
    // LogComponentEnable("LrWpanPhy", LOG_DEBUG);
    // LogComponentEnable("LrWpanNetDevice", LOG_FUNCTION);
    // LogComponentEnable("LrWpanCsmaCa", LOG_DEBUG);

    std::vector<Ptr<PANNetwork>> panNetworks;

    for(int i = 0; i < PAN_COUNT; i++)
    {
        Ptr<PANNetwork> network = CreateObject<PANNetwork>();
        panNetworks.push_back(network);
    }

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    for(std::vector<Ptr<PANNetwork>>::iterator panNetwork = panNetworks.begin(); panNetwork < panNetworks.end(); panNetwork++)
    {
        NS_LOG_UNCOND("Setting up PAN network...(ID: " << (*panNetwork)->GetNetworkId() << ")");
        (*panNetwork)->Install();
        (*panNetwork)->Start();
        (*panNetwork)->InstallCallbacks();

        Simulator::Schedule(
            MilliSeconds((*panNetwork)->GetNetworkId() * SLOT_LENGTH * (NODE_COUNT + 1)),
            MakeEvent(
                &PANNetwork::SendData,
                (*panNetwork)
            )
        );
    }

    Simulator::Schedule(
        Seconds(9.9999),
        MakeEvent(printResult)
    );

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    Simulator::Destroy();


    return 0;
}
