/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2014 University of Washington
 *               2015 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Zhenguo Cui <quejinkook@korea.ac.kr>
 */

/*
 *  This module presents PIFO queue descipline.
 *  It based on pfifo-fast-queue-disc.cc .
 *
 *  The original pfifo-fast-queue uses 3 internal priority queue,
 *  while, this module uses N internal priority queue(default is 1024), the value N is user-configurable.
 *
 *  each packet has its own priority, with range(from 0 to N-1). and enqueue to dedicated queue,
 *
 *  The total allowed buffered packet count is M, which is also user-configurable.
 *
 *  when the queue occupancy is M, then drop the incoming packet, even if it has higher priority.
 *
 *  Note: use SocketPriority instead of SocketIpTosTag, because the SocketIpTosTag is removed during packet transmission,
 *  see Socket and Packet Log_INFO for Detail..
 */


#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/socket.h"
#include "pifo-fast-queue-disc.h"
#include <ns3/string.h>
#include "ns3/myTag.h"

// mutex
//#include <mutex>
//std::mutex mu;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PifoFastQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (PifoFastQueueDisc);

uint32_t getRankTag(Ptr <Packet> packet)
{
    MySimpleTag rankTagTemp;
    packet->PeekPacketTag (rankTagTemp);
    NS_LOG_LOGIC("[PIFO FAST QUEUE DISC] Call Get PKT's SimpleTag :  "  << rankTagTemp.GetRank());
    return rankTagTemp.GetRank();
}

void setRankTag(Ptr <Packet> packet, uint32_t rank)
{
    MySimpleTag rankTag;
    rankTag.SetRank(rank);
    packet->AddPacketTag(rankTag);
    NS_LOG_LOGIC("[PIFO FAST QUEUE DISC] Call Add PKT's SimpleTag :  "  << rank);

}

void
PifoFastQueueDisc::SetPause (uint32_t pause_rank)
{
    PAUSE_PRI_LOCAL = pause_rank;
    m_pausePriority = pause_rank;
    m_ptr_pause_obj->setup (pause_rank,true); // set pauseObj value.
    NS_LOG_LOGIC("[ " << NAME << "]" << " Set Pause with rank" << pause_rank);
}

void
PifoFastQueueDisc::SetUnpause (void)
{
    m_pausePriority = 0;
    m_ptr_pause_obj->setIsPause(false);
    NS_LOG_LOGIC("[ " << NAME << "]" << " Set Unpause");
}
void
PifoFastQueueDisc::TakeSnapShotQueue(void)
{
    NS_LOG_LOGIC("[ " << NAME << "]" << " =============================================");
    NS_LOG_LOGIC("[ " << NAME << "]" << " Take Snapshot");


    for (uint32_t i = 0; i < GetNInternalQueues(); i++) {

        uint32_t queue_occupancy =  GetInternalQueue (i)->GetNPackets ();
        double percentage = (double)queue_occupancy / (double) TOTAL_QUEUE_OCCUPANCY * 100;
        NS_LOG_LOGIC("[ " << NAME << "]" << " Queue # " << i
                    << " Occupancy:" << queue_occupancy
                    << " of Total:" << TOTAL_QUEUE_OCCUPANCY
                    << " Percentage " << percentage

        );

    }

    NS_LOG_LOGIC("[ " << NAME << "]" << " End of Snapshot");
    NS_LOG_LOGIC("[ " << NAME << "]" << " ============================================");


}



TypeId PifoFastQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PifoFastQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<PifoFastQueueDisc> ()
            .AddAttribute ("MaxSize",
                           "The maximum number of packets accepted by this queue disc.",
                           QueueSizeValue (QueueSize ("1000p")),
                           MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                                  &QueueDisc::GetMaxSize),
                           MakeQueueSizeChecker ())
            .AddAttribute ("Name",
                         "Set Current Instance Name.",
                         StringValue ("NoName"),
                         MakeStringAccessor (&PifoFastQueueDisc::NAME),
                         MakeStringChecker ())
            .AddAttribute ("ptr_pause_obj", "pointer for PauseObj(uint32_t m_pauseRank, bool m_isPause) ",
                            PointerValue (),
                           MakePointerAccessor (&PifoFastQueueDisc::m_ptr_pause_obj),
                           MakePointerChecker<PauseObj> ())

            .AddAttribute ("pfc_th_max",
                           "PFC Max Threshold for Pause",
                           UintegerValue (),
                           MakeUintegerAccessor (&PifoFastQueueDisc::PFC_TH_MAX),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("pfc_th_min",
                           "PFC Min Threshold for UnPause",
                           UintegerValue (),
                           MakeUintegerAccessor (&PifoFastQueueDisc::PFC_TH_MIN),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("gpfc_th_max",
                           "GPFC Max Threshold for Pause",
                           UintegerValue (),
                           MakeUintegerAccessor (&PifoFastQueueDisc::GPFC_TH_MAX),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("gpfc_th_min",
                           "GPFC Max Threshold for UnPause",
                            UintegerValue (),
                           MakeUintegerAccessor (&PifoFastQueueDisc::GPFC_TH_MIN),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("max_priority",
                           "GPFC Max Threshold for UnPause",
                            UintegerValue (),
                           MakeUintegerAccessor (&PifoFastQueueDisc::MAX_PRIORITY),
                           MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("gpfc_pause_priority",
                         "GPFC Pause Priority",
                         UintegerValue (),
                         MakeUintegerAccessor (&PifoFastQueueDisc::GPFC_PAUSE_PRI),
                         MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("is_gpfc",
                           "Enable GPFC?",
                            BooleanValue (),
                           MakeBooleanAccessor (&PifoFastQueueDisc::isGPFC),
                            MakeBooleanChecker())
          .AddAttribute ("dequeue_mode",
                         "Dequeue Mode: Priority Queue or Round Robin",
                         StringValue ("PQ"),
                         MakeStringAccessor (&PifoFastQueueDisc::DEQUEUE_MODE),
                         MakeStringChecker ())
          .AddAttribute ("enqueue_mode",
                         "Enqueue Mode: PI(Push-in) or FI(First In)",
                         StringValue ("PI"),
                         MakeStringAccessor (&PifoFastQueueDisc::ENQUEUE_MODE),
                         MakeStringChecker ())

          .AddAttribute ("pause_mode",
                         "Pause Mode: STATIC or DYNAMIC",
                         StringValue ("STATIC"),
                         MakeStringAccessor (&PifoFastQueueDisc::PAUSE_MODE),
                         MakeStringChecker ())
  .AddTraceSource ("PausePriority",
                   "Trace for Current Pause Priority ",
                   MakeTraceSourceAccessor (&PifoFastQueueDisc::m_pausePriority),
                   "ns3::TracedValueCallback::Uint32"
                   )
  ;
  return tid;
}

PifoFastQueueDisc::PifoFastQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS)
{
  NS_LOG_FUNCTION (this);
}

PifoFastQueueDisc::~PifoFastQueueDisc ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO (" =================================");
  NS_LOG_INFO ("[ " << NAME << "]" << " Summary");
  NS_LOG_INFO ("PFC Pause Count" << PAUSE_COUNT_PFC);
  NS_LOG_INFO ("GPFC Pause Count" << PAUSE_COUNT_GPFC);
  NS_LOG_INFO (" =================================");
}

bool
PifoFastQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);


    if (TOTAL_QUEUE_OCCUPANCY >= TOTAL_QUEUE_SIZE)
    {
        NS_LOG_LOGIC ("[ " << NAME << "]" << " Queue disc limit exceeded -- dropping packet");
        DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
        return false;
    }

  uint32_t rank = getRankTag(item->GetPacket ());
  NS_LOG_LOGIC("[ " << NAME << "]" << "Enqueue Packet Rank is  " << rank);

  bool retval = false;

  if(ENQUEUE_MODE == "FI")
  {
      NS_LOG_LOGIC ("[ " << NAME << "]" << " Enqueue with FI mode");
      retval = GetInternalQueue (0)->Enqueue (item);
      NS_LOG_LOGIC ("[ " << NAME << "]" << "Number packets prioirty " << rank << ": " << GetInternalQueue (0)->GetNPackets ()
    << "  Total Occupancy" << TOTAL_QUEUE_OCCUPANCY
    << " Enqueued Packet Priority Average is" << AVG_PRIORITY);
  }
  else if(ENQUEUE_MODE == "PI")
  {
      NS_LOG_LOGIC ("[ " << NAME << "]" << " Enqueue with PI mode");
      retval = GetInternalQueue (rank)->Enqueue (item);
      NS_LOG_LOGIC ("[ " << NAME << "]" << "Number packets prioirty " << rank << ": " << GetInternalQueue (rank)->GetNPackets ()
    << "  Total Occupancy" << TOTAL_QUEUE_OCCUPANCY
    << " Enqueued Packet Priority Average is" << AVG_PRIORITY);
  }

  if (!retval)
    {
        NS_LOG_UNCOND ("[ " << NAME << "]" << "Packet priority " << rank << "enqueue failed. current occupancy is " <<
        TOTAL_QUEUE_OCCUPANCY << "Check the size of the internal queues");
    }
//    mu.lock();
    TOTAL_QUEUE_OCCUPANCY = TOTAL_QUEUE_OCCUPANCY + 1;
// calculate Average Priority,
    AVG_PRIORITY = AVG_PRIORITY + (rank - AVG_PRIORITY) / TOTAL_QUEUE_OCCUPANCY ;

//    mu.unlock();


    if(isGPFC){
        if(!isGPFCPAUSE && (TOTAL_QUEUE_OCCUPANCY >= GPFC_TH_MAX))
        {

            if(PAUSE_MODE == "STATIC") {
                isGPFCPAUSE = true;
                SetPause(GPFC_PAUSE_PRI);
                PAUSE_COUNT_GPFC += 1;
                NS_LOG_LOGIC ("[ " << NAME << "]" << " GPFC PAUSE STATIC No " <<  PAUSE_COUNT_GPFC
                                                     << "--- Time(micro): " << Simulator::Now ().GetMicroSeconds ()
                                   << ", pause priority " <<GPFC_PAUSE_PRI
                                   << "TOTAL_QUEUE_OCCUPANCY : " << TOTAL_QUEUE_OCCUPANCY
                                   << "GPFC_TH_MAX:" <<GPFC_TH_MAX);

                TakeSnapShotQueue();
            }
            else if(PAUSE_MODE == "DYNAMIC") {
                isGPFCPAUSE = true;
                SetPause(int(AVG_PRIORITY));
                PAUSE_COUNT_GPFC += 1;
                NS_LOG_LOGIC ("[ " << NAME << "]" << " GPFC PAUSE DYNAMIC--- " <<  PAUSE_COUNT_GPFC
                                                     << "Time(micro): " << Simulator::Now ().GetMicroSeconds ()
                                   << ", pause priority " <<int(AVG_PRIORITY)
                                   << "TOTAL_QUEUE_OCCUPANCY : " << TOTAL_QUEUE_OCCUPANCY
                                   << "GPFC_TH_MAX:" <<GPFC_TH_MAX);

                TakeSnapShotQueue();
            }
            else{
                NS_LOG_ERROR ("[ " << NAME << "]" << " GPFC PAUSE ERROR--- Unknown Pause Mode: " <<PAUSE_MODE
                                   << ". Expected Value is STATIC or DYNAMIC : ");
            }
        }
    }

    if(!isPFCPAUSE && (TOTAL_QUEUE_OCCUPANCY >= PFC_TH_MAX))
    {
        isPFCPAUSE = true;
        SetPause(PFC_PAUSE_PRI);
        PAUSE_COUNT_PFC += 1;

        NS_LOG_LOGIC ("[ " << NAME << "]" << " PFC PAUSE " <<  PAUSE_COUNT_PFC
                                             << "--- Time: " <<  Simulator::Now ().GetMilliSeconds ()
                           <<"pause priority " <<PFC_PAUSE_PRI
                           << "TOTAL_QUEUE_OCCUPANCY : " << TOTAL_QUEUE_OCCUPANCY
                           << "PFC_TH_MAX:"<<PFC_TH_MAX);
        TakeSnapShotQueue();
    }

    return retval;
}

Ptr<QueueDiscItem>
PifoFastQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item;
//  uint32_t priority = 0;
//  uint32_t dscp = 0;


// In single Node Example, is peek algorithm needed??

// Peek Algorithm.
//   if(isPFCPAUSE || isGPFCPAUSE) {
//       // do peek, return false if the queue is empty, or the head element's priority is lower than pause threshold.
//       NS_LOG_LOGIC("[ " << NAME << "]" << "Queue Pause: is True, Paused packet priority threshold is " << m_ptr_pause_obj->getPauseValue());
//       Ptr<const QueueDiscItem> peek_item;
//       for (uint32_t i = 0; i < GetNInternalQueues (); i++)
//       {
//           if ((peek_item = GetInternalQueue (i)->Peek ()) != 0)
//           {
//               NS_LOG_LOGIC ("[ " << NAME << "]" << "Peeked from Queue " << i << ": " << peek_item);
//               NS_LOG_LOGIC ("[ " << NAME << "]" << "Number packets in Queue " << i << ": " << GetInternalQueue (i)->GetNPackets ());
//
//               //get packet rank
//               uint32_t rank = getRankTag(peek_item->GetPacket ());
//
//               // if the rank is smaller than paused rank,
//               // return false,
//               // else continue to dequeue packet from queue.
//               if(rank >= PAUSE_PRI_LOCAL)
//               {
//                   NS_LOG_LOGIC ("[ " << NAME << "]" << "Peeked element priority(rank) " << rank << "is larger than(or equal) paused priority "
//                                      << PAUSE_PRI_LOCAL );
//                   NS_LOG_LOGIC ("[ " << NAME << "]" << "============ Packet Paused ===========");
//                   return item;
//               }
//           }
//       }
//   }


    if(DEQUEUE_MODE == "RR"){
        item = DoDequeueRR();
    }
    else if(DEQUEUE_MODE == "PQ")
    {
        item = DoDequeuePQ();
    }

    // check unpause condition.
    if(item != nullptr)
    {
        uint32_t rank = getRankTag(item->GetPacket ());
        NS_LOG_LOGIC ("[ " << NAME << "]" << "Average Rank Before : " << AVG_PRIORITY);

        // Calculate Enqueued Packet Average
        // if the queue is empty, reset average to 0
        // else calculate updated average.

        if(TOTAL_QUEUE_OCCUPANCY == 0)
        {
            AVG_PRIORITY = 0;
        }
        else
        {
            AVG_PRIORITY = AVG_PRIORITY - (rank - AVG_PRIORITY) / TOTAL_QUEUE_OCCUPANCY;
        }

        NS_LOG_LOGIC ("[ " << NAME << "]" << "Dequeued Packet Priority: " << rank
                      << "\n Average Rank After :" << AVG_PRIORITY );

        NS_LOG_LOGIC("[ " << NAME << "]" << "Start to Check UnPause Condition");
        if(isGPFCPAUSE && !isPFCPAUSE) // GPFC Pause Only
        {
            // if current queue occupancy is smaller than GPFC Min Threshold, then
            // set unpause
            if(TOTAL_QUEUE_OCCUPANCY <= GPFC_TH_MIN)
            {
                isGPFCPAUSE = false;
                SetUnpause();
                NS_LOG_LOGIC("[ " << NAME << "]" << "SetUnpause by GPFC TH MIN, Queue Occupancy at " << TOTAL_QUEUE_OCCUPANCY);
                TakeSnapShotQueue();
            }
        } else if (isPFCPAUSE) // if PFC pause
        {
            // if current queue occupancy is smaller than PFC Min Threshold, then
            // set unpause
            if(TOTAL_QUEUE_OCCUPANCY <= PFC_TH_MIN)
            {
                isPFCPAUSE = false;
                isGPFCPAUSE = false;
                SetUnpause();
                NS_LOG_LOGIC("[ " << NAME << "]" << "SetUnpause by PFC TH MIN, Queue Occupancy at " << TOTAL_QUEUE_OCCUPANCY);
                TakeSnapShotQueue();
            }
        }
    }else
    {
        NS_LOG_LOGIC("[ " << NAME << "]" << "ByPass Check Pause Condition due to nullptr");
    }

      return item;
}
Ptr<QueueDiscItem>
PifoFastQueueDisc::DoDequeuePQ (void)
{
    NS_LOG_FUNCTION (this);
    Ptr<QueueDiscItem> item;
    for (uint32_t i = 0; i < GetNInternalQueues(); i++) {
        if ((item = GetInternalQueue(i)->Dequeue()) != 0) {
//              mu.lock();
            TOTAL_QUEUE_OCCUPANCY = TOTAL_QUEUE_OCCUPANCY - 1;
//              mu.unlock();

            NS_LOG_LOGIC("[ " << NAME << "]" << "Total Occupancy is " << TOTAL_QUEUE_OCCUPANCY);
            NS_LOG_LOGIC("[ " << NAME << "]" << "Popped from priority " << i << ": " << item);
            NS_LOG_LOGIC(
                    "[ " << NAME << "]" << "Number packets priority " << i << ": " << GetInternalQueue(i)->GetNPackets()
                         << "  Total Queue Occupancy:" << TOTAL_QUEUE_OCCUPANCY);

            uint32_t rank = getRankTag(item->GetPacket());
            NS_LOG_LOGIC("[ " << NAME << "]" << "Dequeued Packet Rank is  " << rank);

            return item;
        }
    }
    NS_LOG_LOGIC ("Queue empty");
    return item;
}

Ptr<QueueDiscItem>
PifoFastQueueDisc::DoDequeueRR (void)
{
    NS_LOG_FUNCTION (this << "Next dequeue rr index is " << DEQUEUE_RR_NEXT_INDEX);
    Ptr<QueueDiscItem> item;

    for (uint32_t i = DEQUEUE_RR_NEXT_INDEX; i < GetNInternalQueues(); i++) {
        if ((item = GetInternalQueue(i)->Dequeue()) != 0) {
//              mu.lock();
            TOTAL_QUEUE_OCCUPANCY = TOTAL_QUEUE_OCCUPANCY - 1;
//              mu.unlock();

            NS_LOG_UNCOND("[ " << NAME << "]" << "Total Occupancy is " << TOTAL_QUEUE_OCCUPANCY);
            NS_LOG_LOGIC("[ " << NAME << "]" << "Popped from priority " << i << ": " << item);
            NS_LOG_LOGIC(
                    "[ " << NAME << "]" << "Number packets priority " << i << ": " << GetInternalQueue(i)->GetNPackets()
                         << "  Total Queue Occupancy:" << TOTAL_QUEUE_OCCUPANCY);

            uint32_t rank = getRankTag(item->GetPacket());
            NS_LOG_UNCOND("[ " << NAME << "]" << "Dequeued Packet Rank is  " << rank);

            DEQUEUE_RR_NEXT_INDEX = (i + 1) % GetNInternalQueues();
            return item;
        }

    }
    // start from head if not found.
    for (uint32_t i = 0; i < DEQUEUE_RR_NEXT_INDEX; i++) {
        if ((item = GetInternalQueue(i)->Dequeue()) != 0) {
//              mu.lock();
            TOTAL_QUEUE_OCCUPANCY = TOTAL_QUEUE_OCCUPANCY - 1;
//              mu.unlock();

            NS_LOG_UNCOND("[ " << NAME << "]" << "Total Occupancy is " << TOTAL_QUEUE_OCCUPANCY);
            NS_LOG_LOGIC("[ " << NAME << "]" << "Popped from priority " << i << ": " << item);
            NS_LOG_LOGIC(
                    "[ " << NAME << "]" << "Number packets priority " << i << ": " << GetInternalQueue(i)->GetNPackets()
                         << "  Total Queue Occupancy:" << TOTAL_QUEUE_OCCUPANCY);

            uint32_t rank = getRankTag(item->GetPacket());
            NS_LOG_UNCOND("[ " << NAME << "]" << "Dequeued Packet Rank is  " << rank);

            DEQUEUE_RR_NEXT_INDEX = (i + 1) % GetNInternalQueues();
            return item;
        }
    }

    NS_LOG_LOGIC ("Queue empty, Reset last dequeue rr index to 0");
    DEQUEUE_RR_NEXT_INDEX = 0;
    return item;
}

Ptr<const QueueDiscItem>
PifoFastQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item;

  for (uint32_t i = 0; i < GetNInternalQueues (); i++)
    {
      if ((item = GetInternalQueue (i)->Peek ()) != 0)
        {
            NS_LOG_LOGIC ("[ " << NAME << "]" << "Peeked from band " << i << ": " << item);
            NS_LOG_LOGIC ("[ " << NAME << "]" << "Number packets band " << i << ": " << GetInternalQueue (i)->GetNPackets ());
          return item;
        }
    }

  NS_LOG_LOGIC ("[ " << NAME << "]" << "Queue empty");
  return item;
}

bool
PifoFastQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("[ " << NAME << "]" << "PifoFastQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () != 0)
    {
      NS_LOG_ERROR ("[ " << NAME << "]" << "PifoFastQueueDisc needs no packet filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create 3 DropTail queues with GetLimit() packets each
      ObjectFactory factory;
      factory.SetTypeId ("ns3::DropTailQueue<QueueDiscItem>");
      factory.Set ("MaxSize", QueueSizeValue (GetMaxSize ()));

        for (uint32_t i = 0; i < MAX_PRIORITY+1; i++){
            NS_LOG_LOGIC ("[ " << NAME << "]" << "Add Internal Queue No." << i);
            AddInternalQueue (factory.Create<InternalQueue> ());
        }
    }

//  if (GetNInternalQueues () != 3)
//    {
//      NS_LOG_ERROR ("PifoFastQueueDisc needs 3 internal queues");
//      return false;
//    }

//  if (GetInternalQueue (0)-> GetMaxSize ().GetUnit () != QueueSizeUnit::PACKETS ||
//      GetInternalQueue (1)-> GetMaxSize ().GetUnit () != QueueSizeUnit::PACKETS ||
//      GetInternalQueue (2)-> GetMaxSize ().GetUnit () != QueueSizeUnit::PACKETS)
//    {
//      NS_LOG_ERROR ("[ " << NAME << "]" << "PifoFastQueueDisc needs 3 internal queues operating in packet mode");
//      return false;
//    }
//
//  for (uint8_t i = 0; i < 2; i++)
//    {
//      if (GetInternalQueue (i)->GetMaxSize () < GetMaxSize ())
//        {
//          NS_LOG_ERROR ("[ " << NAME << "]" << "The capacity of some internal queue(s) is less than the queue disc capacity");
//          return false;
//        }
//    }

  return true;
}

void
PifoFastQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

uint32_t
PifoFastQueueDisc::GetTotalQueueOccupancy(void){
    return TOTAL_QUEUE_OCCUPANCY;
}

} // namespace ns3
