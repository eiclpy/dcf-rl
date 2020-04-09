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

void
PrintFlowMonitorStats (Ptr<FlowMonitor> monitor, FlowMonitorHelper &flowmonHelper, double duration)
{
  // Print per-flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier =
      DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  double throughput = 0.0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin ();
       i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::stringstream protoStream;
      protoStream.str ("UDP");
      std::cout << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> "
                << t.destinationAddress << ":" << t.destinationPort << ") proto "
                << protoStream.str () << "\n";
      std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
      std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
      std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / duration / 1000 / 1000
                << " Mbps\n";
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      if (i->second.rxPackets > 0)
        {
          // Measure the duration of the flow from receiver's perspective
          double rxDuration =
              i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ();
          std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000
                    << " Mbps\n";
          std::cout << "  Mean delay:  "
                    << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets << " ms\n";
          std::cout << "  Mean jitter:  "
                    << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets << " ms\n";
          throughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
        }
      else
        {
          std::cout << "  Throughput:  0 Mbps\n";
          std::cout << "  Mean delay:  0 ms\n";
          std::cout << "  Mean jitter: 0 ms\n";
        }
      std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
    }
  std::cout << "Total throughput: " << throughput << " Mbps\n";
}

int
main (int argc, char *argv[])
{
  uint32_t ueNum = 32;
  double simTime = 5;

  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time in seconds", simTime);
  cmd.AddValue ("ueNum", "Num of sta", ueNum);
  cmd.Parse (argc, argv);

  NodeContainer apNode, staNode;
  apNode.Create (1);
  staNode.Create (ueNum);

  YansWifiChannelHelper channel;
  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  // "Frequency", DoubleValue (5.180e9));
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
  // c.Add (apNode);
  // c.Add (staNode);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // Set postion for AP
  positionAlloc->Add (Vector (0, 0, 0.0));

  // Set postion for STAs
  for (uint32_t i = 0; i < ueNum; ++i)
    {
      double phi = i / ueNum * 2 * PI;
      positionAlloc->Add (Vector (5 * cos (phi), 5 * sin (phi), 0.0));
    }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (c);

  InternetStackHelper stack;
  stack.Install (apNode);
  stack.Install (staNode);

  Ipv4AddressHelper address;
  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apNodeInterface;
  apNodeInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staNodeInterface;
  staNodeInterface = address.Assign (staDevice);

  UdpServerHelper apServer (9);
  ApplicationContainer apServerApp = apServer.Install (apNode.Get (0));
  apServerApp.Start (Seconds (0.0));
  apServerApp.Stop (Seconds (simTime));

  UdpServerHelper *staServer = new UdpServerHelper[ueNum];
  ApplicationContainer *staServerApp = new ApplicationContainer[ueNum];
  for (uint32_t i = 0; i < ueNum; ++i)
    {
      staServer[i] = UdpServerHelper (5000 + i);
      staServerApp[i] = staServer[i].Install (staNode.Get (i));
      staServerApp[i].Start (Seconds (0.0));
      staServerApp[i].Stop (Seconds (simTime));
    }

  UdpClientHelper *apClient = new UdpClientHelper[ueNum];
  ApplicationContainer *apClientApp = new ApplicationContainer[ueNum];

  // for (uint32_t i = 0; i < ueNum; ++i)
  //   {
  //     apClient[i] = UdpClientHelper (staNodeInterface.GetAddress (i), 5000 + i);
  //     apClient[i].SetAttribute ("MaxPackets", UintegerValue (429496729u));
  //     apClient[i].SetAttribute ("Interval", TimeValue (Time ("0.0001"))); //packets/s
  //     apClient[i].SetAttribute ("PacketSize", UintegerValue (1500)); //bytes
  //     apClientApp[i] = apClient[i].Install (apNode);
  //     apClientApp[i].Start (Seconds (0.5));
  //     apClientApp[i].Stop (Seconds (simTime));
  //   }
  UdpClientHelper staClient (apNodeInterface.GetAddress (0), 9);
  staClient.SetAttribute ("MaxPackets", UintegerValue (429496725u));
  staClient.SetAttribute ("Interval", TimeValue (Time("0.0001"))); //packets/s
  staClient.SetAttribute ("PacketSize", UintegerValue (1500)); //bytes
  ApplicationContainer staClientApp;
  staClientApp.Add (staClient.Install (staNode));
  staClientApp.Start (Seconds (0.5));
  staClientApp.Stop (Seconds (simTime));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> monitor = flowmonHelper.Install (c);
  monitor->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  PrintFlowMonitorStats (monitor, flowmonHelper, simTime);

  delete[] staServer;
  delete[] staServerApp;
  delete[] apClient;
  delete[] apClientApp;

  Simulator::Destroy ();
  gDcfRl->SetFinish();
  return 0;
}