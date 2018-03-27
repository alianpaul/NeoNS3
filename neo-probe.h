#ifndef NEO_PROBE_H
#define NEO_PROBE_H

#include "ns3/object.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#include <iostream>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <string>

namespace ns3
{

  class Packet;

  ///Table Fields
  ///1.Flow Field
  struct FlowField
  {
    uint32_t ipv4srcip;
    uint32_t ipv4dstip;
    uint16_t srcport;
    uint16_t dstport;
    uint8_t  ipv4prot;

    FlowField ()
      :ipv4srcip(0), ipv4dstip(0), 
       srcport(0), dstport(0), 
       ipv4prot(0)
    {
    }
    
    void InitFromPacket(const Ipv4Header& ipHeader, Ptr<const Packet> ipPayload);
  };
  bool          operator== (const FlowField& lhs, const FlowField& rhs);
  std::ostream& operator<< (std::ostream& os, const FlowField& flow);

  ///2.Pakcet Byte Counter Field
  struct PckByteField
  {
    uint16_t pckcnt;
    uint32_t bytecnt;

    PckByteField ()
      : pckcnt(0), bytecnt(0)
    {
    }
  };
  std::ostream& operator<< (std::ostream& os, const PckByteField& pckbyte);

  ///Flow statics container type
  struct FlowFieldBoostHash
    : std::unary_function<FlowField, std::size_t>
  {
    std::size_t operator()(FlowField const& f) const
    {
      std::size_t seed = 0;
      boost::hash_combine(seed, f.ipv4srcip);
      boost::hash_combine(seed, f.ipv4dstip);
      boost::hash_combine(seed, f.srcport);
      boost::hash_combine(seed, f.dstport);
      boost::hash_combine(seed, f.ipv4prot);
      return seed;
    }
  };
  typedef boost::unordered_map<FlowField, PckByteField, FlowFieldBoostHash>                 FlowStatContainer;
  typedef boost::unordered_map<FlowField, PckByteField, FlowFieldBoostHash>::iterator       FlowStatContainerI;
  typedef boost::unordered_map<FlowField, PckByteField, FlowFieldBoostHash>::const_iterator FlowStatContainerCI;
  
  
  class Node;

  class NeoProbe : public Object
  {
  protected:
    /// Constructor, subclass call
    NeoProbe (Ptr<Node> node);
  public:
    virtual ~NeoProbe ();
    static TypeId GetTypeId (void);
  private:
    NeoProbe (const NeoProbe& rhs);
    NeoProbe& operator= (const NeoProbe& rhs);

  public:
    void PrintRealFlowStats (std::string fileNameSuffix) const;

  protected:
            void UpdateRealFlowStats (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload);
    virtual void ForwardLogger (const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface) = 0;
    virtual void PrintMeasurementStats (std::string fileNameSuffix) const = 0;
  
  private:
    uint32_t            m_nodeId;
    Ptr<Ipv4L3Protocol> m_ipv4; //the Ipv4L3Protocol this probe is bound to
    FlowStatContainer   m_realFlowStats;
  };

}


#endif
