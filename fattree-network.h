#ifndef FATTREE_NETWORK_H
#define FATTREE_NETWORK_H

#include <vector>

#include "ns3/object.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"

namespace ns3 {
  
  class FatTreeNetwork : public Object
  {
  public:
    static TypeId GetTypeId(void);

    FatTreeNetwork();
    
    void Initialize();

    std::vector<NodeContainer> GetHostNodes() const;

  private:
    void SetupNodes();  
    void SetupLinks();
    void SetupGlobalRoutingTable();
    
    int16_t m_numHostPerPod;
    int16_t m_numPod;
    int16_t m_numCore;

    bool    m_printRoutingTablePredicate;
    bool    m_asciiTracePredicate;
    
    std::vector<NodeContainer>  m_podHostNodes;
    NodeContainer               m_podSwtchNodes;
    NodeContainer               m_coreSwtchNodes;
    
};

}

#endif
