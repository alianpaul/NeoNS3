#include "flowradar-probe.h"

#include "ns3/node.h"
#include "ns3/log.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("FlowRadarProbe");
  NS_OBJECT_ENSURE_REGISTERED(FlowRadarProbe);

  TypeId 
  FlowRadarProbe::GetTypeId()
  {

    static TypeId tid = TypeId("ns3::FlowRadarProbe")
      .SetParent<NeoProbe> ()
      .SetGroupName ("NeoFlowMonitor");

    return tid;

  }

  FlowRadarProbe::FlowRadarProbe (Ptr<Node> node) 
    : NeoProbe(node)
  {
    NS_LOG_FUNCTION(this);
  }

  FlowRadarProbe::~FlowRadarProbe ()
  {
  }

  void
  FlowRadarProbe::ForwardLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface)
  {
    NS_LOG_FUNCTION("radar forward");
    return;
  }

}
