#include "neo-flow-generator.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"

//Debug
uint16_t port = 1;

namespace ns3
{
  //Helper functions declarations
  Ipv4Address GetIpv4Addr(Ptr<Node> hstNode);

}

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("NeoFlowGenerator");
  NS_OBJECT_ENSURE_REGISTERED(NeoFlowGenerator);

  TypeId NeoFlowGenerator::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::NeoFlowGenerator")
      .SetParent<Object>()
      .SetGroupName("NeoFlowMonitor")
      .AddConstructor<NeoFlowGenerator>()
      .AddAttribute("NumOfExpectedFlowsPerSwtch",
		    "The total flow count go through each switch",
		    IntegerValue(1000),
		    MakeIntegerAccessor(&NeoFlowGenerator::m_numExpectedFlowsPerSwtch),
		    MakeIntegerChecker<int32_t>())
      .AddAttribute("IntervalTime",
		    "The time of virtual interval",
		    TimeValue(MilliSeconds(50)),
		    MakeTimeAccessor(&NeoFlowGenerator::m_intervalTime),
		    MakeTimeChecker())
      .AddAttribute("DataRate",
		    "The total data rate of all applications on a host",
		    DataRateValue(DataRate("10Gbps")),
		    MakeDataRateAccessor(&NeoFlowGenerator::m_bpsHst),
		    MakeDataRateChecker())
      .AddAttribute("VirtualInterval",
		    "The origin simultion is divided into several consecutive virtual intervals",
		    IntegerValue(1),
		    MakeIntegerAccessor(&NeoFlowGenerator::m_numVirtualInterval),
		    MakeIntegerChecker<int16_t>());

    return tid;
  }

  NeoFlowGenerator::NeoFlowGenerator()
  {    
  }

  void
  NeoFlowGenerator::Initialize(std::vector<NodeContainer> podHostNodes)
  {

    m_numPod        = podHostNodes.size();
    m_numHostPerPod = podHostNodes[0].GetN();
    m_podHostNodes  = podHostNodes;

    SetupParameters();

    SetupApplications();
  }

  void 
  NeoFlowGenerator::SetupParameters()
  {
    NS_LOG_DEBUG("===Setup flows's parameters===");

    m_nextDstPort.resize(m_numPod);
    for(int iPod = 0; iPod < m_numPod; ++iPod)
      {
	m_nextDstPort[iPod].resize(m_numHostPerPod, 1); //PORT MUST START FROM 1
      }

    NS_LOG_DEBUG("Pods(n) :" << m_numPod);
    NS_LOG_DEBUG("Hosts per pod(n) : " << m_numHostPerPod);
    NS_LOG_DEBUG("Expected number of flows per switch : " << m_numExpectedFlowsPerSwtch);
    NS_LOG_DEBUG("Total bps of all apps on a hst : " << m_bpsHst.GetBitRate());
    NS_LOG_DEBUG("Number of simulation virtual intervals : " << m_numVirtualInterval);
    NS_LOG_DEBUG("Interval time time(ms) : " << m_intervalTime.GetMilliSeconds());
    //NS_LOG_DEBUG(m_intervalTime.GetDouble());

    /*Calculate the number of flows that each host should generate,
     *These flows are devided into 2 categories: Intro subnet, Inter subnet.
     *Intro subnet flows' destinations are every other hosts in the same subnet.
     *Inter subnet flows' destinations are the hosts in the opposite subnets.
     *e.g subnet0/1/2/3' opposite subnets are subnet4, subnet5, subnet6, subnet7,vice versa.
     */

    m_idxVirtualInterval = 0;
    m_startTimeOffset = CreateObject<ExponentialRandomVariable>();
    m_startTimeOffset->SetAttribute("Mean", DoubleValue(m_intervalTime.GetDouble() / 8.));
    m_startTimeOffset->SetAttribute("Bound", DoubleValue(m_intervalTime.GetDouble() / 4.));

    int32_t numInterPodFlowsPerPod = (1.0/m_numPod) * m_numExpectedFlowsPerSwtch;
    NS_LOG_DEBUG("Num of inter Pod flows(/subnet) : " << numInterPodFlowsPerPod);
    int32_t numInterPodFlowsPerHost = numInterPodFlowsPerPod / m_numHostPerPod;
    NS_LOG_DEBUG("Num of inter Pod flows(/host)  : " << numInterPodFlowsPerHost);
    m_numInterPodFlowsPerHostPerInterval = numInterPodFlowsPerHost / m_numVirtualInterval;
    NS_LOG_DEBUG("Num of inter Pod flows(/host/interval) : " << m_numInterPodFlowsPerHostPerInterval);

    int32_t numIntroPodFlowsPerPod = m_numExpectedFlowsPerSwtch - 2 * numInterPodFlowsPerPod;
    NS_LOG_DEBUG("Num of intro Pod flows(/subnet) : " << numIntroPodFlowsPerPod);
    int32_t numIntroPodFlowsPerHost = numIntroPodFlowsPerPod / m_numHostPerPod;
    NS_LOG_DEBUG("Num of intro Pod flows(/host) : " << numIntroPodFlowsPerHost);
    m_numIntroPodFlowsPerHostPerInterval = numIntroPodFlowsPerHost / m_numVirtualInterval;
    NS_LOG_DEBUG("Num of intro Pod flows(/host/interval) : " << m_numIntroPodFlowsPerHostPerInterval);

    int32_t numTotalFlowsPerHost = numInterPodFlowsPerHost + numIntroPodFlowsPerHost;
    NS_LOG_DEBUG("Num of total flows(/host) : " << numTotalFlowsPerHost);
    NS_LOG_DEBUG("Num of total flows(/host/interval) : " 
		 << m_numInterPodFlowsPerHostPerInterval + m_numIntroPodFlowsPerHostPerInterval);

    int32_t  numElephantFlowsPerHost = numTotalFlowsPerHost * 0.2;
    int32_t  numMouseFlowsPerHost    = numTotalFlowsPerHost * 0.8;

    uint64_t bpsElephantFlowsPerHost = m_bpsHst.GetBitRate()*0.8;
    uint64_t bpsMouseFlowsPerHost    = m_bpsHst.GetBitRate()*0.2;

    uint64_t bpsMeanElephantPerFlow  = bpsElephantFlowsPerHost / numElephantFlowsPerHost;
    uint64_t bpsMeanMousePerFlow     = bpsMouseFlowsPerHost    / numMouseFlowsPerHost;
    
    uint64_t bpsBoundElephantPerFlow = bpsMeanElephantPerFlow * 10;
    uint64_t bpsBoundMousePerFlow    = bpsMeanMousePerFlow    * 10;
    NS_LOG_DEBUG("Elephant flow : mean bps " << bpsMeanElephantPerFlow 
		 << " bound " << bpsBoundElephantPerFlow
		 << " cnt " << numElephantFlowsPerHost);
    NS_LOG_DEBUG("Mouse flow : mean bps "    << bpsMeanMousePerFlow    
		 << " bound " << bpsBoundMousePerFlow
		 << " cnt " << numMouseFlowsPerHost);
    
    m_elephantBps = CreateObject<ExponentialRandomVariable>();
    m_elephantBps->SetAttribute("Mean", DoubleValue(bpsMeanElephantPerFlow));
    m_elephantBps->SetAttribute("Bound", DoubleValue(bpsBoundElephantPerFlow));

    m_mouseBps = CreateObject<ExponentialRandomVariable>();;
    m_mouseBps->SetAttribute("Mean", DoubleValue(bpsMeanMousePerFlow));
    m_mouseBps->SetAttribute("Bound", DoubleValue(bpsBoundMousePerFlow));
    
    return;
  }

  void 
  NeoFlowGenerator::SetupApplications()
  {
    NS_LOG_DEBUG("===Setup flow applications===");
    NS_LOG_DEBUG("Interval : " << m_idxVirtualInterval << " Start From: " 
		 << Simulator::Now().GetMilliSeconds() << "ms");

    /*
    for(int iSrcPod = 0; iSrcPod < m_numPod; ++iSrcPod)
      {
	NS_LOG_DEBUG("Host in pod " << iSrcPod << " setting");
	for(int iSrcHst = 0; iSrcHst < m_numHostPerPod; ++iSrcHst )
	  {
	    NS_LOG_DEBUG("Hst " << iSrcHst << " setting");

	    SetupFlowsOriginFrom(iSrcPod, iSrcHst);
	    //SetupTestFlowsOriginFrom(iSrcPod, iSrcHst)；
	    Ptr<Node> node = m_podHostNodes[iSrcPod].Get(iSrcHst);
	    NS_LOG_DEBUG("Total apps on node : " << node->GetNApplications());
	  }
	
      }
    */
    Ptr<Node> srcNode = m_podHostNodes[0].Get(0);
    Ptr<Node> dstNode = m_podHostNodes[3].Get(0);
    uint64_t  bps = m_elephantBps->GetInteger();
    SetupUDPFlow(srcNode, dstNode, bps, port++, Simulator::Now(), Simulator::Now()+m_intervalTime); 

    //Setup next virtual interval simulation
    ++m_idxVirtualInterval;
    if(m_idxVirtualInterval < m_numVirtualInterval)
      {
	NS_LOG_DEBUG("Schedule next interval");
	Simulator::Schedule(m_intervalTime, &NeoFlowGenerator::SetupApplications, this);
      }
    else
      {
	NS_LOG_DEBUG("All flow generated");
      }
  }

  void
  NeoFlowGenerator::SetupFlowsOriginFrom(int iSrcPod, int iSrcHst)
  {
    Ptr<Node> srcNode   = m_podHostNodes[iSrcPod].Get(iSrcHst);
    Time      startTime = Simulator::Now();
    Time      endTime   = Simulator::Now() + m_intervalTime;

    /*1.Set up inter Pod flows
     */
    int16_t              m_numHalfPod   = m_numPod / 2;
    int16_t              startPodOffset = (iSrcPod < m_numHalfPod) ? m_numHalfPod : 0;
    int16_t              nextPod = 0;
    std::vector<int16_t> nextHostInPod(m_numHalfPod, 0);
    int32_t              numElephantThreshold = m_numInterPodFlowsPerHostPerInterval * 0.2; 
    
    for(int32_t iF = 0; iF < m_numInterPodFlowsPerHostPerInterval; ++iF)
      {
	int16_t iDstPod = startPodOffset + nextPod; 
	int16_t iDstHst = nextHostInPod[nextPod];
	int16_t port    = m_nextDstPort[iDstPod][iDstHst]++;
	NS_ASSERT_MSG(port < 65535, "Port overflow");

	//Prepare flow parameters
	Ptr<Node> dstNode = m_podHostNodes[iDstPod].Get(iDstHst);
	NS_ASSERT_MSG(srcNode != dstNode, "Do not send to yourself");
	uint64_t  bps = 0;
	if(iF < numElephantThreshold) bps = m_elephantBps->GetInteger();
	else bps = m_mouseBps->GetInteger();
	Time offset(m_startTimeOffset->GetValue());
	
	/*
	NS_LOG_DEBUG("srcPod "   << iSrcPod << " srcHst " << iSrcHst 
		     << " dstPod " << iDstPod << " dstHst " << iDstHst
		     << " port "   << port << " bps " << bps
		     << " " << (startTime + offset).GetMilliSeconds() 
		     << " " << endTime.GetMilliSeconds());
	*/
	SetupUDPFlow(srcNode, dstNode, bps, port, startTime + offset, endTime);

	//Update Pod Host index;
	nextHostInPod[nextPod]++; 
	nextHostInPod[nextPod] %= m_numHostPerPod;
	
	nextPod++;
	nextPod %= m_numHalfPod;
	
      }

    /*2.Set up intra Pod flows
     */
    int16_t nextHstInSrcPod = 0;
    numElephantThreshold = m_numIntroPodFlowsPerHostPerInterval * 0.2;
    
    for(int32_t iF = 0; iF < m_numIntroPodFlowsPerHostPerInterval; ++iF)
      {
	if(nextHstInSrcPod == iSrcHst) 
	  {
	    ++nextHstInSrcPod;
	    nextHstInSrcPod %= m_numHostPerPod;
	  }
	
	int16_t port = m_nextDstPort[iSrcPod][nextHstInSrcPod]++;
	NS_ASSERT_MSG(port < 65535, "Port overflow");

	//Prepare flow parameters
	Ptr<Node> dstNode = m_podHostNodes[iSrcPod].Get(nextHstInSrcPod);
	NS_ASSERT_MSG(srcNode != dstNode, "Do not sent to yourself");
	uint64_t bps = 0;
	if(iF < numElephantThreshold) bps = m_elephantBps->GetInteger();
	else bps = m_mouseBps->GetInteger();
	Time offset(m_startTimeOffset->GetValue());

	/*
	NS_LOG_DEBUG("srcPod "   << iSrcPod << " srcHst " << iSrcHst 
		     << " dstPod " << iSrcPod << " dstHst " << nextHstInSrcPod
		     << " port "   << port << " bps " << bps
		     << " " << (startTime + offset).GetMilliSeconds() 
		     << " " << endTime.GetMilliSeconds());
	*/
	SetupUDPFlow(srcNode, dstNode, bps, port, startTime + offset, endTime);

	//Update Host index
	++nextHstInSrcPod;
	nextHstInSrcPod %= m_numHostPerPod;
      }
  }

  

  /*Send flow originated from one host, destinated to all other hosts
   *Use to test the network works correctly.
   */
  void
  NeoFlowGenerator::SetupTestFlowsOriginFrom(int iSrcPod, int iSrcHst)
  {

    uint64_t bps  = DataRate("100kbps").GetBitRate();

    for(int iDstPod = 0; iDstPod < m_numPod; ++iDstPod)
      {
	for(int iDstHst = 0; iDstHst < m_numHostPerPod; ++iDstHst)
	  {
	    if(iSrcHst == iDstHst && iSrcPod == iDstPod) continue;
	    
	    uint16_t port = m_nextDstPort[iDstPod][iDstHst]++; //Add 1 Each Time 
	    NS_ASSERT_MSG(port < 65535, "Port overflow");
	    
	    bps += 100; 
	    
	    Ptr<Node> srcNode = m_podHostNodes[iSrcPod].Get(iSrcHst);
	    Ptr<Node> dstNode = m_podHostNodes[iDstPod].Get(iDstHst);
	    SetupUDPFlow(srcNode, dstNode, bps, port, Time(), Time());
	  }
      } 
  }


  void
  NeoFlowGenerator::SetupUDPFlow(Ptr<Node> srcNode, Ptr<Node> dstNode, 
				 uint64_t bps, uint16_t port, 
				 const Time& startTime, const Time& endTime)
  {
    ApplicationContainer apps;
    
    Ipv4Address dstIpv4Addr = GetIpv4Addr(dstNode); 
    OnOffHelper onOff("ns3::UdpSocketFactory",
		      Address(InetSocketAddress(dstIpv4Addr, port)));
    onOff.SetConstantRate(DataRate(bps));
    //For debug
    onOff.SetAttribute("MaxBytes", UintegerValue(512));
    apps.Add(onOff.Install(srcNode));
  
    PacketSinkHelper sink("ns3::UdpSocketFactory",
			  Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    apps.Add(sink.Install(dstNode));

    apps.Start(startTime);
    apps.Stop(endTime);
  }

  /*Helper Functions definations:*/
  Ipv4Address GetIpv4Addr(Ptr<Node> hstNode)
  {
    return hstNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
  }
  
}
