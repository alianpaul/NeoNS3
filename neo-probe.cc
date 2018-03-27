#include "neo-probe.h"

#include "ns3/node.h"
#include "ns3/log.h"

#include <fstream>
#include <sstream>

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("NeoProbe");
  NS_OBJECT_ENSURE_REGISTERED(NeoProbe);

  void 
  FlowField::InitFromPacket(const Ipv4Header& ipHeader, Ptr<const Packet> ipPayload)
  {
    ipv4srcip = ipHeader.GetSource().Get();
    ipv4dstip = ipHeader.GetDestination().Get();
    ipv4prot  = ipHeader.GetProtocol ();
    if (ipv4prot == UdpL4Protocol::PROT_NUMBER)
      {
	UdpHeader udpHeader;
	ipPayload->PeekHeader (udpHeader);
	srcport = udpHeader.GetSourcePort ();
	dstport = udpHeader.GetDestinationPort ();
      }
    else if (ipv4prot == TcpL4Protocol::PROT_NUMBER)
      {
	TcpHeader tcpHeader;
	ipPayload->PeekHeader (tcpHeader);
	srcport = tcpHeader.GetSourcePort ();
	dstport = tcpHeader.GetDestinationPort ();
      }
    else
      {
	NS_FATAL_ERROR("Protocol not supported");
      }
  }

  bool
  operator== (const FlowField& lhs, const FlowField& rhs)
  {
    return lhs.ipv4srcip == rhs.ipv4srcip 
        && lhs.ipv4dstip == rhs.ipv4dstip
        && lhs.ipv4prot  == rhs.ipv4prot
        && lhs.srcport   == rhs.srcport
        && lhs.dstport   == rhs.dstport;
  }

  std::ostream& 
  operator<< (std::ostream& os, const FlowField& flow)
  {
    Ipv4Address srcip (flow.ipv4srcip), dstip (flow.ipv4dstip);
    std::string prot = (flow.ipv4prot == UdpL4Protocol::PROT_NUMBER) ? "UDP" : "TCP" ;

    os << srcip << " " << dstip << " " << prot << " " << flow.srcport << " " << flow.dstport ;
    
    return os;
  }

  std::ostream& 
  operator<< (std::ostream& os, const PckByteField& pckbyte)
  {
    os << "PckCnt " << pckbyte.pckcnt << " ByteCnt " << pckbyte.bytecnt ;

    return os;
  }

  TypeId 
  NeoProbe::GetTypeId (void)
  {
    
    static TypeId tid = TypeId("ns3::NeoProbe")
      .SetParent<Object> ()
      .SetGroupName ("NeoFlowMonitor");

    return tid;
  }

  NeoProbe::NeoProbe (Ptr<Node> node)
  {
    NS_LOG_FUNCTION(this << node->GetId());
    m_ipv4 = node->GetObject<Ipv4L3Protocol> ();

    if (!m_ipv4->TraceConnectWithoutContext ("UnicastForward",
					     MakeCallback (&NeoProbe::ForwardLogger, Ptr<NeoProbe> (this))))
      {
	NS_FATAL_ERROR ("UnicastForward Trace Fail");
      }

    m_nodeId = node->GetId();
  }
  
  NeoProbe::~NeoProbe ()
  {
  }

  void
  NeoProbe::UpdateRealFlowStats (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload)
  {
    
    FlowField flow; flow.InitFromPacket(ipHeader, ipPayload);
    FlowStatContainerI it = m_realFlowStats.find (flow);
    if (it == m_realFlowStats.end())
      {
	m_realFlowStats[flow] = PckByteField();
      }

    m_realFlowStats[flow].pckcnt  += 1;
    m_realFlowStats[flow].bytecnt += ipHeader.GetPayloadSize();

    NS_LOG_DEBUG(flow <<" "<< m_realFlowStats[flow]);

  }

  void
  NeoProbe::PrintRealFlowStats (std::string fileNameSuffix) const
  {
    std::stringstream ss;       ss << m_nodeId << "-" << fileNameSuffix;
    std::string       filename; ss >> filename;
    std::ofstream     file (filename.c_str());
    NS_ASSERT(file);
    
    file << "TotalFlowCnt " << m_realFlowStats.size() << std::endl;
    for(FlowStatContainerCI ci = m_realFlowStats.cbegin(); ci != m_realFlowStats.cend(); ++ci)
      {
	file << ci->first << " " << ci->second << std::endl;
      }
  }
  
}
