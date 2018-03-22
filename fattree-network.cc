#include "fattree-network.h"

#include <string>

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/boolean.h"

#include "ns3/point-to-point-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/trace-helper.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("FatTreeNetwork");
  NS_OBJECT_ENSURE_REGISTERED(FatTreeNetwork);

  TypeId
  FatTreeNetwork::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::FatTreeNetwork")
      .SetParent<Object>()
      .SetGroupName("NeoFlowMonitor")
      .AddConstructor<FatTreeNetwork>()
      .AddAttribute("NumOfHostPerPod",
		    "The num of hosts node in each pod",
		    IntegerValue(20),
		    MakeIntegerAccessor(&FatTreeNetwork::m_numHostPerPod),
		    internal::MakeIntegerChecker(2, 20, "int16_t"))
      .AddAttribute("NumOfPod",
		    "The num of pod",
		    IntegerValue(8),
		    MakeIntegerAccessor(&FatTreeNetwork::m_numPod),
		    internal::MakeIntegerChecker(2, 8, "int16_t"))
      .AddAttribute("NumOfCore",
		    "The num of core swtches",
		    IntegerValue(1),
		    MakeIntegerAccessor(&FatTreeNetwork::m_numCore),
		    internal::MakeIntegerChecker(1, 1, "int16_t"))
      .AddAttribute("PrintRoutingTable",
		    "Set true to turn on routing table",
		    BooleanValue(false),
		    MakeBooleanAccessor(&FatTreeNetwork::m_printRoutingTablePredicate),
		    MakeBooleanChecker())
      .AddAttribute("AsciiTrace",
		    "Set true to turn on AsciiTrace",
		    BooleanValue(false),
		    MakeBooleanAccessor(&FatTreeNetwork::m_asciiTracePredicate),
		    MakeBooleanChecker());
    return tid;
  }
  

  FatTreeNetwork::FatTreeNetwork() 
  {
  }

  void 
  FatTreeNetwork::Initialize()
  {
    m_podHostNodes.resize(m_numPod);

    SetupNodes();

    SetupLinks();

    SetupGlobalRoutingTable();
  }

  void
  FatTreeNetwork::SetupNodes()
  {
    NS_LOG_DEBUG("===Setup nodes and internet stack===");
    
    NS_LOG_DEBUG("Pods : " << m_numPod);
    NS_LOG_DEBUG("Hosts per Pod : " << m_numHostPerPod);
    NS_LOG_DEBUG("Core : " << m_numCore);
     //Create nodes in each sub network;
    InternetStackHelper internetStack;

    for(int iPod = 0; iPod < m_numPod; ++iPod) 
      {
	m_podHostNodes[iPod].Create(m_numHostPerPod);
	internetStack.Install(m_podHostNodes[iPod]);
      }

    //Create edge switches' node
    m_podSwtchNodes.Create(m_numPod);
    internetStack.Install(m_podSwtchNodes);

    //Create core switches' node
    m_coreSwtchNodes.Create(m_numCore);
    internetStack.Install(m_coreSwtchNodes);

  }

  void
  FatTreeNetwork::SetupLinks()
  {
    NS_LOG_DEBUG("===Setup p2p links===");
    
    PointToPointHelper p2p;
    Ipv4AddressHelper  ipv4Addr;
    ipv4Addr.SetBase("10.0.0.0", "255.255.255.0");

    for(int iPod = 0; iPod < m_numPod; ++iPod)
      {
	NS_LOG_DEBUG("Links in Pod " << iPod);
	NodeContainer&      iPodHostNodes = m_podHostNodes[iPod];
	Ptr<Node>           iPodEdgeSwtchNode = m_podSwtchNodes.Get(iPod);

	//Hosts to Edge swtch
	for(int iH = 0; iH < m_numHostPerPod; ++iH)
	  {
	    Ptr<Node> iHostNode = iPodHostNodes.Get(iH);
	    NetDeviceContainer dHdSEdge = p2p.Install(NodeContainer(iHostNode, iPodEdgeSwtchNode));    
	    ipv4Addr.Assign(dHdSEdge); ipv4Addr.NewNetwork();
	  }

	//Edge swtch to Core swtch
	NetDeviceContainer dSEdgeSCore = p2p.Install(NodeContainer(iPodEdgeSwtchNode, m_coreSwtchNodes.Get(0)));
	ipv4Addr.Assign(dSEdgeSCore); ipv4Addr.NewNetwork();
      }

    if(m_asciiTracePredicate)
      {
	AsciiTraceHelper ascii;
	p2p.EnableAsciiAll(ascii.CreateFileStream("neo-flow.tr"));
      }   
  }

  void
  FatTreeNetwork::SetupGlobalRoutingTable()
  {
    NS_LOG_DEBUG("===Setup Routing Tables===");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    /*Output Routing Table Test*/
    if(m_printRoutingTablePredicate)
      {
	Ptr<OutputStreamWrapper> os = Create<OutputStreamWrapper>(&std::cout);
	Ipv4GlobalRoutingHelper::PrintRoutingTableAllAt(Seconds(0.), os);
      }
  }

  std::vector<NodeContainer> 
  FatTreeNetwork::GetHostNodes() const
  {
    return m_podHostNodes;
  }

  
}
