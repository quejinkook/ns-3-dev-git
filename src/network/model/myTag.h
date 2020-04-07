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

        void Print(std::ostream &os) const;

//        void PrintRank(void);

    private:
        uint32_t m_rank; //!< protocol number
    };

    NS_OBJECT_ENSURE_REGISTERED (MySimpleTag);
}

#endif /* TAG_H */