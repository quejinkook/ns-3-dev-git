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
// Network topology Leaf - Spine
//   calculate sending rate for  PFC and GPFC,
// - PIFO queues


#include <iostream>
#include <iomanip>
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
// ----- PFC & GPFC parameters
static std::string QUEUE_SIZE = "1000p";    // queue max size in packet
static uint32_t PFC_TH_MAX = 700;           // PFC XOFF threshold
static uint32_t PFC_TH_MIN = 128;           // PFC XON threshold
static uint32_t GPFC_TH_MAX = 400;          // GPFC XOFF threshold for low priority
static uint32_t GPFC_TH_MIN = 128;          // GPFC XON threshold
static uint32_t GPFC_PAUSE_PRIORITY = 2;    // GPFCPause Priority
static bool isGPFC = true;                  // true for apply GPFC threshold, false for PFC only,
static double FLOW_PRIORITY_MIN = 1;
static double FLOW_PRIORITY_MAX = 2;
static std::string DEQUEUE_MODE = "PQ";
static std::string ENQUEUE_MODE = "PI";

// -------- Topology parameters
//static uint8_t SPINE_COUNT = 2;                         // Spine Node Number
//static uint8_t LEAF_COUNT = 3;                          // Leaf Node Number
//static uint8_t HOST_COUNT = 4;                          // Node number that attached to Leaf Node
static std::string LEAF_HOST_LINK_SPEED = "1Gbps";     // leaf-node link speed
static std::string LEAF_HOST_LINK_DELAY = "10us";       // leaf-node link delay
static std::string LEAF_SPINE_LINK_SPEED = "2Gbps";    // leaf-spine link speed
static std::string LEAF_SPINE_LINK_DELAY = "10us";      // leaf-spine link delay

// --------- Flow Parameters
static uint32_t FLOW_COUNT = 2;                    // flow count per each node.
static std::string FLOW_RATE_HIGH = "0.7Gbps"; // flow rate for high priority flow
static std::string FLOW_RATE_LOW = "0.4Gbps"; // flow rate for low priority flow
static uint32_t FLOW_SIZE = 1000;                  // unit KB.
static uint32_t FLOW_PKT_SIZE = 512;

// ---------- Simulation Parameters
static Time startTime = Seconds (0);
static Time stopTime = Seconds (1);
static Time measurementWindow = MilliSeconds(10);
static Time GPFCStateUpdateWindow = MicroSeconds(100);


// ----------- Log files
static std::string FLOW_XML_NAME ="0518-PFC-Flow-XML.flowmon";
static std::string FLOW_RATE_NAME ="0518-GPFC-Flow-Rate.dat";
static std::string LOG_FILE_NAME ="0518-PFC-Flow-Stat.txt";
static std::string TRACE_FILE_NAME ="0518-PFC.tr";
static std::string PRIORITY_FILE_NAME ="priority.txt";


// static std::string HOST_LINK_DELAY = "1us"; // reference, for 10Gbps line, 100 meter increase 1300byte(bi-direction)






std::vector<uint64_t> flowSize_Pkts;
std::vector<std::string> flowRate_bits;
std::vector<uint32_t> flow_prioritys;

static std::string outputFilePath = ".";
std::vector<uint64_t> flowMon_Bytes;


static uint32_t TOTAL_FLOW_RATE = 0;
static uint32_t TOTAL_FLOW_RATE_HI = 0;
static uint32_t TOTAL_FLOW_RATE_LOW = 0;
static uint32_t TOTAL_FLOW_SIZE = 0;
static uint32_t TOTAL_FLOW_SIZE_HI = 0;
static uint32_t TOTAL_FLOW_SIZE_LOW = 0;

std::ofstream rxThroughput; // log file for throughput
std::vector<uint64_t> rxBytes; // counter for sending rates,



NS_LOG_COMPONENT_DEFINE ("GPFC_LEAF_SPINE");

// Simple Application for host,
// Sending packets in full rate that congifured, // TODO: Maybe change to possion process? jkchoi.200611
// Pause sending if the connected node became pause state,

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

void setRankTag(Ptr <Packet> packet, uint32_t id, uint32_t rank, uint32_t num)
{
    MySimpleTag rankTag;
    Time timeSent = Simulator::Now ();
    rankTag.SetRank(rank);
    rankTag.SetId(id);
    rankTag.SetNo(num);
    rankTag.SetTimeSent(timeSent.GetNanoSeconds());
    packet->AddPacketTag(rankTag);
    NS_LOG_INFO("[My App] Call Add PKT's SimpleTag :  Id:" << rankTag.GetId() << " rank:"  << rankTag.GetRank() << " No:"
    << rankTag.GetNo() <<" at Time:" << rankTag.GetTimeSent() );
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
        MySimpleTag rankTagTemp;
        rankTagTemp.SetRank(m_priority);
        rankTagTemp.SetId(m_appId);
        rankTagTemp.SetNo(m_packetsSent);
        rankTagTemp.SetTimeSent(Simulator::Now ().GetNanoSeconds());
        packet->AddPacketTag(rankTagTemp);


        //
//        setRankTag(packet, m_appId, m_priority, m_packetsSent);
//        uint32_t rank = getRankTag(packet);
//        NS_LOG_INFO("[My App "<< m_appId << "]======> Packet Info: AppID, AppPriority, PktNo, Delay " << m_appId << ","
//                         << rank << "," << m_packetsSent);

//        MySimpleTag rankTagTemp;
//        packet->PeekPacketTag (rankTagTemp);
////            uint32_t appId =  rankTagTemp.GetId();
//        uint32_t appPriority = rankTagTemp.GetRank();
////            uint32_t appPktNo = rankTagTemp.GetNo();
//        Time sentTime = rankTagTemp.GetTimeSent();
        Time finalTime = Simulator::Now ();
        uint64_t delayInNano =finalTime.GetNanoSeconds() - rankTagTemp.GetTimeSent();

        NS_LOG_WARN("[My App "<< m_appId << "]======> Dequeue Packet " << packet);
        NS_LOG_WARN("[My App "<< m_appId << "]======> Packet Info: AppID, AppPriority, PktNo, Delay :" << rankTagTemp.GetId() << ","
                         << rankTagTemp.GetRank() << "," << rankTagTemp.GetNo() << "," << delayInNano << "Ns");

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


void
TraceRate (std::size_t index, Ptr<const Packet> p, const Address& a)
{
    rxBytes[index] += p->GetSize ();
    NS_LOG_INFO("Time "<< Simulator::Now ().GetSeconds () << "Trace Rate at index " << index << " is "<< rxBytes[index]);
}

void
InitializeCounters (void){
    for (std::size_t i = 0; i < FLOW_COUNT; i++)
    {
        rxBytes[i] = 0;
    }
    NS_LOG_INFO("Done for Initialize Counters");

}
void
PrintThroughput (Time measurementWindow)
{
    rxThroughput <<  Simulator::Now ().GetMilliSeconds () << "ms " << "\t";
    for (std::size_t i = 0; i < FLOW_COUNT; i++)
    {
        rxThroughput << (rxBytes[i] * 8) / (measurementWindow.GetMilliSeconds ()) / 1e3 << "\t";

    }
    rxThroughput << std::endl;

    InitializeCounters();
    std::cout << "Print Throughput" << std::fixed << std::setprecision (1) << Simulator::Now ().GetMilliSeconds () << " ms simulation time" << std::endl;
    Simulator::Schedule (measurementWindow, &PrintThroughput, measurementWindow);

}
void
PrintProgress (Time interval)
{
    std::cout << "Progress to " << std::fixed << std::setprecision (1) << Simulator::Now ().GetMilliSeconds () << " ms simulation time" << std::endl;
    Simulator::Schedule (interval, &PrintProgress, interval);
}

void
GPFCStatusManageFunc (Ptr<PauseObj> ptr_pause_obj, QueueDiscContainer qdc, Time interval)
{
    bool isPause_temp = false;
    bool isUnPause_temp = false;
    uint32_t pauseRank_temp = 999;


	NS_LOG_INFO("Monitor Status for  " << ptr_pause_obj->getName() <<"  at Time: "<< std::fixed << std::setprecision (1)
    << Simulator::Now ().GetMicroSeconds () << "micro sec,  "
    << "  Is Pause" << ptr_pause_obj->getIsPause()
    << "  Pause Rank" << ptr_pause_obj->getPauseValue());

    std::cout << "Monitor Status for  " << ptr_pause_obj->getName()
    <<"  at Time: "<< std::fixed << std::setprecision (1)
    << Simulator::Now ().GetMicroSeconds () << "micro sec,  "
    << "  Is Pause" << ptr_pause_obj->getIsPause()
    << "  Pause Rank" << ptr_pause_obj->getPauseValue()
    << std::endl;


    for (std::size_t i = 0; i < qdc.GetN(); i++){
//    for (QueueDiscContainer::ConstIterator i = qdc.Begin (); i != qdc.End (); ++i)
//    {
//      NS_LOG_INFO("TCH: For loop index:" << i);
//        Ptr<PifoFastQueueDisc> qc = (Ptr<PifoFastQueueDisc>)(*i);
        Ptr<QueueDisc> qc = qdc.Get(i);
        uint32_t queue_occupancy = qc->getQueueOccupancy();
//        uint32_t queue_occupancy = 0;
        // check pause status

        std::cout << "Queue Occupancy for   " << qc <<"  is: " << queue_occupancy << std::endl;
        NS_LOG_INFO("Queue Occupancy for   " << qc <<"  is: " << queue_occupancy);


        // logic, check XOFF and XON condition,

        // if obj is unpaused, then check XOFF condition,
        // if isGPFC, then check GPFC XOFF condition,
        // else obj is paused,
        // if isGPFC, then check

//        if(!ptr_pause_obj->getIsPause()){
//            // if obj is unpaused, then check XOFF condition,
//            // if isGPFC, then check GPFC XOFF condition,
//
//
//        }else {
//
//        }






        if(isGPFC && queue_occupancy > GPFC_TH_MAX){
//        	NS_LOG_INFO("Check 1: goto GPFC pause status check");
//            std::cout << "Check 1: goto GPFC pause status check" << std::endl;

            //update pause rank if the last pause rank is bigger than GPFC_PAUSE_PRIORITY
            isPause_temp = true;
            pauseRank_temp = GPFC_PAUSE_PRIORITY;
//            std::cout << "XOFF by GPFC with pause rank" << pauseRank_temp << std::endl;

//            NS_LOG_INFO("XOFF by GPFC with pause rank" << pauseRank_temp);
        }

        if(queue_occupancy > PFC_TH_MAX){
//            NS_LOG_INFO("Check 2: goto GPFC pause status check");
//            std::cout << "Check 2: goto GPFC pause status check" << std::endl;
            isPause_temp = true;
            pauseRank_temp = 0; // pause all
//            std::cout << "XOFF by PFC with pause rank" << pauseRank_temp << std::endl;
//			NS_LOG_INFO("XOFF by PFC with pause rank" << pauseRank_temp);

        }

        // check unpause status
        if(queue_occupancy < PFC_TH_MIN){
//        	NS_LOG_INFO("Check 3: Unpause status check" );
//            std::cout << "Check 3: Unpause status check" << std::endl;
            isUnPause_temp = true;
//            pauseRank_temp = 999;
//            std::cout << "XON by PFC with pause rank" << std::endl;
//            NS_LOG_INFO("XON by PFC with pause rank" );
        }
//        std::cout << "Check 4: End for loop isPause_temp:" <<isPause_temp << "pauseRank_temp"<< pauseRank_temp
//        << "isUnPause_temp" << isUnPause_temp << std::endl;
//        std::cout << std::endl;
//        NS_LOG_INFO("Check 4: End for loop isPause_temp:" <<isPause_temp << "pauseRank_temp"<< pauseRank_temp
//        << "isUnPause_temp" << isUnPause_temp);

    }

    // update pause status




    std::cout << " Final Setup:";
    if(isPause_temp){
        ptr_pause_obj->setup(pauseRank_temp, true);
        std::cout << " Set XOFF State" << " pauseRank_temp" << pauseRank_temp <<std::endl;
        NS_LOG_INFO("Final Setup: Set XOFF State pauseRank_temp:" << pauseRank_temp );
    } else if(isUnPause_temp)
    {
        ptr_pause_obj->setup(0, false);
        std::cout << " Set XON State" << std::endl;
        NS_LOG_INFO("Final Setup: Set XON State" << pauseRank_temp );
    }

    std::cout << " =================================================================" <<std::endl;
    Simulator::Schedule (interval, &GPFCStatusManageFunc, ptr_pause_obj, qdc, interval);
}


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
    #if 0
//    LogComponentEnable ("GPFC_LEAF_SPINE", LOG_LEVEL_ALL);
//    LogComponentEnable ("PifoFastQueueDisc", LOG_LEVEL_WARN);
//    LogComponentEnable ("PifoFastQueueDisc", LOG_LEVEL_ERROR);
    LogComponentEnable ("PointToPointNetDevice", LOG_LEVEL_ALL);
    LogComponentEnable ("PointToPointChannel", LOG_LEVEL_ALL);
    LogComponentEnable ("TrafficControlHelper", LOG_LEVEL_ALL);
    LogComponentEnable ("UdpSocket", LOG_LEVEL_ALL);



//    LogComponentEnable ("PointToPointHelper", LOG_LEVEL_ALL);
//    LogComponentEnable ("FlowMonitor", LOG_LEVEL_INFO);

     LogComponentEnable ("QueueDisc", LOG_LEVEL_ALL);
//    LogComponentEnable ("InetSocketAddress", LOG_LEVEL_ALL);
//    LogComponentEnable ("Packet", LOG_LEVEL_ALL);
//    LogComponentEnable ("Socket", LOG_LEVEL_ALL);
    LogComponentEnable ("PifoFastQueueDisc", LOG_LEVEL_ALL);
    LogComponentEnable ("SimpleNetDevice", LOG_LEVEL_ALL);


//    LogComponentEnable ("RedQueueDisc", LOG_LEVEL_INFO);

    NS_LOG_INFO("Start Logging");
    #endif



    CommandLine cmd;

    //Sender Setting
    cmd.AddValue ("flow_count", "Total Flow Number", FLOW_COUNT);
//    cmd.AddValue ("input_band", "Total Input Bandwidth unit(Mbps)", INPUT_BAND);
    cmd.AddValue ("pkt_size", "Average PKT Size", FLOW_PKT_SIZE);
//    cmd.AddValue ("flow_rate", "Mean Flow Rate in Normal Distribution: unit String eg. 100mb/s", FLOW_RATE);
    cmd.AddValue ("flow_size", "Mean Flow Size in Normal Distribution: unit KB", FLOW_SIZE);
//    cmd.AddValue ("link_speed", "Link Speed between Sender and Receiver. eg. '100Mbps'", LINK_SPEED);
//    cmd.AddValue ("link_delay", "Link Delay Between Sender and Receiver. 1us", LINK_DELAY);
    cmd.AddValue ("priority_min", "Flow priority Minimum Value", FLOW_PRIORITY_MIN);
    cmd.AddValue ("priority_max", "Flow Priority Maximum Value", FLOW_PRIORITY_MAX);
    cmd.AddValue ("flow_rate_high", "Flow Rate for High Priority Flow, eg: 1Gbps", FLOW_RATE_HIGH);
    cmd.AddValue ("flow_rate_low", "Flow Rate for Low Priority Flow, eg: 1Gbps", FLOW_RATE_LOW);

    // Queue Setting
    cmd.AddValue ("is_gpfc", "Turn on GPFC?", isGPFC);
    cmd.AddValue ("queue_max_size_in_pkt", "Queue Size in Packet Count: e.g. 1000p", QUEUE_SIZE);
    cmd.AddValue ("pfc_threshold_max", "Threshold Max for PFC", PFC_TH_MAX );
    cmd.AddValue ("pfc_threshold_min", "Threshold Min for PFC", PFC_TH_MIN );
    cmd.AddValue ("gpfc_threshold_max", "Threshold Max for GPFC", GPFC_TH_MAX );
    cmd.AddValue ("gpfc_threshold_min", "Threshold Min for GPFC", GPFC_TH_MIN );
    cmd.AddValue ("gpfc_pause_priority", "GPFC Pause Priority", GPFC_PAUSE_PRIORITY );
    cmd.AddValue ("flow_monitor_xml_file_name", "Flow Monitoring XML File Name", FLOW_XML_NAME );
    cmd.AddValue ("flow_monitor_rate_file_name", "Flow Rate File Name", FLOW_RATE_NAME );
    cmd.AddValue ("log_file_name", "Log File Name", LOG_FILE_NAME );
    cmd.AddValue ("dequeue_mode", "Dequeue Mode: PQ(Priority Queue) or RR(Round Robin)", DEQUEUE_MODE );
    cmd.AddValue ("enqueue_mode", "Enqueue Mode: PI(Push in) or FI(First In )", ENQUEUE_MODE);
//    cmd.AddValue ("pause_mode", "Pause Mode: STATIC, or DYNAMIC", PAUSE_MODE );


    cmd.Parse (argc, argv);


    // check parameter parsing.
    NS_LOG_INFO("FLOW_COUNT:"<<FLOW_COUNT);
//    NS_LOG_INFO("INPUT_BAND:"<<INPUT_BAND);
    NS_LOG_INFO("FLOW_PKT_SIZE:"<<FLOW_PKT_SIZE);
    NS_LOG_INFO("FLOW_SIZE:"<<FLOW_SIZE);
//    NS_LOG_INFO("LINK_SPEED:"<<LINK_SPEED);
//    NS_LOG_INFO("LINK_DELAY:"<<LINK_DELAY);
    NS_LOG_INFO("FLOW_PRIORITY_MIN:"<<FLOW_PRIORITY_MIN);
    NS_LOG_INFO("FLOW_PRIORITY_MAX:"<<FLOW_PRIORITY_MAX);
    NS_LOG_INFO("FLOW_RATE_HIGH:"<<FLOW_RATE_HIGH);
    NS_LOG_INFO("FLOW_RATE_LOW:"<<FLOW_RATE_LOW);


    NS_LOG_INFO("isGPFC:"<<isGPFC);
    NS_LOG_INFO("QUEUE_SIZE:"<<QUEUE_SIZE);
    NS_LOG_INFO("PFC_TH_MAX:"<<PFC_TH_MAX);
    NS_LOG_INFO("PFC_TH_MIN:"<<PFC_TH_MIN);
    NS_LOG_INFO("PFC_PAUSE_PRIORITY:"<<GPFC_PAUSE_PRIORITY);
    NS_LOG_INFO("GPFC_TH_MAX:"<<GPFC_TH_MAX);
    NS_LOG_INFO("GPFC_TH_MIN:"<<GPFC_TH_MIN);
    NS_LOG_INFO("FLOW_XML_NAME:"<<FLOW_XML_NAME);
    NS_LOG_INFO("FLOW_RATE_NAME:"<<FLOW_RATE_NAME);
    NS_LOG_INFO("LOG_FILE_NAME:"<<LOG_FILE_NAME);
    NS_LOG_INFO("DEQUEUE_MODE:"<<DEQUEUE_MODE);
    NS_LOG_INFO("ENQUEUE_MODE:"<<ENQUEUE_MODE);
    
//    NS_LOG_INFO("PAUSE_MODE:"<<PAUSE_MODE);



    // step1. prepare for the flow size, and rate.
    // follows normal distribution,
    flowSize_Pkts.reserve(FLOW_COUNT);
    flowRate_bits.reserve(FLOW_COUNT);
    flowMon_Bytes.reserve(FLOW_COUNT);
    flow_prioritys.reserve(FLOW_COUNT);

    rxBytes.reserve(FLOW_COUNT);

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
        // set packet count as 100000
        uint32_t pkt_count = 100000;

        NS_LOG_INFO ("Flow ID #" << i << " s Size is " << pkt_count << " Pkts.");
        flowSize_Pkts.push_back(pkt_count);
        flow_size_list_str += std::to_string(pkt_count) + ",";

        std::string rate_str = "";
        // set flow rate
        if(i <= 2){
            rate_str = FLOW_RATE_HIGH;
        } else {
            rate_str = FLOW_RATE_LOW;
        }
        NS_LOG_INFO ("Flow ID #" << i << " s rate is " << rate_str);
        flowRate_bits.push_back(rate_str);
        flow_rate_list_str += rate_str + ",";

        uint32_t priority = i;
        NS_LOG_INFO("UniformRandomVariable result: Flow " << i << "'s priority is" << priority );
        flow_prioritys.push_back(priority);
        priority_list_str = priority_list_str + std::to_string(priority) + ",";

        TOTAL_FLOW_SIZE += pkt_count;
        TOTAL_FLOW_RATE += 0; // unused.
        if(priority < GPFC_PAUSE_PRIORITY)
        {
            TOTAL_FLOW_RATE_HI += 0;
            TOTAL_FLOW_SIZE_HI += pkt_count;
         }
        else{
            TOTAL_FLOW_RATE_LOW += 0;
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


    // pause object
    NS_LOG_INFO("INIT Pause Objs");
    PauseObj pauseObj_l1 = PauseObj("[pauseObj-L1]");
    PauseObj pauseObj_l2 = PauseObj("[pauseObj-L2]");
    PauseObj pauseObj_l3 = PauseObj("[pauseObj-L3]");
    PauseObj pauseObj_s1 = PauseObj("[pauseObj-S1]");
    PauseObj pauseObj_s2 = PauseObj("[pauseObj-S2]");
    PauseObj pauseObj_host = PauseObj("[pauseObj-HOST]");


    // Init Node Containers.
    NS_LOG_INFO("INIT Node Containers");
    // Init Leaf Node : 3
    Ptr<Node> L1 = CreateObject<Node> ();
    Ptr<Node> L2 = CreateObject<Node> ();
    Ptr<Node> L3 = CreateObject<Node> ();
    // Init Spine Node : 3
    Ptr<Node> S1 = CreateObject<Node> ();
    Ptr<Node> S2 = CreateObject<Node> ();
    // Init Host Node : 3 * 1
    Ptr<Node> H1 = CreateObject<Node> ();
    Ptr<Node> H2 = CreateObject<Node> ();
    Ptr<Node> H3 = CreateObject<Node> ();
    Ptr<Node> H4 = CreateObject<Node> ();
    Ptr<Node> H5 = CreateObject<Node> ();
    Ptr<Node> H6 = CreateObject<Node> ();


    // Init link types
    NS_LOG_INFO("INIT PtP Links");
    PointToPointHelper ptp_link_leaf_host; // point to point link between leaf and node.
    ptp_link_leaf_host.SetDeviceAttribute ("DataRate", StringValue (LEAF_HOST_LINK_SPEED));
    ptp_link_leaf_host.SetChannelAttribute ("Delay", StringValue (LEAF_HOST_LINK_DELAY));

    PointToPointHelper ptp_link_leaf_spine; // point to point link between leaf and node.
    ptp_link_leaf_spine.SetDeviceAttribute ("DataRate", StringValue (LEAF_SPINE_LINK_SPEED));
    ptp_link_leaf_spine.SetChannelAttribute ("Delay", StringValue (LEAF_SPINE_LINK_DELAY));


    // Init G-PFC queue
    NS_LOG_INFO("INIT TC Helper");
    TrafficControlHelper tch_pifo_l1;
    tch_pifo_l1.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),
                             "Name",StringValue("OQ_to_L1"),
                             "ptr_pause_obj",PointerValue(&pauseObj_l1),
                             "pfc_th_max", UintegerValue(PFC_TH_MAX),
                             "pfc_th_min", UintegerValue(PFC_TH_MIN),
                             "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
                             "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
                             "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
                             "max_priority", UintegerValue(FLOW_PRIORITY_MAX),
                             "is_gpfc", BooleanValue(isGPFC),
//                             "pause_mode", StringValue(PAUSE_MODE),
                             "dequeue_mode", StringValue(DEQUEUE_MODE),
                             "enqueue_mode", StringValue(ENQUEUE_MODE)
    );
    TrafficControlHelper tch_pifo_l2;
    tch_pifo_l2.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),
                                 "Name",StringValue("OQ_to_L2"),
                                 "ptr_pause_obj",PointerValue(&pauseObj_l2),
                                 "pfc_th_max", UintegerValue(PFC_TH_MAX),
                                 "pfc_th_min", UintegerValue(PFC_TH_MIN),
                                 "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
                                 "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
                                 "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
                                 "max_priority", UintegerValue(FLOW_PRIORITY_MAX),
                                 "is_gpfc", BooleanValue(isGPFC),
//                                 "pause_mode", StringValue(PAUSE_MODE),
                                 "dequeue_mode", StringValue(DEQUEUE_MODE),
                                 "enqueue_mode", StringValue(ENQUEUE_MODE)
    );
    TrafficControlHelper tch_pifo_l3;
    tch_pifo_l3.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),
                                 "Name",StringValue("OQ_to_L3"),
                                 "ptr_pause_obj",PointerValue(&pauseObj_l3),
                                 "pfc_th_max", UintegerValue(PFC_TH_MAX),
                                 "pfc_th_min", UintegerValue(PFC_TH_MIN),
                                 "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
                                 "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
                                 "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
                                 "max_priority", UintegerValue(FLOW_PRIORITY_MAX),
                                 "is_gpfc", BooleanValue(isGPFC),
//                                 "pause_mode", StringValue(PAUSE_MODE),
                                 "dequeue_mode", StringValue(DEQUEUE_MODE),
                                 "enqueue_mode", StringValue(ENQUEUE_MODE)
    );
    TrafficControlHelper tch_pifo_s1;
    tch_pifo_s1.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),
                                 "Name",StringValue("OQ_to_S1"),
                                 "ptr_pause_obj",PointerValue(&pauseObj_s1),
                                 "pfc_th_max", UintegerValue(PFC_TH_MAX),
                                 "pfc_th_min", UintegerValue(PFC_TH_MIN),
                                 "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
                                 "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
                                 "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
                                 "max_priority", UintegerValue(FLOW_PRIORITY_MAX),
                                 "is_gpfc", BooleanValue(isGPFC),
//                                 "pause_mode", StringValue(PAUSE_MODE),
                                 "dequeue_mode", StringValue(DEQUEUE_MODE),
                                 "enqueue_mode", StringValue(ENQUEUE_MODE)
    );
    TrafficControlHelper tch_pifo_s2;
    tch_pifo_s2.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),
                                 "Name",StringValue("OQ_to_S2"),
                                 "ptr_pause_obj",PointerValue(&pauseObj_s2),
                                 "pfc_th_max", UintegerValue(PFC_TH_MAX),
                                 "pfc_th_min", UintegerValue(PFC_TH_MIN),
                                 "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
                                 "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
                                 "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
                                 "max_priority", UintegerValue(FLOW_PRIORITY_MAX),
                                 "is_gpfc", BooleanValue(isGPFC),
//                                 "pause_mode", StringValue(PAUSE_MODE),
                                 "dequeue_mode", StringValue(DEQUEUE_MODE),
                                 "enqueue_mode", StringValue(ENQUEUE_MODE)
    );
    TrafficControlHelper tch_host;
    tch_host.SetRootQueueDisc("ns3::PifoFastQueueDisc","MaxSize",StringValue(QUEUE_SIZE),
                              "Name",StringValue("OQ_to_Host"),
                              "ptr_pause_obj",PointerValue(&pauseObj_host),
                              "pfc_th_max", UintegerValue(PFC_TH_MAX),
                              "pfc_th_min", UintegerValue(PFC_TH_MIN),
                              "gpfc_th_max", UintegerValue(GPFC_TH_MAX),
                              "gpfc_th_min", UintegerValue(GPFC_TH_MIN),
                              "gpfc_pause_priority", UintegerValue(GPFC_PAUSE_PRIORITY),
                              "max_priority", UintegerValue(FLOW_PRIORITY_MAX),
                              "is_gpfc", BooleanValue(isGPFC),
//                                 "pause_mode", StringValue(PAUSE_MODE),
                              "dequeue_mode", StringValue(DEQUEUE_MODE),
                              "enqueue_mode", StringValue(ENQUEUE_MODE)
    );

    // Install Output Queue.

    NS_LOG_INFO("Install Net Device Container Between Host and Leaf");
    // Make connection between leaf and host,
    NetDeviceContainer ndc_h1l1 = ptp_link_leaf_host.Install(H1,L1);
    NetDeviceContainer ndc_h2l1 = ptp_link_leaf_host.Install(H2,L1);
    NetDeviceContainer ndc_h3l2 = ptp_link_leaf_host.Install(H3,L2);
    NetDeviceContainer ndc_h4l2 = ptp_link_leaf_host.Install(H4,L2);
    NetDeviceContainer ndc_h5l3 = ptp_link_leaf_host.Install(H5,L3);
    NetDeviceContainer ndc_h6l3 = ptp_link_leaf_host.Install(H6,L3);

    NS_LOG_INFO("Install Net Device Container Between Leaf and Spine");
    // Make connection between leaf and host,
    NetDeviceContainer ndc_l1s1 = ptp_link_leaf_spine.Install(L1,S1);
    NetDeviceContainer ndc_l2s1 = ptp_link_leaf_spine.Install(L2,S1);
    NetDeviceContainer ndc_l3s1 = ptp_link_leaf_spine.Install(L3,S1);
    NetDeviceContainer ndc_l1s2 = ptp_link_leaf_spine.Install(L1,S2);
    NetDeviceContainer ndc_l2s2 = ptp_link_leaf_spine.Install(L2,S2);
    NetDeviceContainer ndc_l3s2 = ptp_link_leaf_spine.Install(L3,S2);

    InternetStackHelper stack;
    stack.InstallAll ();

    NS_LOG_INFO("Install Output Queue : ");
//    Ptr<TrafficControlLayer> tc = ndc_h1l1.Get(0)->GetNode ()->GetObject<TrafficControlLayer> ();
//    NS_LOG_INFO("TC value from ndc_h1l1.Get(0) : " << tc);
    //output queue to host

//    tch_pifo_l1.Install(ndc_h1l1);


    NetDeviceContainer oq_to_l1;
    oq_to_l1.Add(ndc_h1l1.Get(0));
    oq_to_l1.Add(ndc_h2l1.Get(0));
    oq_to_l1.Add(ndc_l1s1.Get(1));
    oq_to_l1.Add(ndc_l1s2.Get(1));

    NetDeviceContainer oq_to_l2;
    oq_to_l2.Add(ndc_h3l2.Get(0));
    oq_to_l2.Add(ndc_h4l2.Get(0));
    oq_to_l2.Add(ndc_l2s1.Get(1));
    oq_to_l2.Add(ndc_l2s2.Get(1));

    NetDeviceContainer oq_to_l3;
    oq_to_l3.Add(ndc_h5l3.Get(0));
    oq_to_l3.Add(ndc_h6l3.Get(0));
    oq_to_l3.Add(ndc_l3s1.Get(1));
    oq_to_l3.Add(ndc_l3s2.Get(1));

    NetDeviceContainer oq_to_s1;
    oq_to_s1.Add(ndc_l1s1.Get(0));
    oq_to_s1.Add(ndc_l2s1.Get(0));
    oq_to_s1.Add(ndc_l3s1.Get(0));

    NetDeviceContainer oq_to_s2;
    oq_to_s2.Add(ndc_l1s2.Get(0));
    oq_to_s2.Add(ndc_l2s2.Get(0));
    oq_to_s2.Add(ndc_l3s2.Get(0));

    NetDeviceContainer oq_to_host;
    oq_to_host.Add(ndc_h1l1.Get(1));
    oq_to_host.Add(ndc_h2l1.Get(1));
    oq_to_host.Add(ndc_h3l2.Get(1));
    oq_to_host.Add(ndc_h4l2.Get(1));
    oq_to_host.Add(ndc_h5l3.Get(1));
    oq_to_host.Add(ndc_h6l3.Get(1));


//    install output queue to l1
    QueueDiscContainer qdc_l1 = tch_pifo_l1.Install(oq_to_l1);
    QueueDiscContainer qdc_l2 = tch_pifo_l2.Install(oq_to_l2);
    QueueDiscContainer qdc_l3 = tch_pifo_l3.Install(oq_to_l3);
    QueueDiscContainer qdc_s1 = tch_pifo_s1.Install(oq_to_s1);
    QueueDiscContainer qdc_s2 = tch_pifo_s2.Install(oq_to_s2);
    QueueDiscContainer qdc_host = tch_host.Install(oq_to_host);


// 	set name for each output queue,
	qdc_l1.Get(0)->setQueueName("H1-to-L1");
	qdc_l1.Get(1)->setQueueName("H2-to-L1");
	qdc_l1.Get(2)->setQueueName("S1-to-L1");
	qdc_l1.Get(3)->setQueueName("S1-to-L1");
	qdc_l2.Get(0)->setQueueName("H3-to-L2");
	qdc_l2.Get(1)->setQueueName("H4-to-L2");
	qdc_l2.Get(2)->setQueueName("S1-to-L2");
	qdc_l2.Get(3)->setQueueName("S1-to-L2");
	qdc_l3.Get(0)->setQueueName("H5-to-L3");
	qdc_l3.Get(1)->setQueueName("H6-to-L3");
	qdc_l3.Get(2)->setQueueName("S1-to-L3");
	qdc_l3.Get(3)->setQueueName("S1-to-L3");
	qdc_s1.Get(0)->setQueueName("L1-to-S1");
	qdc_s1.Get(1)->setQueueName("L2-to-S1");
	qdc_s1.Get(2)->setQueueName("L3-to-S1");
	qdc_s2.Get(0)->setQueueName("L1-to-S2");
	qdc_s2.Get(1)->setQueueName("L2-to-S2");
	qdc_s2.Get(2)->setQueueName("L3-to-S2");
	qdc_host.Get(2)->setQueueName("L2-to-H3");






    QueueDiscContainer gpfc_mon_l1;
    // add l1-s1, l1-s2, l1-h1, l1-h2,
    gpfc_mon_l1.Add(qdc_s1.Get(0)); // add l1-s1
    gpfc_mon_l1.Add(qdc_s2.Get(0)); // add l1-s1
    gpfc_mon_l1.Add(qdc_host.Get(0)); // add l1-s1
    gpfc_mon_l1.Add(qdc_host.Get(1)); // add l1-s1

    QueueDiscContainer gpfc_mon_l2;
    // add l2-s1, l2-s2, l2-h3, l2-h4,
    gpfc_mon_l2.Add(qdc_s1.Get(1)); // add l2-s1
    gpfc_mon_l2.Add(qdc_s2.Get(1)); // add l2-s2
    gpfc_mon_l2.Add(qdc_host.Get(2)); // add l2-h3
    gpfc_mon_l2.Add(qdc_host.Get(3)); // add l2-h4

    QueueDiscContainer gpfc_mon_l3;
    // add l3-s1, l3-s2, l3-h5, l3-h6,
    gpfc_mon_l3.Add(qdc_s1.Get(2)); // add l3-s1
    gpfc_mon_l3.Add(qdc_s2.Get(2)); // add l3-s2
    gpfc_mon_l3.Add(qdc_host.Get(4)); // add l3-h3
    gpfc_mon_l3.Add(qdc_host.Get(5)); // add l3-h4

    QueueDiscContainer gpfc_mon_s1;
    // add s1-l1, s1-l2, s1-l3,
    gpfc_mon_s1.Add(qdc_l1.Get(2)); // add s1-l1
    gpfc_mon_s1.Add(qdc_l2.Get(2)); // add s1-l2
    gpfc_mon_s1.Add(qdc_l3.Get(2)); // add s1-l3

    QueueDiscContainer gpfc_mon_s2;
    // add s2-l1, s2-l2, s2-l3,
    gpfc_mon_s2.Add(qdc_l1.Get(3)); // add s1-l1
    gpfc_mon_s2.Add(qdc_l2.Get(3)); // add s2-l2
    gpfc_mon_s2.Add(qdc_l3.Get(3)); // add s3-l3

    //IP Setting

    // set receiver ip
//    address.SetBase ("100.0.0.0", "255.255.255.0");
//    Ipv4InterfaceContainer ipT1R1 = address.Assign (T1R1);

    // set sender ip
    // assign ip to each link, TODO: assign ip for each flow,
//    std::vector<Ipv4InterfaceContainer> ipH1;
//    std::vector<Ipv4InterfaceContainer> ipH2;
//    std::vector<Ipv4InterfaceContainer> ipH3;
//    std::vector<Ipv4InterfaceContainer> ipH4;
//
//    ipH1.reserve (FLOW_COUNT);
//    ipH2.reserve (FLOW_COUNT);
//    ipH3.reserve (FLOW_COUNT);
//    ipH4.reserve (FLOW_COUNT);


    Ipv4AddressHelper addressH1;
    Ipv4AddressHelper addressH2;
    Ipv4AddressHelper addressH3;
    Ipv4AddressHelper addressH4;
    Ipv4AddressHelper addressSpine1;
    Ipv4AddressHelper addressSpine2;
    addressH1.SetBase ("100.1.0.0", "255.255.255.0");
    addressH2.SetBase ("100.2.0.0", "255.255.255.0");
    addressH3.SetBase ("100.3.0.0", "255.255.255.0");
    addressH4.SetBase ("100.4.0.0", "255.255.255.0");

    addressSpine1.SetBase ("172.16.0.0", "255.255.255.0");
    addressSpine2.SetBase ("172.17.0.0", "255.255.255.0");


    Ipv4InterfaceContainer iph1l1 = addressH1.Assign(ndc_h1l1);
    addressH1.NewNetwork ();
    Ipv4InterfaceContainer iph2l1 = addressH1.Assign(ndc_h2l1);
    addressH1.NewNetwork ();

    Ipv4InterfaceContainer iph3l2 =addressH2.Assign(ndc_h3l2);
    addressH2.NewNetwork ();
    Ipv4InterfaceContainer iph4l2 =addressH2.Assign(ndc_h4l2);
    addressH2.NewNetwork ();

    Ipv4InterfaceContainer iph5l3 =addressH3.Assign(ndc_h5l3);
    addressH3.NewNetwork ();
    Ipv4InterfaceContainer iph6l3 =addressH3.Assign(ndc_h6l3);
    addressH3.NewNetwork ();

    Ipv4InterfaceContainer ipl1s1 =addressSpine1.Assign(ndc_l1s1);
    addressSpine1.NewNetwork ();
    Ipv4InterfaceContainer ipl2s1 =addressSpine1.Assign(ndc_l2s1);
    addressSpine1.NewNetwork ();
    Ipv4InterfaceContainer ipl3s1 =addressSpine1.Assign(ndc_l3s1);
    addressSpine1.NewNetwork ();

    Ipv4InterfaceContainer ipl1s2 =addressSpine2.Assign(ndc_l1s2);
    addressSpine2.NewNetwork ();
    Ipv4InterfaceContainer ipl2s2 =addressSpine2.Assign(ndc_l2s2);
    addressSpine2.NewNetwork ();
    Ipv4InterfaceContainer ipl3s2 =addressSpine2.Assign(ndc_l3s2);
    addressSpine2.NewNetwork ();


    // Init Routing Tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


    // Init Application,
    // Scenario1, start
    // Each sender in H1 sends to a receiver in H3
//    FLOW_COUNT = 2;
    std::vector<Ptr<PacketSink> > H1H3Sinks;
    H1H3Sinks.reserve(FLOW_COUNT);

    auto Receiver_Node = H3;
    auto Receiver_IP = iph3l2.GetAddress (0);
    auto SenderNode1 = H1;
    auto PauseObj1 =  &pauseObj_l1;
    auto SenderNode2 = H6;
    auto PauseObj2 =  &pauseObj_l3;

    for (uint32_t i = 0; i < FLOW_COUNT; i++){

        uint16_t port = 50000 + i+1;

//        uint32_t priority = flow_prioritys[i];
        uint32_t priority = i+1;
        Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
        ApplicationContainer sinkApp = sinkHelper.Install (Receiver_Node);

        Ptr<PacketSink> packetSink = sinkApp.Get (0)->GetObject<PacketSink> ();
        H1H3Sinks.push_back (packetSink);
        sinkApp.Start (startTime);
        sinkApp.Stop (stopTime + TimeStep (1));

        // app 1
        Address recvAddress (InetSocketAddress (Receiver_IP, port));
        Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (SenderNode1, UdpSocketFactory::GetTypeId ());
        Ptr<MyPFCApp> app = CreateObject<MyPFCApp> ();
        app->Setup (ns3UdpSocket, recvAddress, FLOW_PKT_SIZE, 10000,
                DataRate (FLOW_RATE_LOW),priority,PauseObj1,i);
        NS_LOG_INFO("PFC App Setup : pkt count" << 10000 << ", data rate:"
        << FLOW_RATE_LOW << ", priority is " << priority);
        SenderNode1->AddApplication(app);
        app->SetStartTime (startTime + TimeStep (1));
        app->SetStopTime (stopTime);

        // app 2
        ns3UdpSocket = Socket::CreateSocket (SenderNode2, UdpSocketFactory::GetTypeId ());
        app = CreateObject<MyPFCApp> ();
        app->Setup (ns3UdpSocket, recvAddress, FLOW_PKT_SIZE, 10000,
                    DataRate (FLOW_RATE_LOW),priority,PauseObj2,i+2);
        NS_LOG_INFO("PFC App Setup : pkt count" << 10000 << ", data rate:"
                                                << FLOW_RATE_LOW << ", priority is " << priority);
        SenderNode2->AddApplication(app);
        app->SetStartTime (startTime + TimeStep (1));
        app->SetStopTime (stopTime);

    }
    // Scenario1, end

    rxThroughput.open (FLOW_RATE_NAME, std::ios::out);
    rxThroughput << "#Time(s) flow thruput(Mb/s)" << std::endl;

    for (std::size_t i = 0; i < FLOW_COUNT; i++)
    {
        H1H3Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceRate, i));
    }

    Simulator::Schedule (startTime, &InitializeCounters);
    Simulator::Schedule (startTime, &PrintThroughput, measurementWindow);
    Simulator::Schedule (startTime, &PrintProgress, measurementWindow);

    // G-PFC State Management: Input, pauseObj, QueueDiscContainer, time interval,
    Simulator::Schedule (startTime, &GPFCStatusManageFunc, &pauseObj_l1, gpfc_mon_l1, GPFCStateUpdateWindow);
    Simulator::Schedule (startTime, &GPFCStatusManageFunc, &pauseObj_l2, gpfc_mon_l2, GPFCStateUpdateWindow);
    Simulator::Schedule (startTime, &GPFCStatusManageFunc, &pauseObj_l3, gpfc_mon_l3, GPFCStateUpdateWindow);
    Simulator::Schedule (startTime, &GPFCStatusManageFunc, &pauseObj_s1, gpfc_mon_s1, GPFCStateUpdateWindow);
    Simulator::Schedule (startTime, &GPFCStatusManageFunc, &pauseObj_s2, gpfc_mon_s2, GPFCStateUpdateWindow);
//    Simulator::Schedule (startTime, &GPFCStatusManageFunc, &pauseObj_host, qdc_s2, GPFCStateUpdateWindow);

    Simulator::Stop (stopTime + Seconds (2));

    AsciiTraceHelper ascii;
    ptp_link_leaf_host.EnableAsciiAll (ascii.CreateFileStream ("ptp_leaf_host.tr"));
    ptp_link_leaf_spine.EnableAsciiAll (ascii.CreateFileStream ("ptp_leaf_spine.tr"));

    FlowMonitorHelper flowmonHelper;
    flowmonHelper.InstallAll ();

    Simulator::Run ();
    rxThroughput.close ();

    flowmonHelper.SerializeToXmlFile (FLOW_XML_NAME, false, false);

    Simulator::Destroy ();
    return 0;

}
