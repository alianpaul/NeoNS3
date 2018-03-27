#ifndef FLOWRADAR_PROBE_H
#define FLOWRADAR_PROBE_H

#include "neo-probe.h"

namespace ns3
{

  class FlowRadarProbe : public NeoProbe
  {
  public:
    FlowRadarProbe (Ptr<Node> node);
    virtual ~FlowRadarProbe ();
    static TypeId GetTypeId (void);

  public:
    void ForwardLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface);
  };

}

#endif
