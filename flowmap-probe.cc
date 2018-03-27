#include "flowmap-probe.h"

#include "ns3/node.h"
#include "ns3/log.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("FlowMapProbe");
  NS_OBJECT_ENSURE_REGISTERED(FlowMapProbe);

  TypeId 
  FlowMapProbe::GetTypeId ()
  {
    static TypeId tid = TypeId("ns3::FlowMapProbe")
      .SetParent<NeoProbe> ()
      .SetGroupName ("NeoFlowMonitor");

    return tid;
  }

  FlowMapProbe::FlowMapProbe(Ptr<Node> node)
    : NeoProbe(node)
  {
  }

  FlowMapProbe::~FlowMapProbe()
  {
  }

  void
  FlowMapProbe::ForwardLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface)
  {
    //1. Update real flow stats;
    UpdateRealFlowStats (ipHeader, ipPayload);
    
    return;
  }

}
