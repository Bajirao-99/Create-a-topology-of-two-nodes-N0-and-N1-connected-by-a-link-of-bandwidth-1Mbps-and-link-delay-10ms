#include <iostream>
#include <fstream>
#include <string>
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/packet-sink.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;


Ptr<PacketSink> cbrReceiver[5], tcpReceiver;
long long int totalBytesTransfered = 0;
long long int totalPacketsDropped = 0;


NS_LOG_COMPONENT_DEFINE ("Assignment3");

// Function to record Congestion Window values (storing old congestion and new congestion window values)
// Can be used to find the slow start, congestion avoidance
static void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
    *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << oldCwnd << " " << newCwnd << "\n";
}

// Function to record packet drops
// totalPacketsDropped is incremented each time the packet is dropped
static void RxDrop(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
    totalPacketsDropped += 1;
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << totalPacketsDropped << "\n";
}

// Function to find the total cumulative transfered bytes
// totalBytesTransfered is incremented each time the byte is transfered
static void TotalRx(Ptr<OutputStreamWrapper> stream)
{
    totalBytesTransfered += tcpReceiver->GetTotalRx();
    
    for(int i = 0; i < 5; i++){
        totalBytesTransfered += cbrReceiver[i]->GetTotalRx();
    }
    
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << totalBytesTransfered << "\n";
    
    Simulator::Schedule(Seconds(0.001),&TotalRx, stream);
}

// Trace Congestion window length
// Function to call CwndChange function with appropriate stream to write the value in
static void TraceCwnd(Ptr<OutputStreamWrapper> stream)
{
    //Trace changes to the congestion window
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange,stream));
}

int executeSimulation(string tcpProtocol, uint32_t maxBytes = 0) {
    // maxBytes = 0;
    // Selecting the TCP type 
    if(tcpProtocol == "TcpWestwood"){
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
    }
    else if(tcpProtocol == "TcpNewReno"){
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
    }
    else if(tcpProtocol == "TcpScalable"){
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
    }
    else if(tcpProtocol == "TcpVegas"){
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));
    }
    else if(tcpProtocol == "TcpHybla"){
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
    }
    else {
        NS_LOG_DEBUG("Not Valid TCP Type");
        cout << "return ....." << endl;
        return 0;
    }

    // Explicitly create the point-to-point link required by the topology (shown above).
    NS_LOG_INFO("Creating channels");

    // Creating point-to-point link with given Data rate 1Mbps and Delay 10ms
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));
    // Bandwidth * delay= 10^4 bits = 1250 Bytes and we have set packet size = 1250 Bytes, so quesize is 1p.
    pointToPoint.SetQueue("ns3::DropTailQueue","MaxSize", StringValue ("1p"));

    // Explicitly create the nodes required by the topology(shown above).
    NS_LOG_INFO("Creating nodes");
    NodeContainer nodes;
    nodes.Create(2);

    // Attaching nodes to the NetDeviceContainer
    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Install the internet stack on the nodes
    InternetStackHelper internet;
    internet.Install(nodes);

    // Create error model
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute("ErrorRate", DoubleValue(0.00001));
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    // We've got the "hardware" in place. Now we need to add IP addresses.
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4Container = ipv4.Assign(devices);

    // Create a BulkSendApplication and install it on node 0
    NS_LOG_INFO("Create Applications");
    uint16_t portTCP = 12100;
    BulkSendHelper source("ns3::TcpSocketFactory",InetSocketAddress(ipv4Container.GetAddress(1), portTCP));

    // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get (0));
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(1.80));

    // Create a PacketSinkApplication and install it on node 1
    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), portTCP));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(1.80));

    tcpReceiver = DynamicCast<PacketSink>(sinkApps.Get(0));

    
    // Start and End times for the 5 CBR traffic agents
    double start[5] = {0.2, 0.4, 0.6, 0.8, 1.0};
    double end[5]   = {1.8, 1.8, 1.2, 1.4, 1.6};

    // creating a port
    uint16_t port = 12000;

    // creating 5 CBR streams given in assignment
    for(int i = 0; i < 5; i++){
        ApplicationContainer cbrApps;
        ApplicationContainer cbrSinkApps;

        // port+i is used to get the different port numbers
        OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ipv4Container.GetAddress(1), port+i));
        onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));
        onOffHelper.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        onOffHelper.SetAttribute("DataRate", StringValue("300Kbps"));
        onOffHelper.SetAttribute("StartTime", TimeValue(Seconds(start[i])));
        onOffHelper.SetAttribute("StopTime", TimeValue(Seconds(end[i])));
        cbrApps.Add(onOffHelper.Install(nodes.Get(0)));
        
        // Packet sinks(Receiver) for each CBR agent
        PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port+i));
        cbrSinkApps = sink.Install(nodes.Get(1));
        cbrSinkApps.Start(Seconds(0.0));
        cbrSinkApps.Stop(Seconds(1.8));
        cbrReceiver[i] = DynamicCast<PacketSink>(cbrSinkApps.Get(0));
    }

    // File names for storage    
    string str1 = tcpProtocol + "_bytes_received.dat";
    string str2 = tcpProtocol + "_dropped_packets.dat";
    string str3 = tcpProtocol + "_congestion_window_size.dat";

    AsciiTraceHelper ascii;

    // Create file streams for saving required data
    Ptr<OutputStreamWrapper> bytes_rcvd = ascii.CreateFileStream(str1); // total Bytes received
    Ptr<OutputStreamWrapper> dropped_packets = ascii.CreateFileStream(str2); // dropped packets
    Ptr<OutputStreamWrapper> cwnd_stat = ascii.CreateFileStream(str3); // congestion window size

    // setting dropped packet at time 0 as 0
    *dropped_packets->GetStream() << 0 << " " << 0 << "\n";

    // Keeping track of dropped packets using the RxDrop function
    devices.Get(1)->TraceConnectWithoutContext("PhyRxDrop", MakeBoundCallback(&RxDrop, dropped_packets));

    
    NS_LOG_INFO("Run Simulation");
    // Running recieved bytes count function
    Simulator::Schedule(Seconds(0.00001),&TotalRx,bytes_rcvd);
    // Running Congestion window size function
    Simulator::Schedule(Seconds(0.00001),&TraceCwnd,cwnd_stat);

    // Installing Flow monitor in all hosts.
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    // Stopping time for simulation
    Simulator::Stop(Seconds(1.80));
    Simulator::Run(); // Running the simulation
    
    // Saving flow data to file
    flowMonitor->SerializeToXmlFile("stats.flowmon", true, true);

    // closing simulation
    Simulator::Destroy();

    return 1;
}

int main(int argc, char *argv[]) {
    // Different protocols for congestion in TCP
    string protocols[] = {"TcpNewReno", "TcpHybla", "TcpVegas", "TcpScalable", "TcpWestwood"};
    
    for(int i = 0 ; i < 5; i++) {
        // Resetting the global paramaeters for simulation
        totalBytesTransfered = 0;
        totalPacketsDropped = 0;
        Config::Reset();

        // selected protocol
        string protocol = protocols[i];
        // cout << "********************************************" << endl;
        cout << "****** Running for protocol : " << protocol << " ****" << endl;
        executeSimulation(protocol);
        cout << "********************** END ************************" << endl << endl;
    }
}

