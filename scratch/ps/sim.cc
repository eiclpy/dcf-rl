/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Huazhong University of Science and Technology, Dian Group
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
 * Author: Pengyu Liu <eic_lpy@hust.edu.cn>
 */

#include "ns3/core-module.h"
#include "ns3/config-store-module.h"
#include "ns3/mobility-module.h"
#include "ns3/gnuplot.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;
using namespace std;
#define PI 3.1415926535

std::map<Mac48Address, uint32_t> packetsReceived;
std::map<Mac48Address, uint32_t> bytesReceived;
std::set<uint32_t> associated;

Mac48Address
ContextToMac (std::string context)
{
  uint32_t nodeId, devId;
  sscanf (context.c_str (), "/NodeList/%u/DeviceList/%u", &nodeId, &devId);
  Ptr<WifiNetDevice> d = NodeList::GetNode (nodeId)->GetDevice (devId)->GetObject<WifiNetDevice> ();
  return Mac48Address::ConvertFrom (d->GetAddress ());
}

void
TracePacketReception (std::string context, Ptr<const Packet> packet)
{
  WifiMacHeader hdr;
  packet->PeekHeader (hdr);
  // if (hdr.GetAddr1 () != ContextToMac (context))
  //   return;
  if (packet->GetSize () > 1000)
    bytesReceived[hdr.GetAddr2 ()] += packet->GetSize ();
}

int
main (int argc, char *argv[])
{
  uint32_t ueNum = 32;
  double simTime = 50;

  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time in seconds", simTime);
  cmd.AddValue ("ueNum", "Num of sta", ueNum);
  cmd.Parse (argc, argv);

  NodeContainer apNode, staNode;
  apNode.Create (1);
  staNode.Create (ueNum);

  YansWifiChannelHelper channel;
  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  phy.Set ("TxPowerStart", DoubleValue (16)); // dBm (1.26 mW)
  phy.Set ("TxPowerEnd", DoubleValue (16));
  phy.Set ("Frequency", UintegerValue (5180));

  WifiMacHelper mac;
  WifiHelper wifi;
  Ssid ssid = Ssid ("ns3");

  // ap
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate6Mbps"));
  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, apNode);

  // sta
  mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (phy, mac, staNode);

  NodeContainer c (apNode, staNode);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  positionAlloc->Add (Vector (0, 0, 0.0));

  for (uint32_t i = 0; i < ueNum; ++i)
    {
      double phi = i / ueNum * 2 * PI;
      positionAlloc->Add (Vector (5 * cos (phi), 5 * sin (phi), 0.0));
    }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (c);

  InternetStackHelper stack;
  stack.Install (c);

  for (uint32_t i = 0; i < ueNum; ++i)
    {
      PacketSocketAddress socketAddr;
      socketAddr.SetSingleDevice (staDevice.Get (i)->GetIfIndex ());
      socketAddr.SetPhysicalAddress (apDevice.Get (0)->GetAddress ());
      socketAddr.SetProtocol (1);

      Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
      client->SetRemote (socketAddr);
      staNode.Get (i)->AddApplication (client);
      client->SetAttribute ("PacketSize", UintegerValue (1500));
      client->SetAttribute ("MaxPackets", UintegerValue (0));
      client->SetAttribute ("Interval", TimeValue (MicroSeconds (1000)));
      client->SetStartTime (Seconds (10));

      Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
      server->SetLocal (socketAddr);
      apNode.Get (0)->AddApplication (server);
    }

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxEnd",
                   MakeCallback (&TracePacketReception));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  double throughput = 0.;

  for (auto it = bytesReceived.begin (); it != bytesReceived.end (); it++)
    {
      double duration = simTime - 10;
      double nodeThroughput = (it->second * 8 / duration / 1e6);
      throughput += nodeThroughput;
      std::cout << "Node " << it->first << "; throughput " << nodeThroughput << std::endl;
    }
  std::cout << "Total throughput " << throughput << std::endl;

  Simulator::Destroy ();
  gDcfRl->SetFinish ();
  return 0;
}