#include "myTag.h"

namespace ns3 {
void
MySimpleTag::Serialize (TagBuffer i) const
{
    i.WriteU32 (m_rank);
}
void
MySimpleTag::Deserialize (TagBuffer i)
{

    m_rank = i.ReadU32 ();
}

TypeId
MySimpleTag::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::PifoSimpleTag")
            .SetParent<Tag> ()
            .SetGroupName("Network")
            .AddConstructor<MySimpleTag> ()
    ;
    return tid;
}
TypeId
MySimpleTag::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

uint32_t
MySimpleTag::GetSerializedSize (void) const
{
    return 4;
}

uint32_t
MySimpleTag::GetRank (void)
{
    return m_rank;
}
void
MySimpleTag::SetRank (uint32_t rank)
{
    m_rank = rank;
}
//void
//MySimpleTag::PrintRank (void)
//{
//    NS_LOG_INFO("[SimpleTag] Print => Rank is " << m_rank);
//}
void
    MySimpleTag::Print (std::ostream &os) const
{
    os << "SimpleTag Rank=" << m_rank ;
}

}