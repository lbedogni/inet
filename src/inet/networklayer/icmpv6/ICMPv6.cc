//
// Copyright (C) 2005 Andras Varga
// Copyright (C) 2005 Wei Yang, Ng
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

#include "inet/common/INETDefs.h"

#include "inet/applications/pingapp/PingPayload_m.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/networklayer/contract/L3Error.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"
#include "inet/networklayer/icmpv6/ICMPv6.h"
#include "inet/networklayer/icmpv6/ICMPv6Message_m.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/networklayer/ipv6/IPv6InterfaceData.h"

namespace inet {

Define_Module(ICMPv6);

namespace {

//TODO add other constants, verify
L3ErrorControlInfoErrorCodes icmpv6ToErrorCode(ICMPv6Type type, int code)
{
    switch (type) {
        case ICMPv6_DESTINATION_UNREACHABLE:
            switch (code) {
                case NO_ROUTE_TO_DEST: return L3ERROR_DEST_UNREACHABLE;
                case ADDRESS_UNREACHABLE: return L3ERROR_HOST_UNREACHABLE;
                case COMM_WITH_DEST_PROHIBITED: return L3ERROR_PROTOCOL_UNREACHABLE;
                case PORT_UNREACHABLE: return L3ERROR_PORT_UNREACHABLE;
                default: break;
            }
            throw cRuntimeError("Unknown ICMPv6 destination unreachable code: %d", code);
        case ICMPv6_PACKET_TOO_BIG: return L3ERROR_FRAGMENTATION_NEEDED;
        case ICMPv6_PARAMETER_PROBLEM: return L3ERROR_PARAMETER_PROBLEM;
        case ICMPv6_TIME_EXCEEDED: return L3ERROR_TIME_EXCEEDED;
        default: break;
    }
    throw cRuntimeError("Unknown ICMPv6 type: %d, code: %d", type, code);
}

//TODO add other constants, verify
void convertErrorCodeToIcmpv6TypeAndCode(int errorCode, ICMPv6Type& type, int& code)
{
    switch (errorCode) {
        case L3ERROR_DEST_UNREACHABLE: type = ICMPv6_DESTINATION_UNREACHABLE; code = NO_ROUTE_TO_DEST; break;
        case L3ERROR_HOST_UNREACHABLE: type = ICMPv6_DESTINATION_UNREACHABLE; code = ADDRESS_UNREACHABLE; break;
        case L3ERROR_PROTOCOL_UNREACHABLE: type = ICMPv6_DESTINATION_UNREACHABLE; code = COMM_WITH_DEST_PROHIBITED; break;
        case L3ERROR_PORT_UNREACHABLE: type = ICMPv6_DESTINATION_UNREACHABLE; code = PORT_UNREACHABLE; break;
        case L3ERROR_FRAGMENTATION_NEEDED: type = ICMPv6_PACKET_TOO_BIG; code = 0; break;
        case L3ERROR_PARAMETER_PROBLEM: type = ICMPv6_PARAMETER_PROBLEM; code = 0; break;
        case L3ERROR_TIME_EXCEEDED: type = ICMPv6_TIME_EXCEEDED; code = 0; break;
        default: throw cRuntimeError("Unknown L3ErrorControlInfoErrorCodes value: %d", errorCode); break;
    }
}

} // namespace

void ICMPv6::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_NETWORK_LAYER) {
        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");
    }
    else if (stage == INITSTAGE_NETWORK_LAYER_2) {
        registerProtocol(Protocol::icmpv6, gate("ipv6Out"));
    }
}

void ICMPv6::handleMessage(cMessage *msg)
{
    ASSERT(!msg->isSelfMessage());    // no timers in ICMPv6

    cGate *arrivalGate = msg->getArrivalGate();

    // process arriving ICMP message
    if (arrivalGate->isName("ipv6In")) {
        EV_INFO << "Processing ICMPv6 message.\n";
        processICMPv6Message(check_and_cast<ICMPv6Message *>(msg));
        return;
    }
    // request from application
    else if (arrivalGate->isName("transportIn")) {
        processUpperMessage(msg);
        return;
    }
    else
        throw cRuntimeError("unknown gate: '%s'", arrivalGate->getName());
}

void ICMPv6::processUpperMessage(cMessage *msg)
{
    L3ErrorControlInfo *icmpCtrl = check_and_cast<L3ErrorControlInfo *>(msg->removeControlInfo());
    IPv6ControlInfo *ctrl = check_and_cast<IPv6ControlInfo *>(icmpCtrl->getNetworkProtocolControlInfo());
    icmpCtrl->setNetworkProtocolControlInfo(nullptr);
    ICMPv6Type type;
    int code;
    convertErrorCodeToIcmpv6TypeAndCode(icmpCtrl->getErrorCode(), type, code);
    sendErrorMessage(PK(msg), ctrl, type, code);
    delete icmpCtrl;
}

void ICMPv6::processICMPv6ErrorMessage(ICMPv6Message *icmpv6msg)
{
    // ICMPv6 errors are delivered to the appropriate higher layer protocol
    IPv6Datagram *bogusL3Packet = check_and_cast<IPv6Datagram *>(icmpv6msg->getEncapsulatedPacket());
    cPacket *bogusTransportPacket = bogusL3Packet->decapsulate();
    if (bogusTransportPacket) {
        int transportProtocol = bogusL3Packet->getTransportProtocol();
        if (transportProtocol == IP_PROT_IPv6_ICMP) {
            EV_DETAIL << "ICMPv6 error response for ICMPv6 packet, packet dropped\n";
            delete bogusTransportPacket;
        }
        else if (transportProtocols.find(transportProtocol) == transportProtocols.end()) {
            EV_WARN << "Transport protocol " << transportProtocol << " not registered, packet dropped\n";
            delete bogusTransportPacket;
        }
        else {
            L3ErrorControlInfo *ctrl = new L3ErrorControlInfo();
            ctrl->setTransportProtocol(bogusL3Packet->getTransportProtocol());
            ctrl->setSourceAddress(bogusL3Packet->getSourceAddress());
            ctrl->setDestinationAddress(bogusL3Packet->getDestinationAddress());
            ctrl->setErrorCode(icmpv6ToErrorCode((ICMPv6Type)icmpv6msg->getType(), icmpv6msg->getCode()));
            L3Error *errmsg = new L3Error();
            errmsg->encapsulate(bogusTransportPacket);
            errmsg->setControlInfo(ctrl);
            errmsg->setName(icmpv6msg->getName());
            send(errmsg, "transportOut");
        }
    }
    delete icmpv6msg;
}

void ICMPv6::processICMPv6InfoMessage(ICMPv6Message *icmpv6msg)
{
    switch (icmpv6msg->getType()) {
        case ICMPv6_ECHO_REQUEST:
            EV_INFO << "ICMPv6 Echo Request Message Received." << endl;
            processEchoRequest(check_and_cast<ICMPv6EchoRequestMsg *>(icmpv6msg));
            break;
        case ICMPv6_ECHO_REPLY:
            // Echo reply processed in PingApp
            delete icmpv6msg;
            break;

        default:
            throw cRuntimeError("Unknown ICMPv6 type %d in %s(%s)", icmpv6msg->getType(), icmpv6msg->getName(), icmpv6msg->getClassName());
   }
}

void ICMPv6::processICMPv6Message(ICMPv6Message *icmpv6msg)
{
    if (icmpv6msg->getType() <= 127 )
        processICMPv6ErrorMessage(icmpv6msg);
    else
        processICMPv6InfoMessage(icmpv6msg);
}

/*
 * RFC 4443 4.2:
 *
 * Every node MUST implement an ICMPv6 Echo responder function that
 * receives Echo Requests and originates corresponding Echo Replies.  A
 * node SHOULD also implement an application-layer interface for
 * originating Echo Requests and receiving Echo Replies, for diagnostic
 * purposes.
 *
 * The source address of an Echo Reply sent in response to a unicast
 * Echo Request message MUST be the same as the destination address of
 * that Echo Request message.
 *
 * An Echo Reply SHOULD be sent in response to an Echo Request message
 * sent to an IPv6 multicast or anycast address.  In this case, the
 * source address of the reply MUST be a unicast address belonging to
 * the interface on which the Echo Request message was received.
 *
 * The data received in the ICMPv6 Echo Request message MUST be returned
 * entirely and unmodified in the ICMPv6 Echo Reply message.
 */
void ICMPv6::processEchoRequest(ICMPv6EchoRequestMsg *request)
{
    //Create an ICMPv6 Reply Message
    ICMPv6EchoReplyMsg *reply = new ICMPv6EchoReplyMsg("Echo Reply");
    reply->setName((std::string(request->getName()) + "-reply").c_str());
    reply->setType(ICMPv6_ECHO_REPLY);
    reply->encapsulate(request->decapsulate());

    IPv6ControlInfo *ctrl = check_and_cast<IPv6ControlInfo *>(request->getControlInfo());
    IPv6ControlInfo *replyCtrl = new IPv6ControlInfo();
    replyCtrl->setProtocol(IP_PROT_IPv6_ICMP);
    replyCtrl->setDestAddr(ctrl->getSrcAddr());

    if (ctrl->getDestAddr().isMulticast()    /*TODO check for anycast too*/) {
        IInterfaceTable *it = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        IPv6InterfaceData *ipv6Data = it->getInterfaceById(ctrl->getInterfaceId())->ipv6Data();
        replyCtrl->setSrcAddr(ipv6Data->getPreferredAddress());
        // TODO implement default address selection properly.
        //      According to RFC 3484, the source address to be used
        //      depends on the destination address
    }
    else
        replyCtrl->setSrcAddr(ctrl->getDestAddr());

    reply->setControlInfo(replyCtrl);

    delete request;
    sendToIP(reply);
}

void ICMPv6::sendErrorMessage(IPv6Datagram *origDatagram, ICMPv6Type type, int code)
{
    Enter_Method("sendErrorMessage(datagram, type=%d, code=%d)", type, code);

    // get ownership
    take(origDatagram);

    if (!validateDatagramPromptingError(origDatagram))
        return;

    ICMPv6Message *errorMsg;

    if (type == ICMPv6_DESTINATION_UNREACHABLE)
        errorMsg = createDestUnreachableMsg(code);
    //TODO: implement MTU support.
    else if (type == ICMPv6_PACKET_TOO_BIG)
        errorMsg = createPacketTooBigMsg(0);
    else if (type == ICMPv6_TIME_EXCEEDED)
        errorMsg = createTimeExceededMsg(code);
    else if (type == ICMPv6_PARAMETER_PROBLEM)
        errorMsg = createParamProblemMsg(code);
    else
        throw cRuntimeError("Unknown ICMPv6 error type: %d\n", type);

    // Encapsulate the original datagram, but the whole ICMPv6 error
    // packet cannot be larger than the minimum IPv6 MTU (RFC 4443 2.4. (c)).
    // NOTE: since we just overwrite the errorMsg length without actually
    // truncating origDatagram, one can get "packet length became negative"
    // error when decapsulating the origDatagram on the receiver side.
    // A workaround is to avoid decapsulation, or to manually set the
    // errorMessage length to be larger than the encapsulated message.
    errorMsg->encapsulate(origDatagram);
    if (errorMsg->getByteLength() + IPv6_HEADER_BYTES > IPv6_MIN_MTU)
        errorMsg->setByteLength(IPv6_MIN_MTU - IPv6_HEADER_BYTES);

    // debugging information
    EV_DEBUG << "sending ICMP error: (" << errorMsg->getClassName() << ")" << errorMsg->getName()
             << " type=" << type << " code=" << code << endl;

    // if srcAddr is not filled in, we're still in the src node, so we just
    // process the ICMP message locally, right away
    if (origDatagram->getSrcAddress().isUnspecified()) {
        // pretend it came from the IP layer
        IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
        ctrlInfo->setSrcAddr(IPv6Address::LOOPBACK_ADDRESS);    // FIXME maybe use configured loopback address
        ctrlInfo->setProtocol(IP_PROT_ICMP);
        errorMsg->setControlInfo(ctrlInfo);

        // then process it locally
        processICMPv6Message(errorMsg);
    }
    else {
        sendToIP(errorMsg, origDatagram->getSrcAddress());
    }
}

void ICMPv6::sendErrorMessage(cPacket *transportPacket, IPv6ControlInfo *ctrl, ICMPv6Type type, int code)
{
    Enter_Method("sendErrorMessage(transportPacket, ctrl, type=%d, code=%d)", type, code);

    IPv6Datagram *datagram = ctrl->removeOrigDatagram();
    delete ctrl;
    take(transportPacket);
    take(datagram);
    datagram->encapsulate(transportPacket);
    sendErrorMessage(datagram, type, code);
}

void ICMPv6::sendToIP(ICMPv6Message *msg, const IPv6Address& dest)
{
    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setDestAddr(dest);
    ctrlInfo->setProtocol(IP_PROT_IPv6_ICMP);
    msg->setControlInfo(ctrlInfo);

    send(msg, "ipv6Out");
}

void ICMPv6::sendToIP(ICMPv6Message *msg)
{
    // assumes IPv6ControlInfo is already attached
    send(msg, "ipv6Out");
}

ICMPv6Message *ICMPv6::createDestUnreachableMsg(int code)
{
    ICMPv6DestUnreachableMsg *errorMsg = new ICMPv6DestUnreachableMsg("Dest Unreachable");
    errorMsg->setType(ICMPv6_DESTINATION_UNREACHABLE);
    errorMsg->setCode(code);
    return errorMsg;
}

ICMPv6Message *ICMPv6::createPacketTooBigMsg(int mtu)
{
    ICMPv6PacketTooBigMsg *errorMsg = new ICMPv6PacketTooBigMsg("Packet Too Big");
    errorMsg->setType(ICMPv6_PACKET_TOO_BIG);
    errorMsg->setCode(0);    //Set to 0 by sender and ignored by receiver.
    errorMsg->setMTU(mtu);
    return errorMsg;
}

ICMPv6Message *ICMPv6::createTimeExceededMsg(int code)
{
    ICMPv6TimeExceededMsg *errorMsg = new ICMPv6TimeExceededMsg("Time Exceeded");
    errorMsg->setType(ICMPv6_TIME_EXCEEDED);
    errorMsg->setCode(code);
    return errorMsg;
}

ICMPv6Message *ICMPv6::createParamProblemMsg(int code)
{
    ICMPv6ParamProblemMsg *errorMsg = new ICMPv6ParamProblemMsg("Parameter Problem");
    errorMsg->setType(ICMPv6_PARAMETER_PROBLEM);
    errorMsg->setCode(code);
    //TODO: What Pointer? section 3.4
    return errorMsg;
}

bool ICMPv6::validateDatagramPromptingError(IPv6Datagram *origDatagram)
{
    // don't send ICMP error messages for multicast messages
    if (origDatagram->getDestAddress().isMulticast()) {
        EV_INFO << "won't send ICMP error messages for multicast message " << origDatagram << endl;
        delete origDatagram;
        return false;
    }

    // do not reply with error message to error message
    if (origDatagram->getTransportProtocol() == IP_PROT_IPv6_ICMP) {
        ICMPv6Message *recICMPMsg = check_and_cast<ICMPv6Message *>(origDatagram->getEncapsulatedPacket());
        if (recICMPMsg->getType() < 128) {
            EV_INFO << "ICMP error received -- do not reply to it" << endl;
            delete origDatagram;
            return false;
        }
    }
    return true;
}

bool ICMPv6::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    //pingMap.clear();
    throw cRuntimeError("Lifecycle operation support not implemented");
}

void ICMPv6::handleRegisterProtocol(const Protocol& protocol, cGate *gate)
{
    Enter_Method("handleRegisterProtocol");
    if (!strcmp("transportIn", gate->getBaseName())) {
        transportProtocols.insert(ProtocolGroup::ipprotocol.getProtocolNumber(&protocol));
    }
}

} // namespace inet

