//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_ICMP_H
#define __INET_ICMP_H

//  Cleanup and rewrite: Andras Varga, 2004

#include "inet/common/INETDefs.h"

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/ipv4/ICMPMessage.h"
#include "inet/networklayer/ipv4/IIPv4RoutingTable.h"

namespace inet {

class IPv4Datagram;
class IPv4ControlInfo;

/**
 * ICMP module.
 */
class INET_API ICMP : public cSimpleModule, public IProtocolRegistrationListener
{
  protected:
    std::set<int> transportProtocols;    // where to send up packets
  protected:
    virtual void processICMPMessage(ICMPMessage *);
    virtual void processIcmpErrorFromIPv4(IPv4Datagram *dgram);
    virtual void processUpperMessage(cMessage *msg);
    virtual void errorOut(ICMPMessage *);
    virtual void processEchoRequest(ICMPMessage *);
    virtual void sendToIP(ICMPMessage *, const IPv4Address& dest);
    virtual void sendToIP(ICMPMessage *msg);
    virtual bool possiblyLocalBroadcast(const IPv4Address& addr, int interfaceId);
    virtual void handleRegisterProtocol(const Protocol& protocol, cGate *gate) override;
    virtual void sendUpIcmpError(IPv4Datagram *bogusL3Packet, ICMPType icmpType, int icmpCode);
    virtual void sendErrorMessage(cPacket *transportPacket, IPv4ControlInfo *ctrl, ICMPType type, ICMPCode code);

  public:
    /**
     * This method can be called from other modules to send an ICMP error packet
     * in response to a received bogus packet. It will not send ICMP error in response
     * to broadcast or multicast packets -- in that case it will simply delete the packet.
     * Kludge: if inputInterfaceId cannot be determined, pass in -1.
     */
    virtual void sendErrorMessage(IPv4Datagram *datagram, int inputInterfaceId, ICMPType type, ICMPCode code);

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
};

} // namespace inet

#endif // ifndef __INET_ICMP_H

