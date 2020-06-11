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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 *           Tom Henderson <tomhend@u.washington.edu>
 */

#ifndef PFIFO_FAST_H
#define PFIFO_FAST_H

#include "ns3/queue-disc.h"
#include "ns3/object.h"
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/pause-obj.h"

#include <ns3/string.h>

namespace ns3 {
class PifoFastQueueDisc;
/**
 * \ingroup traffic-control
 *
 * Linux pfifo_fast is the default priority queue enabled on Linux
 * systems. Packets are enqueued in three FIFO droptail queues according
 * to three priority bands based on the packet priority.
 *
 * The system behaves similar to three ns3::DropTail queues operating
 * together, in which packets from higher priority bands are always
 * dequeued before a packet from a lower priority band is dequeued.
 *
 * The queue disc capacity, i.e., the maximum number of packets that can
 * be enqueued in the queue disc, is set through the limit attribute, which
 * plays the same role as txqueuelen in Linux. If no internal queue is
 * provided, three DropTail queues having each a capacity equal to limit are
 * created by default. User is allowed to provide queues, but they must be
 * three, operate in packet mode and each have a capacity not less
 * than limit. No packet filter can be provided.
 */
class PifoFastQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief PfifoFastQueueDisc constructor
   *
   * Creates a queue with a depth of 1000 packets per band by default
   */
  PifoFastQueueDisc ();

  uint32_t getQueueOccupancy(void);
  virtual ~PifoFastQueueDisc();

  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded

private:
//  /**
//   * Priority to band map. Values are taken from the prio2band array used by
//   * the Linux pfifo_fast queue disc.
//   */
//  static const uint32_t prio2band[16];


    uint32_t TOTAL_QUEUE_OCCUPANCY  = 0;
    uint32_t TOTAL_QUEUE_SIZE = 1024; // in pkt count
    uint32_t MAX_PRIORITY  = 64; // priority form 1 to 63

    uint32_t PFC_TH_MAX  = 700; // pause threshold.
    uint32_t PFC_TH_MIN  = 128; // unpause threshold
    uint32_t PFC_PAUSE_PRI = 0;

    uint32_t GPFC_TH_MAX  = 500; // pause threshold.
    uint32_t GPFC_TH_MIN  = 128; // unpause threshold
    uint32_t GPFC_PAUSE_PRI  = MAX_PRIORITY / 2; // pause priority
    bool isGPFC = true; // true for enable gpfc.

    Ptr<PauseObj> m_ptr_pause_obj; // pause object share among sender and current queue.
    std::string NAME; // Name for logging convention.

    bool isPFCPAUSE = false; // PFC PAUSE
    bool isGPFCPAUSE = false; // GPFC PAUSE

    uint32_t PAUSE_PRI_LOCAL = 0;
    uint32_t DEQUEUE_RR_NEXT_INDEX = 0;
    std:: string DEQUEUE_MODE = "PQ";
    std:: string ENQUEUE_MODE = "PI"; // PI(Push-in), FI(First-in)

    std::string PAUSE_MODE="STATIC";
    double AVG_PRIORITY = 0;


    uint32_t PAUSE_COUNT_PFC = 0;
    uint32_t PAUSE_COUNT_GPFC = 0;


    // trace pause rank
    TracedValue<uint32_t> m_pausePriority = 0;

    virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
    virtual Ptr<QueueDiscItem> DoDequeue (void);
    virtual Ptr<QueueDiscItem> DoDequeuePQ (void); // priority queue
    virtual Ptr<QueueDiscItem> DoDequeueRR (void); // round robin
//    virtual void CheckUnPause(void);

    virtual Ptr<const QueueDiscItem> DoPeek (void);
    virtual bool CheckConfig (void);
    virtual void InitializeParams (void);

    virtual void SetPause(uint32_t pause_rank);
    virtual void SetUnpause();

    virtual void TakeSnapShotQueue();

    };

} // namespace ns3

#endif /* PFIFO_FAST_H */
