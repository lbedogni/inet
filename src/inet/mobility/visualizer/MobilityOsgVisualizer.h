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

#ifndef __INET_MOBILITYOSGVISUALIZER_H
#define __INET_MOBILITYOSGVISUALIZER_H

#include "inet/common/visualizer/VisualizerBase.h"
#include "inet/mobility/contract/IMobility.h"

namespace inet {

class INET_API MobilityOsgVisualizer : public VisualizerBase, public cListener
{
  protected:
    std::map<const IMobility *, osg::PositionAttitudeTransform *> positionAttitudeTransforms;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    virtual osg::PositionAttitudeTransform* ensureOsgNode(const IMobility* mobility);

  public:
    MobilityOsgVisualizer() {}
    virtual ~MobilityOsgVisualizer();

    static osg::PositionAttitudeTransform *createOsgNode(const IMobility *mobility);
    static void setPosition(const Coord& position, osg::PositionAttitudeTransform *positionAttitudeTransform);

    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object) override;

};

} // namespace inet

#endif // ifndef __INET_MOBILITYOSGVISUALIZER_H

