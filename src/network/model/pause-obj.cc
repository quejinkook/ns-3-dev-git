#include "pause-obj.h"
namespace ns3 {

    PauseObj::PauseObj()
            : m_pauseRank(0),
              m_isPause(false),
              m_nodeName("Dummy_Node")
    {}
    PauseObj::PauseObj(std::string name)
            : m_pauseRank(0),
              m_isPause(false),
              m_nodeName(name)
    {}

    PauseObj::~PauseObj(){}

    TypeId
    PauseObj::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::PauseObj")
                .SetGroupName("Network")
                .AddConstructor<PauseObj> ()
        ;
        return tid;
    }
    TypeId
    PauseObj::GetInstanceTypeId (void) const
    {
        return GetTypeId ();
    }

    void PauseObj::setName(std::string name){
        m_nodeName = name;
    }

    void
    PauseObj::setup(uint32_t pause_value, bool is_pause)
    {
        m_pauseRank = pause_value;
        m_isPause = is_pause;
    }

    bool
    PauseObj::getIsPause() {
        return m_isPause;
    }

    uint32_t
    PauseObj::getPauseValue() {
        return m_pauseRank;
    }

    void
    PauseObj::setIsPause(bool is_pause) {
        m_isPause = is_pause;
    }

    void
            PauseObj::setPauseValue(uint32_t pause_value){
    m_pauseRank = pause_value;
    }

    std::string PauseObj::getName() {
        return m_nodeName;
    }

}