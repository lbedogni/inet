#ifndef __INET_L3ERRORCONTROLINFO_H
#define __INET_L3ERRORCONTROLINFO_H


#include "inet/networklayer/contract/L3Error_m.h"

#include "inet/common/ProtocolGroup.h"
#include "inet/common/IPacketControlInfo.h"

namespace inet {

class INET_API L3ErrorControlInfo : public L3ErrorControlInfo_Base, public IPacketControlInfo
{
  public:
    L3ErrorControlInfo() : L3ErrorControlInfo_Base() {}
    L3ErrorControlInfo(const L3ErrorControlInfo& other) : L3ErrorControlInfo_Base(other) {}
    L3ErrorControlInfo& operator=(const L3ErrorControlInfo& other) {if (this==&other) return *this; L3ErrorControlInfo_Base::operator=(other); return *this;}
    virtual L3ErrorControlInfo *dup() const override {return new L3ErrorControlInfo(*this);}

    virtual L3Address getSourceAddress() const override { return _getSourceAddress(); }
    virtual L3Address getDestinationAddress() const override { return _getDestinationAddress(); }
    virtual int getPacketProtocolId() const override { return ProtocolGroup::ipprotocol.getProtocol(getTransportProtocol())->getId(); }
};

} // namespace inet

#endif // ifndef __INET_L3ERRORCONTROLINFO_H

