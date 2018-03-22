#ifndef NEO_FLOW_GENERATOR_H
#define NEO_FLOW_GENERATOR_H

#include <vector>

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

namespace ns3
{

  class NodeContainer;
  class Ipv4Address;
  class Node;
  class ExponentialRandomVariable;

  class NeoFlowGenerator : public Object
  {
  public:
    static TypeId GetTypeId(void);
    
    NeoFlowGenerator();
    
    void Initialize(std::vector<NodeContainer> podHostNodes);

  private:
    void SetupParameters();
    void SetupApplications();

    void SetupFlowsOriginFrom(int iSrcSub, int iSrcHst);
    void SetupTestFlowsOriginFrom(int iSrcSub, int iSrcHst);

    void SetupUDPFlow(Ptr<Node> srcNode, Ptr<Node> dstNode, 
		      uint64_t bps, uint16_t port,
		      const Time& startTime, const Time& endTime);
    
    int32_t m_numExpectedFlowsPerSwtch;
    int32_t m_numInterPodFlowsPerHostPerInterval;
    int32_t m_numIntroPodFlowsPerHostPerInterval;

    int16_t m_numHostPerPod;
    int16_t m_numPod;

    Time                           m_intervalTime;    //Attribute
    Ptr<ExponentialRandomVariable> m_startTimeOffset;
    int16_t                        m_numVirtualInterval;  //Attribute
    int16_t                        m_idxVirtualInterval;


    std::vector<NodeContainer>          m_podHostNodes;
    std::vector<std::vector<uint16_t> > m_nextDstPort;

    DataRate                       m_bpsHst;  //Attribute
    Ptr<ExponentialRandomVariable> m_elephantBps;
    Ptr<ExponentialRandomVariable> m_mouseBps;
  };

}

#endif
