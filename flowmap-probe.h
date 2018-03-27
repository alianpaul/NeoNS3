#ifndef FLOWMAP_PROBE_H
#define FLOWMAP_PROBE_H

#include "neo-probe.h"

namespace ns3
{

  class FlowMapProbe : public NeoProbe
  {
  public:
    FlowMapProbe(Ptr<Node> node);
    virtual ~FlowMapProbe   ();
    static TypeId GetTypeId (void);

  public:
    void ForwardLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface);
  };

}

#endif
