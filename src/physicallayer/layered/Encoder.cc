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

#include "Encoder.h"
#include "BitVector.h"
#include "ModuleAccess.h"

namespace inet {

namespace physicallayer {

Define_Module(Encoder);

void Encoder::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        serializer = check_and_cast<ISerializer *>(getSubmodule("serializer"));
        scrambler = dynamic_cast<IScrambler *>(getSubmodule("scrambler"));
        forwardErrorCorrection = dynamic_cast<IForwardErrorCorrection *>(getSubmodule("forwardErrorCorrection"));
        interleaver = dynamic_cast<IInterleaver *>(getSubmodule("interleaver"));
    }
}

const ITransmissionBitModel *Encoder::encode(const ITransmissionPacketModel *packetModel) const
{
    const int bitLength = headerBitLength + packetModel->getPacket()->getBitLength();
    const cPacket *packet = packetModel->getPacket();
    BitVector serializedPacket = serializer->serialize(packet);
    BitVector scrambledBits = scrambler->scrambling(serializedPacket);
    BitVector fecEncodedBits = forwardErrorCorrection->encode(scrambledBits);
    BitVector interleavedBits = interleaver->interleaving(fecEncodedBits);
    return new TransmissionBitModel(bitLength, bitRate, interleavedBits, NULL, forwardErrorCorrection);
}

} // namespace physicallayer

} // namespace inet