#ifndef MYTAG_H
#define MYTAG_H

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/tag.h"

namespace ns3 {

    class MySimpleTag : public Tag {
    public:
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        virtual TypeId GetInstanceTypeId(void) const;

        virtual uint32_t GetSerializedSize(void) const;

        virtual void Serialize(TagBuffer i) const;

        virtual void Deserialize(TagBuffer i);

        void SetRank(uint32_t rank);
        uint32_t GetRank();

        uint32_t GetId();
        void SetId(uint32_t id);

        uint32_t GetNo();
        void SetNo(uint32_t num);

        uint64_t GetTimeSent();
        void SetTimeSent(uint64_t time);



        void Print(std::ostream &os) const;

//        void PrintRank(void);

    private:
        uint32_t m_rank; // rank value
        uint32_t m_id;   // app/flow id
        uint32_t m_pkt_no; // pkt number,
        uint64_t m_time_sent_in_nano;  //first sent time
        uint64_t m_time_receive_in_nano; // receive time
    };

    NS_OBJECT_ENSURE_REGISTERED (MySimpleTag);
}

#endif /* TAG_H */