#ifndef PAUSEOBJ_H
#define PAUSEOBJ_H

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"

namespace ns3 {
    class PauseObj : public Object{
    public:
        PauseObj();
        PauseObj(std::string name);
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        virtual TypeId GetInstanceTypeId(void) const;

        virtual ~PauseObj();

        void setup(uint32_t pause_value, bool is_pause);

        uint32_t getPauseValue(void);

        bool getIsPause(void);
        std::string getName(void);
        void setName(std::string name);
        void setPauseValue(uint32_t pause_value);

        void setIsPause(bool is_pause);

    private:

        uint32_t    m_pauseRank;
        bool        m_isPause;
        std::string m_nodeName;
    };
}
#endif /* TAG_H */