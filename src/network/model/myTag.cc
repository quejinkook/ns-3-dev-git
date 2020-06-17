#include "myTag.h"

namespace ns3 {
void
MySimpleTag::Serialize (TagBuffer i) const
{
    i.WriteU32 (m_rank);
    i.WriteU32 (m_pkt_no);
    i.WriteU32 (m_id);
    i.WriteU64 (m_time_sent_in_nano);
    i.WriteU64 (m_time_receive_in_nano);

}
void
MySimpleTag::Deserialize (TagBuffer i)
{

    m_rank = i.ReadU32 ();
    m_pkt_no = i.ReadU32();
    m_id = i.ReadU32();
    m_time_sent_in_nano = i.ReadU64();
    m_time_receive_in_nano = i.ReadU64();
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
    return 28;
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

uint32_t
MySimpleTag::GetId(){
    return m_id;
}
void
MySimpleTag::SetId(uint32_t id){
    m_id = id;
}

uint32_t
MySimpleTag::GetNo(){
    return m_pkt_no;
}

void MySimpleTag::SetNo(uint32_t num){
    m_pkt_no = num;
}

uint64_t
MySimpleTag::GetTimeSent(){
    return m_time_sent_in_nano;
}
void
MySimpleTag::SetTimeSent(uint64_t time){
    m_time_sent_in_nano = time;
}



void
    MySimpleTag::Print (std::ostream &os) const
{
    os << "SimpleTag Rank=" << m_rank ;
}

}