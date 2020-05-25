/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

//
// Network topology
//   4:1 incast
//
//
//
//
//
//====================  original code below ============
// - all links are point-to-point links with indicated one-way BW/delay
// - CBR/UDP flows from n0 to n3, and from n3 to n1
// - FTP/TCP flow from n0 to n3, starting at time 1.2 to time 1.35 sec.
// - UDP packet size of 210 bytes, with per-packet interval 0.00375 sec.
//   (i.e., DataRate of 448,000 bps)
// - DropTail queues
// - Tracing of queues and packet receptions to file "simple-global-routing.tr"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/pifo-fast-queue-disc.h"
#include "ns3/random-variable-stream.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/myTag.h"
#include "ns3/pause-obj.h"



using namespace ns3;
using namespace std;
//class PauseObj;

//parameters
static uint32_t PFC_TH_MAX = 700;
static uint32_t PFC_TH_MIN = 128;
static uint32_t GPFC_TH_MAX = 700;
static uint32_t GPFC_TH_MIN = 128;
static uint32_t GPFC_PAUSE_PRIORITY = 4;

static std::string QUEUE_SIZE = "1000p";
static bool isGPFC = false;

static uint32_t FLOW_COUNT = 64; // flow per each node.
static uint32_t INPUT_BAND = 400; // 4096Mbps -> 4Gbps. 400-> 400Mbps
static uint32_t FLOW_PKT_SIZE = 512; // packet length is 512 byte
//static std::string FLOW_RATE = "100mb/s"; // packet length is 512 byte
static uint32_t FLOW_SIZE = 100000; // unit KB.
static std::string LINK_SPEED = "100Mbps";
static std::string LINK_DELAY = "1us";
static std::string PAUSE_MODE = "STATIC";// or DYNAMIC
// static std::string HOST_LINK_SPEED = "100Mbps";
// static std::string HOST_LINK_DELAY = "1us"; // reference, for 10Gbps line, 100 meter increase 1300byte(bi-direction)
static Time startTime = Seconds (0);
static Time stopTime = Seconds (300);
static PauseObj pauseObj = PauseObj();
static double FLOW_PRIORITY_MIN = 1;
static double FLOW_PRIORITY_MAX = 8;
static std::string DEQUEUE_MODE = "PQ";
static std::string ENQUEUE_MODE = "PI";

static std::string FLOW_XML_NAME ="0324-PFC-Flow-XML.flowmon";
static std::string LOG_FILE_NAME ="0324-PFC-Flow-Stat.txt";
static std::string TRACE_FILE_NAME ="0324-PFC.tr";
static std::string PRIORITY_FILE_NAME ="priority.txt";

std::vector<uint64_t> flowSize_Pkts;
std::vector<std::string> flowRate_bits;
std::vector<uint32_t> flow_prioritys;


static std::string EXAMPLE_NAME = "GreedyPFCExample";
static std::string outputFilePath = ".";
std::vector<uint64_t> flowMon_Bytes;


static uint32_t TOTAL_FLOW_RATE = 0;
static uint32_t TOTAL_FLOW_RATE_HI = 0;
static uint32_t TOTAL_FLOW_RATE_LOW = 0;
static uint32_t TOTAL_FLOW_SIZE = 0;
static uint32_t TOTAL_FLOW_SIZE_HI = 0;
static uint32_t TOTAL_FLOW_SIZE_LOW = 0;


NS_LOG_COMPONENT_DEFINE (EXAMPLE_NAME);

class MyPFCApp : public Application
{
public:

    MyPFCApp();
    virtual ~MyPFCApp();

    void Setup(Ptr<Socket> socket, Address address, uint32_t pkt_size, uint32_t npkt, DataRate dataRate);
    void Setup(Ptr<Socket> socket, Address address, uint32_t pkt_size, uint32_t npkt, DataRate dataRate, uint8_t priority);
    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint8_t priority, Ptr<PauseObj> ptr_pause_obj);
    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, 
        uint8_t priority, Ptr<PauseObj> ptr_pause_obj, uint32_t appId);

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleTx (void);
    void SendPacket (void);
    void SetPause(bool isPause);


    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_nPackets;
    DataRate        m_dataRate;
    EventId         m_sendEvent;
    bool            m_running;
    bool            m_isPause;
    uint32_t        m_packetsSent;
    uint32_t        m_priority;
    Ptr<PauseObj>   m_ptr_pauseObject;
    uint32_t        m_appId;
};

MyPFCApp::MyPFCApp ()
        : m_socket (0),
          m_peer (),
          m_packetSize (0),
          m_nPackets (0),
          m_dataRate (0),
          m_sendEvent (),
          m_running (false),
          m_isPause (false),
          m_packetsSent (0),
          m_priority(0),
          m_ptr_pauseObject(),
          m_appId(9999)
{
}

MyPFCApp::~MyPFCApp()
{
    m_socket = 0;
}

void
MyPFCApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}
void
MyPFCApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint8_t priority)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
    m_priority = priority;
}
void
MyPFCApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint8_t priority, Ptr<PauseObj> ptr_pause_obj)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
    m_priority = priority;
    m_ptr_pauseObject = ptr_pause_obj;
}
void
MyPFCApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, 
    DataRate dataRate, uint8_t priority, Ptr<PauseObj> ptr_pause_obj, uint32_t appId)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
    m_priority = priority;
    m_ptr_pauseObject = ptr_pause_obj;
    m_appId = appId;
}



void
MyPFCApp::StartApplication (void)
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind ();
    m_socket->Connect (m_peer);
    SendPacket ();
}

void
MyPFCApp::StopApplication (void)
{
    m_running = false;

    if (m_sendEvent.IsRunning ())
    {
        Simulator::Cancel (m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close ();
    }
}

uint32_t getRankTag(Ptr <Packet> packet)
{
    MySimpleTag rankTagTemp;
    packet->PeekPacketTag (rankTagTemp);
    NS_LOG_INFO("[My App] Call Get PKT's SimpleTag :  "  << rankTagTemp.GetRank());
    return rankTagTemp.GetRank();
}

void setRankTag(Ptr <Packet> packet, uint32_t rank)
{
    MySimpleTag rankTag;
    rankTag.SetRank(rank);
    packet->AddPacketTag(rankTag);
    NS_LOG_INFO("[My App] Call Add PKT's SimpleTag :  "  << rank);

}

void
MyPFCApp::SendPacket (void)
{
    // if it is pause state, then go to schedule,
    // else, send packet,

    NS_LOG_INFO("[My App "<< m_appId << "] Pause Obj Value :  priority"  << m_ptr_pauseObject->getPauseValue() << "is pause : " << m_ptr_pauseObject->getIsPause());

    if(m_ptr_pauseObject->getIsPause() && m_priority >=  m_ptr_pauseObject->getPauseValue() )
        {
            NS_LOG_INFO("[My App "<< m_appId << "] Priority Pause: Stop Sending Packet with priority" << m_priority << " is larger or equal to pause priority." << m_ptr_pauseObject->getPauseValue());
            ScheduleTx ();
        }
    else {

        Ptr <Packet> packet = Create<Packet>(m_packetSize);

//        m_priority = m_nPackets - m_packetsSent;
//        NS_LOG_INFO("[My App] The packet's priority is " << m_nPackets << "-" << m_packetsSent << "=" << m_priority);
//        NS_LOG_INFO("[My App] The packet's priority is " << m_priority << "/ 64 =" << (m_priority/64));
//        m_priority = m_priority / 64;

//        SocketIpTosTag tosTag;
//        tosTag.SetTos (m_priority >> 2);
//        NS_LOG_INFO("[My App] Call Replace PKT's PriorityTag :  "  << tosTag.GetTos());
//        packet->ReplacePacketTag (tosTag);

        setRankTag(packet, m_priority);
        uint32_t rank = getRankTag(packet);

        NS_LOG_INFO("[My App "<< m_appId << "] Call Get PKT's SimpleTag from function. :  "  << rank);


        // SocketPriorityTag priorityTag;
        // priorityTag.SetPriority (m_priority);
        // NS_LOG_INFO("[My App "<< m_appId << "] Call Replace PKT's PriorityTag :  "  << m_priority);

        // packet->ReplacePacketTag (priorityTag);

        // SocketPriorityTag priorityTagTemp;
        // packet->PeekPacketTag (priorityTagTemp);

        // NS_LOG_INFO("[My App "<< m_appId << "] Check Replace PKT's PriorityTag :  "  << priorityTagTemp.GetPriority());

        // NS_LOG_INFO("[My App "<< m_appId << "] Send "<< m_packetsSent << " th Packet at Time :  "  << Simulator::Now ().GetMicroSeconds());


        m_socket->Send(packet);

        if (++m_packetsSent < m_nPackets) {
            ScheduleTx();
        }
        else{
            NS_LOG_INFO("[My App "<< m_appId << "] Finish to Send the "<< m_packetsSent << "th Packet at Time :  "  << Simulator::Now ().GetMicroSeconds());
        }

    }
}

void
MyPFCApp::ScheduleTx (void)
{
    if (m_running)
    {
        Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
        m_sendEvent = Simulator::Schedule (tNext, &MyPFCApp::SendPacket, this);
    }
}

void
MyPFCApp::SetPause (bool isPause)
{
    m_isPause = isPause;
}


//static void
//RxDrop (Ptr<const Packet> p)
//{
//    NS_LOG_INFO ("[Callback] RxDrop at " << Simulator::Now ().GetSeconds ());
//    std::cout << "std::cout  RxDrop at " <<  Simulator::Now ().GetSeconds () << std::endl;
//}

//static void
//PauseChanged (uint32_t oldValue, uint32_t newValue)
//{
//    NS_LOG_INFO ("[PauseChanged]Queue Pause Changed at " << Simulator::Now ().GetSeconds ());
//    NS_LOG_INFO ("[PauseChanged]Queue Pause Threshold Changed from" << oldValue << " to " << newValue);
//
//    std::cout << "std::cout  Traced pause priority changed from  " << oldValue
//    << "to " << newValue << " at" <<  Simulator::Now ().GetSeconds () << std::endl;
//}




//static void
//FlowPause(uint32_t flow_id)
//{
//    NS_LOG_UNCOND ("Pause Flow Id " << flow_id << "at " << Simulator::Now ().GetSeconds ());
//}


void
writePriorityFile(std::string file_name, std::string priority_str)
{
    ofstream myfile;
    myfile.open (file_name);
    myfile << priority_str <<"\n";
    myfile.close();
}

int
main (int argc, char *argv[])
{
    // Users may find it convenient to turn on explicit debugging
    // for selected modules; the below lines suggest how to do this
    #if 1
     LogComponentEnable ("GreedyPFCExample", LOG_LEVEL_ALL);
    LogComponentEnable ("PifoFastQueueDisc", LOG_LEVEL_ALL);
    // LogComponentEnable ("QueueDisc", LOG_LEVEL_LOGIC);
//    LogComponentEnable ("InetSocketAddress", LOG_LEVEL_ALL);
//    LogComponentEnable ("Packet", LOG_LEVEL_ALL);
//    LogComponentEnable ("Socket", LOG_LEVEL_ALL);
//    LogComponentEnable ("SimpleNetDevice", LOG_LEVEL_ALL);


//    LogComponentEnable ("RedQueueDisc", LOG_LEVEL_INFO);
    #endif

    NS_LOG_INFO("Start Logging");


    // Set up some default values for the simulation.  Use the
//    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (FLOW_PKT_SIZE));
//    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("100Mbps"));
    //DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

    // Allow the user to override any of the defaults and the above
    // DefaultValue::Bind ()s at run-time, via command-line arguments
    CommandLine cmd;

    //Sender Setting
    cmd.AddValue ("flow_count", "Total Flow Number", FLOW_COUNT);
    cmd.AddValue ("input_band", "Total Input Bandwidth unit(Mbps)", INPUT_BAND);
    cmd.AddValue ("pkt_size", "Average PKT Size", FLOW_PKT_SIZE);
//    cmd.AddValue ("flow_rate", "Mean Flow Rate in Normal Distribution: unit String eg. 100mb/s", FLOW_RATE);
    cmd.AddValue ("flow_size", "Mean Flow Size in Normal Distribution: unit KB", FLOW_SIZE);
    cmd.AddValue ("link_speed", "Link Speed between Sender and Receiver. eg. '100Mbps'", LINK_SPEED);
    cmd.AddValue ("link_delay", "Link Delay Between Sender and Receiver. 1us", LINK_DELAY);
    cmd.AddValue ("priority_min", "Flow priority Minimum Value", FLOW_PRIORITY_MIN);
    cmd.AddValue ("priority_max", "Flow Priority Maximum Value", FLOW_PRIORITY_MAX);

    // Queue Setting
    cmd.AddValue ("is_gpfc", "Turn on GPFC?", isGPFC);
    cmd.AddValue ("queue_max_size_in_pkt", "Queue Size in Packet Count: e.g. 1000p", QUEUE_SIZE);
    cmd.AddValue ("pfc_threshold_max", "Threshold Max for PFC", PFC_TH_MAX );
    cmd.AddValue ("pfc_threshold_min", "Threshold Min for PFC", PFC_TH_MIN );
    cmd.AddValue ("gpfc_threshold_max", "Threshold Max for GPFC", GPFC_TH_MAX );
    cmd.AddValue ("gpfc_threshold_min", "Threshold Min for GPFC", GPFC_TH_MIN );
    cmd.AddValue ("gpfc_pause_priority", "GPFC Pause Priority", GPFC_PAUSE_PRIORITY );
    cmd.AddValue ("flow_monitor_xml_file_name", "Flow Monitoring XML File Name", FLOW_XML_NAME );
    cmd.AddValue ("log_file_name", "Log File Name", LOG_FILE_NAME );
    cmd.AddValue ("dequeue_mode", "Dequeue Mode: PQ(Priority Queue) or RR(Round Robin)", DEQUEUE_MODE );
    cmd.AddValue ("enqueue_mode", "Enqueue Mode: PI(Push in) or FI(First In )", ENQUEUE_MODE);
    cmd.AddValue ("pause_mode", "Pause Mode: STATIC, or DYNAMIC", PAUSE_MODE );

    cmd.Parse (argc, argv);


    // check parameter parsing.
    NS_LOG_INFO("FLOW_COUNT:"<<FLOW_COUNT);
    NS_LOG_INFO("INPUT_BAND:"<<INPUT_BAND);
    NS_LOG_INFO("FLOW_PKT_SIZE:"<<FLOW_PKT_SIZE);
    NS_LOG_INFO("FLOW_SIZE:"<<FLOW_SIZE);
    NS_LOG_INFO("LINK_SPEED:"<<LINK_SPEED);
    NS_LOG_INFO("LINK_DELAY:"<<LINK_DELAY);
    NS_LOG_INFO("FLOW_PRIORITY_MIN:"<<FLOW_PRIORITY_MIN);
    NS_LOG_INFO("FLOW_PRIORITY_MAX:"<<FLOW_PRIORITY_MAX);
    NS_LOG_INFO("isGPFC:"<<isGPFC);
    NS_LOG_INFO("QUEUE_SIZE:"<<QUEUE_SIZE);
    NS_LOG_INFO("PFC_TH_MAX:"<<PFC_TH_MAX);
    NS_LOG_INFO("PFC_TH_MIN:"<<PFC_TH_MIN);
    NS_LOG_INFO("PFC_PAUSE_PRIORITY:"<<GPFC_PAUSE_PRIORITY);
    NS_LOG_INFO("GPFC_TH_MAX:"<<GPFC_TH_MAX);
    NS_LOG_INFO("GPFC_TH_MIN:"<<GPFC_TH_MIN);
    NS_LOG_INFO("FLOW_XML_NAME:"<<FLOW_XML_NAME);
    NS_LOG_INFO("LOG_FILE_NAME:"<<LOG_FILE_NAME);
    NS_LOG_INFO("DEQUEUE_MODE:"<<DEQUEUE_MODE);
    NS_LOG_INFO("ENQUEUE_MODE:"<<ENQUEUE_MODE);
    
    NS_LOG_INFO("PAUSE_MODE:"<<PAUSE_MODE);


    // step1. prepare for the flow size, and rate.
    // follows normal distribution,
    flowSize_Pkts.reserve(FLOW_COUNT);
    flowRate_bits.reserve(FLOW_COUNT);
    flowMon_Bytes.reserve(FLOW_COUNT);
    flow_prioritys.reserve(FLOW_COUNT);

    Ptr<NormalRandomVariable> ramdom_size = CreateObject<NormalRandomVariable> ();
    ramdom_size->SetAttribute ("Mean", DoubleValue (0.0));
    ramdom_size->SetAttribute ("Variance", DoubleValue (1.0));

    Ptr<NormalRandomVariable> ramdom_rate = CreateObject<NormalRandomVariable> ();
    ramdom_rate->SetAttribute ("Mean", DoubleValue (0.0));
    ramdom_rate->SetAttribute ("Variance", DoubleValue (1.0));

    Ptr<UniformRandomVariable> random_priority = CreateObject<UniformRandomVariable> ();
    random_priority->SetAttribute ("Min", DoubleValue(FLOW_PRIORITY_MIN));
    random_priority->SetAttribute ("Max", DoubleValue(FLOW_PRIORITY_MAX));


    std::string priority_list_str = "";
    std::string flow_size_list_str = "";
    std::string flow_rate_list_str = "";
    // calculate flow size and rate, with normal random variable.
    for (std::size_t i = 1; i <= FLOW_COUNT; i++)
    {
        // set flow size
        double size = ramdom_size->GetValue();
//        NS_LOG_INFO ("double ramdom value is " << size );
        uint32_t pkt_count = (size + 4) * FLOW_SIZE / FLOW_PKT_SIZE / 4;

        NS_LOG_INFO ("Flow ID #" << i << " s Size is " << pkt_count << " Pkts.");
        flowSize_Pkts.push_back(pkt_count);
        flow_size_list_str += std::to_string(pkt_count) + ",";


        // set flow rate
        double rate = ramdom_rate->GetValue();

        uint32_t flow_rate = (rate + 4) * INPUT_BAND / FLOW_COUNT / 4;


        std::string rate_str = "" + std::to_string(flow_rate) + "Mbps";
        NS_LOG_INFO ("Flow ID #" << i << " s rate is " << rate_str);
        flowRate_bits.push_back(rate_str);
        flow_rate_list_str += rate_str + ",";

        uint32_t priority = random_priority->GetInteger();
        NS_LOG_INFO("UniformRandomVariable result: Flow " << i << "'s priority is" << priority );
        flow_prioritys.push_back(priority);
        priority_list_str = priority_list_str + std::to_string(priority) + ",";

        TOTAL_FLOW_SIZE += pkt_count;
        TOTAL_FLOW_RATE += flow_rate;
        if(priority < GPFC_PAUSE_PRIORITY)
        {

            TOTAL_FLOW_RATE_HI += flow_rate;
            TOTAL_FLOW_SIZE_HI += pkt_count;
         }
        else{

            TOTAL_FLOW_RATE_LOW += flow_rate;
            TOTAL_FLOW_SIZE_LOW += pkt_count;
        }

    }
        NS_LOG_INFO("Total Priority List is : " << priority_list_str);
        NS_LOG_INFO("Write Priority List into file :" );
        // writePriorityFile(PRIORITY_FILE_NAME, priority_list_str);
        NS_LOG_INFO("Write Priority List into file : Done");
        NS_LOG_INFO("Total Flow Size List is : " << flow_size_list_str);
        NS_LOG_INFO("Total Flow Rate List is : " << flow_rate_list_str);
        NS_LOG_INFO("Total Flow Rate is : " << TOTAL_FLOW_RATE);

        NS_LOG_INFO("Total Flow Rate High , Low : " << TOTAL_FLOW_RATE_HI << ", " << TOTAL_FLOW_RATE_LOW);
        NS_LOG_INFO("Total Flow SIZE High , Low : " << TOTAL_FLOW_SIZE_HI << ", " << TOTAL_FLOW_SIZE_LOW);


        // Write Important Parameters
            ofstream myfile;
            myfile.open (PRIORITY_FILE_NAME);
            myfile << priority_list_str <<"\n";
            myfile << "Total Flow Size List is : " << flow_size_list_str <<"\n";
            myfile << "Total Flow Rate List is : " << flow_rate_list_str <<"\n";
            myfile << "Total Flow Rate is : " << TOTAL_FLOW_RATE <<"\n";
            myfile << "Total Flow Rate High , Low : " << TOTAL_FLOW_RATE_HI << ", " << TOTAL_FLOW_RATE_LOW <<"\n";
            myfile << "Total Flow SIZE High , Low : " << TOTAL_FLOW_SIZE_HI << ", " << TOTAL_FLOW_SIZE_LOW <<"\n";
            myfile.close();


    // Init Node Container.
    NodeContainer N1;
    N1.Create(FLOW_COUNT);

    // Init ToR Switch, Aggregate Switch, Receiver.
    Ptr<Node> T1 = CreateObject<Node> ();
    Ptr<Node> R1 = CreateObject<Node> ();

    // Create links.
    std::vector<NetDeviceContainer> N1T1;
    N1T1.reserve (FLOW_COUNT);

    PointToPointHelper pointToPointLink;
    pointToPointLink.SetDeviceAttribute ("DataRate", StringValue (LINK_SPEED));
    pointToPointLink.SetChannelAttribute ("Delay", StringValue (LINK_DELAY));

    // link T1-to-R1
    NetDeviceContainer T1R1 = pointToPointLink.Install (T1, R1);

    for (std::size_t i = 0; i < FLOW_COUNT; i++){
        Ptr<Node> n = N1.Get (i);
        N1T1.push_back(pointToPointLink.Install(n,T1));
//        N1T1[i].Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
//        N1T1[i].Get (0)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
    }

    InternetStackHelper stack;
    stack.InstallAll ();

    // Queue Setting

    TrafficControlHelper tchRed1;
    // MinTh = 50, MaxTh = 150 recommended in ACM SIGCOMM 2010 DCTCP Paper
    // This yields a target (MinTh) queue depth of 60us at 10 Gb/s

    tchRed1.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),
            "Name",StringValue("tchRed1"),
            "ptr_pause_obj",PointerValue(&pauseObj),
            "pfc_th_max", UintegerValue(PFC_TH_MAX),
             "pfc_th_min", UintegerValue(PFC_TH_MIN),
             "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
             "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
             "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
             "max_priority", UintegerValue(FLOW_PRIORITY_MAX),
             "is_gpfc", BooleanValue(isGPFC),
             "pause_mode", StringValue(PAUSE_MODE),
             "dequeue_mode", StringValue(DEQUEUE_MODE),
             "enqueue_mode", StringValue(ENQUEUE_MODE)

    );
    NS_LOG_INFO("Install tchRed1 : ");
    tchRed1.Install (T1R1);


    TrafficControlHelper tchRed2;
    tchRed2.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),"Name",StringValue("tchRed2"),
             "pfc_th_max", UintegerValue(PFC_TH_MAX),
             "pfc_th_min", UintegerValue(PFC_TH_MIN),
             "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
             "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
             "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
             "max_priority", UintegerValue(1),
             "is_gpfc", BooleanValue(isGPFC),
             "pause_mode", StringValue(PAUSE_MODE),
             "dequeue_mode", StringValue("PQ"),
             "enqueue_mode", StringValue("FI")
    );

//    tchRed2.SetRootQueueDisc("ns3::DropTailQueueDisc","MaxSize",StringValue(QUEUE_SIZE));


//    tchRed2.Install (T1R1);

    for (std::size_t i = 0; i < FLOW_COUNT; i++){
        NS_LOG_INFO("Install tchRed2 : " << i);
        tchRed2.Install(N1T1[i]);
    }

    //IP Setting
    Ipv4AddressHelper address;

    // set receiver ip
    address.SetBase ("100.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ipT1R1 = address.Assign (T1R1);

    // set sender ip
    std::vector<Ipv4InterfaceContainer> ipN1T1;
    ipN1T1.reserve (FLOW_COUNT);
    address.SetBase ("192.168.10.0", "255.255.255.0");
    for (std::size_t i = 0; i < FLOW_COUNT; i++){
        ipN1T1.push_back (address.Assign (N1T1[i]));
        address.NewNetwork ();
    }

    // Init Routing Tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Each sender in N1 sends to a receiver in R1
    std::vector<Ptr<PacketSink> > N1R1Sinks;
    N1R1Sinks.reserve(FLOW_COUNT);

    for (std::size_t i = 0; i < FLOW_COUNT; i++){

        uint16_t port = 50000 + i+1;
//        uint16_t priority = i << 2;

        uint32_t priority = flow_prioritys[i];

        Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
        ApplicationContainer sinkApp = sinkHelper.Install (R1);

        Ptr<PacketSink> packetSink = sinkApp.Get (0)->GetObject<PacketSink> ();
        N1R1Sinks.push_back (packetSink);
        sinkApp.Start (startTime);
        sinkApp.Stop (stopTime + TimeStep (1));

        Address recvAddress (InetSocketAddress (ipT1R1.GetAddress (1), port));
        Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (N1.Get (i), UdpSocketFactory::GetTypeId ());

        Ptr<MyPFCApp> app = CreateObject<MyPFCApp> ();
        app->Setup (ns3UdpSocket, recvAddress, FLOW_PKT_SIZE, flowSize_Pkts[i], DataRate (flowRate_bits[i]),priority,&pauseObj,i);
        NS_LOG_INFO("PFC App Setup : pkt count" << flowSize_Pkts[i] << ", data rate:" << flowRate_bits[i] << ", priority is " << priority);
        N1.Get(i)->AddApplication(app);
        app->SetStartTime (startTime + TimeStep (1));
        app->SetStopTime (stopTime);

    }

    Simulator::Stop (stopTime + TimeStep (2));

    AsciiTraceHelper ascii;
    pointToPointLink.EnableAsciiAll (ascii.CreateFileStream ("Greedy-PFC.tr"));

    FlowMonitorHelper flowmonHelper;
    flowmonHelper.InstallAll ();


    Simulator::Run ();
    flowmonHelper.SerializeToXmlFile (FLOW_XML_NAME, false, false);
    Simulator::Destroy ();
    return 0;

}
