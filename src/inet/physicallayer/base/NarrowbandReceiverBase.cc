//
// Copyright (C) 2013 OpenSim Ltd.
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

#include "inet/physicallayer/contract/IRadioMedium.h"
#include "inet/physicallayer/base/APSKModulationBase.h"
#include "inet/physicallayer/base/NarrowbandReceiverBase.h"
#include "inet/physicallayer/base/NarrowbandTransmissionBase.h"
#include "inet/physicallayer/base/NarrowbandReceptionBase.h"
#include "inet/physicallayer/base/NarrowbandNoiseBase.h"
#include "inet/physicallayer/common/BandListening.h"
#include "inet/physicallayer/common/ListeningDecision.h"
#include "inet/physicallayer/common/ReceptionDecision.h"

namespace inet {

namespace physicallayer {

NarrowbandReceiverBase::NarrowbandReceiverBase() :
    SNIRReceiverBase(),
    modulation(nullptr),
    carrierFrequency(Hz(NaN)),
    bandwidth(Hz(NaN))
{
}

void NarrowbandReceiverBase::initialize(int stage)
{
    SNIRReceiverBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        modulation = APSKModulationBase::findModulation(par("modulation"));
        carrierFrequency = Hz(par("carrierFrequency"));
        bandwidth = Hz(par("bandwidth"));
    }
}

void NarrowbandReceiverBase::printToStream(std::ostream& stream) const
{
    stream << "modulation = { " << modulation << " }, "
           << "carrierFrequency = " << carrierFrequency << ", "
           << "bandwidth = " << bandwidth << ", ";
    SNIRReceiverBase::printToStream(stream);
}

const IListening *NarrowbandReceiverBase::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord startPosition, const Coord endPosition) const
{
    return new BandListening(radio, startTime, endTime, startPosition, endPosition, carrierFrequency, bandwidth);
}

bool NarrowbandReceiverBase::computeIsReceptionPossible(const ITransmission *transmission) const
{
    // TODO: check if modulation matches?
    const NarrowbandTransmissionBase *narrowbandTransmission = check_and_cast<const NarrowbandTransmissionBase *>(transmission);
    return carrierFrequency == narrowbandTransmission->getCarrierFrequency() && bandwidth == narrowbandTransmission->getBandwidth();
}

// TODO: this is not purely functional, see interface comment
bool NarrowbandReceiverBase::computeIsReceptionPossible(const IListening *listening, const IReception *reception) const
{
    const BandListening *bandListening = check_and_cast<const BandListening *>(listening);
    const NarrowbandReceptionBase *narrowbandReception = check_and_cast<const NarrowbandReceptionBase *>(reception);
    if (bandListening->getCarrierFrequency() != narrowbandReception->getCarrierFrequency() || bandListening->getBandwidth() != narrowbandReception->getBandwidth()) {
        EV_DEBUG << "Computing reception possible: listening and reception bands are different -> reception is impossible" << endl;
        return false;
    }
    else
        return true;
}

const IReceptionDecision *NarrowbandReceiverBase::computeReceptionDecision(const IListening *listening, const IReception *reception, const IInterference *interference) const
{
    const BandListening *bandListening = check_and_cast<const BandListening *>(listening);
    const NarrowbandReceptionBase *narrowbandReception = check_and_cast<const NarrowbandReceptionBase *>(reception);
    if (bandListening->getCarrierFrequency() == narrowbandReception->getCarrierFrequency() && bandListening->getBandwidth() == narrowbandReception->getBandwidth())
        return SNIRReceiverBase::computeReceptionDecision(listening, reception, interference);
    else
        return new ReceptionDecision(reception, new ReceptionIndication(), false, false, false);
}

} // namespace physicallayer

} // namespace inet
