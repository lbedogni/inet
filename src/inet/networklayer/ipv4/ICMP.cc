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

//  Cleanup and rewrite: Andras Varga, 2004

#include <string.h>

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/ipv4/ICMP.h"

#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/contract/L3Error.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/applications/pingapp/PingPayload_m.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/ipv4/IcmpErrorFromIPControlInfo_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/common/ModuleAccess.h"

namespace inet {

Define_Module(ICMP);

namespace {

//TODO add other constants, verify
L3ErrorControlInfoErrorCodes icmpToErrorCode(int type, int code)
{
    switch (type) {
        case ICMP_DESTINATION_UNREACHABLE:
            switch (code) {
                case ICMP_DU_NETWORK_UNREACHABLE: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_HOST_UNREACHABLE: return L3ERROR_HOST_UNREACHABLE;
                case ICMP_DU_PROTOCOL_UNREACHABLE: return L3ERROR_PROTOCOL_UNREACHABLE;
                case ICMP_DU_PORT_UNREACHABLE: return L3ERROR_PORT_UNREACHABLE;
                case ICMP_DU_FRAGMENTATION_NEEDED: return L3ERROR_FRAGMENTATION_NEEDED;
                case ICMP_DU_SOURCE_ROUTE_FAILED: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_DESTINATION_NETWORK_UNKNOWN: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_DESTINATION_HOST_UNKNOWN: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_SOURCE_HOST_ISOLATED: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_NETWORK_PROHIBITED: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_HOST_PROHIBITED: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_NETWORK_UNREACHABLE_FOR_TYPE_OF_SERVICE: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_HOST_UNREACHABLE_FOR_TYPE_OF_SERVICE: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_COMMUNICATION_PROHIBITED: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_HOST_PRECEDENCE_VIOLATION: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_DU_PRECEDENCE_CUTOFF_IN_EFFECT: return L3ERROR_DEST_UNREACHABLE;
                case ICMP_AODV_QUEUE_FULL: return L3ERROR_DEST_UNREACHABLE;
                default: throw cRuntimeError("Unknown ICMP destination unreachable code: %d", code);
            }
        case ICMP_TIME_EXCEEDED: return L3ERROR_TIME_EXCEEDED;
        default: throw cRuntimeError("Unknown ICMP type: %d", type);
    }
}

//TODO add other constants, verify
void convertErrorCodeToTypeAndCode(int errorCode, ICMPType& type, ICMPCode& code)
{
    switch (errorCode) {
        case L3ERROR_DEST_UNREACHABLE: type = ICMP_DESTINATION_UNREACHABLE; code = 0; break;
        case L3ERROR_HOST_UNREACHABLE: type = ICMP_DESTINATION_UNREACHABLE; code = ICMP_DU_HOST_UNREACHABLE; break;
        case L3ERROR_PROTOCOL_UNREACHABLE: type = ICMP_DESTINATION_UNREACHABLE; code = ICMP_DU_PROTOCOL_UNREACHABLE; break;
        case L3ERROR_PORT_UNREACHABLE: type = ICMP_DESTINATION_UNREACHABLE; code = ICMP_DU_PORT_UNREACHABLE; break;
        case L3ERROR_FRAGMENTATION_NEEDED: type = ICMP_DESTINATION_UNREACHABLE; code = ICMP_DU_FRAGMENTATION_NEEDED; break;
        case L3ERROR_TIME_EXCEEDED: type = ICMP_TIME_EXCEEDED; code = 0; break;
        default: throw cRuntimeError("Unknown L3ErrorControlInfoErrorCodes value: %d", errorCode); break;
    }
}

} // namespace

void ICMP::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER_2) {
        registerProtocol(Protocol::icmpv4, gate("ipOut"));
        registerProtocol(Protocol::icmpv4, gate("transportOut"));
    }
}

void ICMP::handleMessage(cMessage *msg)
{
    cGate *arrivalGate = msg->getArrivalGate();

    // process arriving ICMP message
    if (!strcmp(arrivalGate->getName(), "ipIn")) {
        EV_INFO << "Received " << msg << " from network protocol.\n";
        if (ICMPMessage *icmpMsg = dynamic_cast<ICMPMessage *>(msg))
            processICMPMessage(icmpMsg);
        else
            processIcmpErrorFromIPv4(check_and_cast<IPv4Datagram *>(msg));
        return;
    }
    else if (!strcmp(arrivalGate->getName(), "transportIn")) {
        processUpperMessage(msg);
    }
    else
        throw cRuntimeError("unknown gate: '%s'", arrivalGate->getName());
}

void ICMP::sendErrorMessage(IPv4Datagram *origDatagram, int inputInterfaceId, ICMPType type, ICMPCode code)
{
    Enter_Method("sendErrorMessage(datagram, type=%d, code=%d)", type, code);

    // get ownership
    take(origDatagram);

    // don't send ICMP error messages in response to broadcast or multicast messages
    IPv4Address origDestAddr = origDatagram->getDestAddress();
    if (origDestAddr.isMulticast() || origDestAddr.isLimitedBroadcastAddress() || possiblyLocalBroadcast(origDestAddr, inputInterfaceId)) {
        EV_DETAIL << "won't send ICMP error messages for broadcast/multicast message " << origDatagram << endl;
        delete origDatagram;
        return;
    }

    // don't send ICMP error messages response to unspecified, broadcast or multicast addresses
    IPv4Address origSrcAddr = origDatagram->getSrcAddress();
    if (origSrcAddr.isUnspecified() || origSrcAddr.isMulticast() || origSrcAddr.isLimitedBroadcastAddress() || possiblyLocalBroadcast(origSrcAddr, inputInterfaceId)) {
        EV_DETAIL << "won't send ICMP error messages to broadcast/multicast address, message " << origDatagram << endl;
        delete origDatagram;
        return;
    }

    // do not reply with error message to error message
    if (origDatagram->getTransportProtocol() == IP_PROT_ICMP) {
        ICMPMessage *recICMPMsg = check_and_cast<ICMPMessage *>(origDatagram->getEncapsulatedPacket());
        if (!isIcmpInfoType(recICMPMsg->getType())) {
            EV_DETAIL << "ICMP error received -- do not reply to it" << endl;
            delete origDatagram;
            return;
        }
    }

    // assemble a message name
    char msgname[100];
    static long ctr;
    sprintf(msgname, "ICMP-error-#%ld-type%d-code%d", ++ctr, type, code);

    // debugging information
    EV_DETAIL << "sending ICMP error " << msgname << endl;

    // create and send ICMP packet
    ICMPMessage *errorMessage = new ICMPMessage(msgname);
    errorMessage->setType(type);
    errorMessage->setCode(code);
    errorMessage->encapsulate(origDatagram);

    // ICMP message length: the internet header plus the first 8 bytes of
    // the original datagram's data is returned to the sender.
    //
    // NOTE: since we just overwrite the errorMessage length without actually
    // truncating origDatagram, one can get "packet length became negative"
    // error when decapsulating the origDatagram on the receiver side.
    // A workaround is to avoid decapsulation, or to manually set the
    // errorMessage length to be larger than the encapsulated message.
    int dataLength = origDatagram->getByteLength() - origDatagram->getHeaderLength();
    int truncatedDataLength = dataLength <= 8 ? dataLength : 8;
    errorMessage->setByteLength(8 + origDatagram->getHeaderLength() + truncatedDataLength);

    // if srcAddr is not filled in, we're still in the src node, so we just
    // process the ICMP message locally, right away
    if (origDatagram->getSrcAddress().isUnspecified()) {
        // pretend it came from the IPv4 layer
        IPv4ControlInfo *controlInfo = new IPv4ControlInfo();
        controlInfo->setSrcAddr(IPv4Address::LOOPBACK_ADDRESS);    // FIXME maybe use configured loopback address
        controlInfo->setProtocol(IP_PROT_ICMP);
        errorMessage->setControlInfo(controlInfo);

        // then process it locally
        processICMPMessage(errorMessage);
    }
    else {
        sendToIP(errorMessage, origDatagram->getSrcAddress());
    }
}

void ICMP::sendErrorMessage(cPacket *transportPacket, IPv4ControlInfo *ctrl, ICMPType type, ICMPCode code)
{
    Enter_Method("sendErrorMessage(transportPacket, ctrl, type=%d, code=%d)", type, code);

    IPv4Datagram *datagram = ctrl->removeOrigDatagram();
    int inputInterfaceId = ctrl->getInterfaceId();
    delete ctrl;
    take(transportPacket);
    take(datagram);
    datagram->encapsulate(transportPacket);
    sendErrorMessage(datagram, inputInterfaceId, type, code);
}

bool ICMP::possiblyLocalBroadcast(const IPv4Address& addr, int interfaceId)
{
    if ((addr.getInt() & 1) == 0)
        return false;

    IIPv4RoutingTable *rt = getModuleFromPar<IIPv4RoutingTable>(par("routingTableModule"), this);
    if (rt->isLocalBroadcastAddress(addr))
        return true;

    // if the input interface is unconfigured, we won't recognize network-directed broadcasts because we don't what network we are on
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (interfaceId != -1) {
        InterfaceEntry *ie = ift->getInterfaceById(interfaceId);
        bool interfaceUnconfigured = (ie->ipv4Data() == nullptr) || ie->ipv4Data()->getIPAddress().isUnspecified();
        return interfaceUnconfigured;
    }
    else {
        // if all interfaces are configured, we are OK
        bool allInterfacesConfigured = true;
        for (int i = 0; i < (int)ift->getNumInterfaces(); i++)
            if ((ift->getInterface(i)->ipv4Data() == nullptr) || ift->getInterface(i)->ipv4Data()->getIPAddress().isUnspecified())
                allInterfacesConfigured = false;

        return !allInterfacesConfigured;
    }
}

void ICMP::processUpperMessage(cMessage *msg)
{
    L3Error *errmsg = check_and_cast<L3Error *>(msg);
    cPacket *pk = errmsg->decapsulate();
    L3ErrorControlInfo *icmpCtrl = check_and_cast<L3ErrorControlInfo *>(errmsg->getControlInfo());
    IPv4ControlInfo *ctrl = check_and_cast<IPv4ControlInfo *>(icmpCtrl->getNetworkProtocolControlInfo());
    icmpCtrl->setNetworkProtocolControlInfo(nullptr);
    ICMPType type;
    ICMPCode code;
    convertErrorCodeToTypeAndCode(icmpCtrl->getErrorCode(), type, code);
    sendErrorMessage(pk, ctrl, type, code);
    delete icmpCtrl;
    delete errmsg;
}

void ICMP::sendUpIcmpError(IPv4Datagram *bogusL3Packet, ICMPType icmpType, int icmpCode)
{
    // ICMP errors are delivered to the appropriate higher layer protocol
    cPacket *bogusTransportPacket = bogusL3Packet->decapsulate();
    if (bogusTransportPacket) {
        int transportProtocol = bogusL3Packet->getTransportProtocol();
        if (transportProtocol == IP_PROT_ICMP) {
            EV_DETAIL << "ICMP error response for ICMP packet, packet dropped\n";
            delete bogusTransportPacket;
        }
        else if (transportProtocols.find(transportProtocol) == transportProtocols.end()) {
            EV_WARN << "Transport protocol " << transportProtocol << " not registered, packet dropped\n";
            delete bogusTransportPacket;
        }
        else {
            L3Error *msg = new L3Error("L3Error");
            L3ErrorControlInfo *ctrl = new L3ErrorControlInfo();
            ctrl->setTransportProtocol(bogusL3Packet->getTransportProtocol());
            ctrl->setSourceAddress(bogusL3Packet->getSourceAddress());
            ctrl->setDestinationAddress(bogusL3Packet->getDestinationAddress());
            int icmpUpErrCode = icmpToErrorCode(icmpType, icmpCode);
            ctrl->setErrorCode(icmpUpErrCode);
            msg->encapsulate(bogusTransportPacket);
            msg->setControlInfo(ctrl);
            cEnum *enump = cEnum::get("inet::L3ErrorControlInfoErrorCodes");
            const char *namep = enump->getStringFor(icmpUpErrCode);
            if (!namep)
                namep = "L3 ERROR";
            std::string name = std::string() + namep + " (" + bogusTransportPacket->getName() + ")";
            msg->setName(name.c_str());
            send(msg, "transportOut");
        }
    }
    delete bogusL3Packet;
}

void ICMP::processICMPMessage(ICMPMessage *icmpmsg)
{
    switch (icmpmsg->getType()) {
        case ICMP_REDIRECT:
            // TODO implement redirect handling
            delete icmpmsg;
            break;

        case ICMP_DESTINATION_UNREACHABLE:
        case ICMP_TIME_EXCEEDED:
        case ICMP_PARAMETER_PROBLEM: {
            // ICMP errors are delivered to the appropriate higher layer protocol
            icmpmsg->setByteLength(0);  // Hack for prevent cRuntimeError: decapsulate(): packet length is smaller than encapsulated packet.
            IPv4Datagram *bogusL3Packet = check_and_cast<IPv4Datagram *>(icmpmsg->decapsulate());
            sendUpIcmpError(bogusL3Packet, (ICMPType)icmpmsg->getType(), icmpmsg->getCode());
            delete icmpmsg;
            break;
        }

        case ICMP_ECHO_REQUEST:
            processEchoRequest(icmpmsg);
            break;

        case ICMP_ECHO_REPLY:
            delete icmpmsg;
            break;

        case ICMP_TIMESTAMP_REQUEST:
            processEchoRequest(icmpmsg);
            break;

        case ICMP_TIMESTAMP_REPLY:
            delete icmpmsg;
            break;

        default:
            throw cRuntimeError("Unknown ICMP type %d", icmpmsg->getType());
    }
}

void ICMP::processIcmpErrorFromIPv4(IPv4Datagram *dgram)
{
    IcmpErrorFromIPControlInfo *ctrl = check_and_cast<IcmpErrorFromIPControlInfo *>(dgram->removeControlInfo());
    switch (ctrl->getDirection()) {
        case TO_NETWORK:
            sendErrorMessage(dgram, ctrl->getInterfaceId(), (ICMPType)ctrl->getIcmpType(), ctrl->getIcmpCode());
            break;
        case TO_LOCAL:
            sendUpIcmpError(dgram, (ICMPType)ctrl->getIcmpType(), ctrl->getIcmpCode());
            break;
        default:
            throw cRuntimeError("Invalid direction %d in IcmpErrorFromIPControlInfo", ctrl->getDirection());
    }
    delete ctrl;
}

void ICMP::errorOut(ICMPMessage *icmpmsg)
{
    delete icmpmsg;
}

void ICMP::processEchoRequest(ICMPMessage *request)
{
    // turn request into a reply
    ICMPMessage *reply = request;
    reply->setName((std::string(request->getName()) + "-reply").c_str());
    reply->setType(ICMP_ECHO_REPLY);

    // swap src and dest
    // TBD check what to do if dest was multicast etc?
    IPv4ControlInfo *ctrl = check_and_cast<IPv4ControlInfo *>(reply->getControlInfo());
    IPv4Address src = ctrl->getSrcAddr();
    IPv4Address dest = ctrl->getDestAddr();
    // A. Ariza Modification 5/1/2011 clean the interface id, this forces the use of routing table in the IPv4 layer
    ctrl->setInterfaceId(-1);
    ctrl->setSrcAddr(dest);
    ctrl->setDestAddr(src);

    sendToIP(reply);
}

void ICMP::sendToIP(ICMPMessage *msg, const IPv4Address& dest)
{
    IPv4ControlInfo *controlInfo = new IPv4ControlInfo();
    controlInfo->setDestAddr(dest);
    controlInfo->setProtocol(IP_PROT_ICMP);
    msg->setControlInfo(controlInfo);
    sendToIP(msg);
}

void ICMP::sendToIP(ICMPMessage *msg)
{
    // assumes IPv4ControlInfo is already attached
    EV_INFO << "Sending " << msg << " to lower layer.\n";
    send(msg, "ipOut");
}

void ICMP::handleRegisterProtocol(const Protocol& protocol, cGate *gate)
{
    Enter_Method("handleRegisterProtocol");
    if (!strcmp("transportIn", gate->getBaseName())) {
        transportProtocols.insert(ProtocolGroup::ipprotocol.getProtocolNumber(&protocol));
    }
}

} // namespace inet

