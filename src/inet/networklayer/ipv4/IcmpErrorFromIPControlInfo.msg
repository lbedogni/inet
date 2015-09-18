//
// Copyright (C) 2015 OpenSim Ltd.
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
// @author Zoltan Bojthe
//

cplusplus {{
#include "inet/common/IPacketControlInfo.h"
#include "inet/common/Protocol.h"
#include "inet/networklayer/common/IPProtocolId_m.h"
#include "inet/networklayer/ipv4/ICMPMessage_m.h"
}}

namespace inet;

enum ICMPType;

enum IcmpErrorDirection {
    TO_NETWORK = 1;
    TO_LOCAL = 2;
};

class IcmpErrorFromIPControlInfo
{
    @implements(IPacketControlInfo);
    int packetProtocolId = Protocol::icmpv4.getId();
    int direction @enum(IcmpErrorDirection) = 0;
    int interfaceId = -1;
    int icmpType @enum(ICMPType) = -1;
    int icmpCode = -1;
}

