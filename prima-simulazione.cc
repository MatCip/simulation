/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Gaurav Sathe <gaurav.sathe@tcs.com>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <iostream>
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates one eNodeB,
 * attaches three UE to eNodeB starts a flow for each UE to  and from a remote host.
 * It also instantiates one dedicated bearer per UE
 */
NS_LOG_COMPONENT_DEFINE ("Prima_simulazione");
int
main (int argc, char *argv[])
{

  uint16_t numberOfNodes = 1; //numero di enB
  uint16_t numberOfUeNodes = 12;  //DEVE ESSERE DIVISIBILE PER 4
  double simTime = 1.1;
  double distance = 60.0;   //distanza tra eNB
  double interPacketInterval = 100;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue ("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse (argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Enable Logging
  //LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

  LogComponentEnable ("Prima_simulazione", LOG_LEVEL_ALL);
  //LogComponentEnable ("LteHelper", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("EpcHelper", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("EpcEnbApplication", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("EpcSgwPgwApplication", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("EpcMme", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_LOGIC);


  // Create a single RemoteHost: servizio connesso ad internet a cui collegarsi
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet, non da modificare perchè riferiti all'internet "fisso"
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodesH2H;
  NodeContainer ueNodesM2MMobile;
  NodeContainer ueNodesM2MFixed;
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (numberOfNodes);
  ueNodesH2H.Create(numberOfUeNodes/2);
  ueNodesM2MMobile.Create(numberOfUeNodes/4);
  ueNodesM2MFixed.Create(numberOfUeNodes/4);

  // Install Mobility Model
  //Crea le antenne in riga distaziate da distance=60 m
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector (distance * i, 0, 0));  //vettore delle posizione
    }
  MobilityHelper mobilityeNB;
  mobilityeNB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNB.SetPositionAllocator (positionAlloc);
  mobilityeNB.Install (enbNodes);

  //mobilità per i M2M fissi, disposti casualmente
  MobilityHelper mobilityM2MFixed;
  mobilityM2MFixed.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityM2MFixed.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobilityM2MFixed.Install(ueNodesM2MFixed);

  //mobilità per gli H2H dinamici con random walk e posizione iniziale casuale
  MobilityHelper mobilityH2H;
  mobilityH2H.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobilityH2H.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|200|0|200"));
  mobilityH2H.Install (ueNodesH2H);

  //mobilità per gli M2M dinamici con random walk e posizione iniziale causale
  MobilityHelper mobilityM2MMobile;
  mobilityM2MMobile.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobilityM2MMobile.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|200|0|200"));
  mobilityM2MMobile.Install (ueNodesM2MMobile);

  //aggiungo i vari tipi di nodi ue ad ueNodes per compatibilità successiva
  ueNodes.Add(ueNodesH2H);ueNodes.Add(ueNodesM2MMobile); ueNodes.Add(ueNodesM2MFixed);
  
  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  
  NS_LOG_UNCOND("Il numero di dispositivi totali connessi all'inizio è: " 
                << ueLteDevs.GetN() << "\n"
                << "Di cui: \nDispositivi H2H: " << ueNodesH2H.GetN()
                << "\nDispositivi M2M mobili: " << ueNodesM2MMobile.GetN()
                << "\nDispositivi M2M fissi: " << ueNodesM2MFixed.GetN() << "\n\n");

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach a UE to a eNB
  lteHelper->Attach (ueLteDevs, enbLteDevs.Get (0));

  // Activate an EPS bearer on all UEs
  // qui dipende tutto dal tipo di servizio
  // per comunicazioni H2H metto tipologia conversazione vocale
  for (uint32_t u = 0; u < ueNodes.GetN()-ueNodesM2MFixed.GetN()-ueNodesM2MMobile.GetN(); ++u)
    {
      //in questo caso il bit rate garantito è anche quello massimo: bit rate costante
      Ptr<NetDevice> ueDevice = ueLteDevs.Get (u);
      GbrQosInformation qos;  //Guaranteed Bit Rate: vedi 3GPP TS 36.143 9.2.1.18 GBR QoS Information
      qos.gbrDl = 132;  // bit/s, considering IP, UDP, RLC, PDCP header size
      qos.gbrUl = 132;
      qos.mbrDl = qos.gbrDl;   //massimo bit rate in download
      qos.mbrUl = qos.gbrUl;   //massimo bit rate in upload

      enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;   //qci=Qos Class indication
      EpsBearer bearer (q, qos);
      bearer.arp.priorityLevel = 15-u; //va da 1 a 15; 1 è il piu alto (15 - (u + 1);)
                                       //quindi ho una priorità crescente per ogni conversazione vocale 
      bearer.arp.preemptionCapability = true;
      bearer.arp.preemptionVulnerability = true;
      lteHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, EpcTft::Default ());
    }

  // per comunicazioni M2M metto tipologia conversazione vocale IN ATTESA DI ALTRE TIPOLOGIE
  for (uint32_t u = ueNodes.GetN()-ueNodesM2MFixed.GetN()-ueNodesM2MMobile.GetN() ; u < ueNodes.GetN(); ++u)
    {
      Ptr<NetDevice> ueDevice = ueLteDevs.Get (u);
      GbrQosInformation qos;  //Guaranteed Bit Rate: vedi 3GPP TS 36.143 9.2.1.18 GBR QoS Information
      qos.gbrDl = 132;  // bit/s, considering IP, UDP, RLC, PDCP header size
      qos.gbrUl = 132;
      qos.mbrDl = qos.gbrDl;   //massimo bit rate in download
      qos.mbrUl = qos.gbrUl;   //massimo bit rate in upload

      enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;   //qci=Qos Class indication
      EpsBearer bearer (q, qos);
      bearer.arp.priorityLevel = 15-u; //va da 1 a 15; 1 è il piu alto (15 - (u + 1);)
                                       //quindi ho una priorità crescente per ogni comunicazione;
                                       //inoltre la priorità è maggiore rispetto a quella per conversazioni vocali
      bearer.arp.preemptionCapability = true;
      bearer.arp.preemptionVulnerability = true;
      lteHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, EpcTft::Default ());
    }
    
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      serverApps.Add (packetSinkHelper.Install (ueNodes.Get (u)));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds (interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue (1000000));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds (interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));

      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
      client.SetAttribute ("Interval", TimeValue (MilliSeconds (interPacketInterval)));
      client.SetAttribute ("MaxPackets", UintegerValue (1000000));

      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get (u)));
      if (u + 1 < ueNodes.GetN ())
        {
          clientApps.Add (client.Install (ueNodes.Get (u + 1)));
        }
      else
        {
          clientApps.Add (client.Install (ueNodes.Get (0)));
        }
    }

  serverApps.Start (Seconds (0.030));
  clientApps.Start (Seconds (0.030));

  double statsStartTime = 0.04; // need to allow for RRC connection establishment + SRS
  double statsDuration = 1.0;

  lteHelper->EnableRlcTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("StartTime", TimeValue (Seconds (statsStartTime)));
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (statsDuration)));

  //get ue device pointer for UE-ID 0 IMSI 1 and enb device pointer
  Ptr<NetDevice> ueDevice = ueLteDevs.Get (0);
  Ptr<NetDevice> enbDevice = enbLteDevs.Get (0);

  //std::cout << ueLteDevs.GetN() << "\n";
  NS_LOG_UNCOND("Il numero di dispositivi connessi alla fine è: " << ueLteDevs.GetN() << "\n");

  /*
   *   Instantiate De-activation using Simulator::Schedule() method which will initiate bearer de-activation after deActivateTime
   *   Instantiate De-activation in sequence (Time const &time, MEM mem_ptr, OBJ obj, T1 a1, T2 a2, T3 a3)
   */
   
  Time deActivateTime (Seconds (1.5));
  Simulator::Schedule (deActivateTime, &LteHelper::DeActivateDedicatedEpsBearer, lteHelper, ueDevice, enbDevice, 2);

  //stop simulation after 3 seconds
  Simulator::Stop (Seconds (3.0));

  Simulator::Run ();
  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy ();
  return 0;

}