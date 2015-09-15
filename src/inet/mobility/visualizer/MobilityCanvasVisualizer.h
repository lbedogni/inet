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

#ifndef __INET_MOBILITYCANVASVISUALIZER_H
#define __INET_MOBILITYCANVASVISUALIZER_H

#include "inet/common/visualizer/VisualizerBase.h"
#include "inet/common/figures/TrailFigure.h"
#include "inet/common/geometry/common/CanvasProjection.h"
#include "inet/mobility/contract/IMobility.h"

namespace inet {

class INET_API MobilityCanvasVisualizer : public VisualizerBase, public cListener
{
  protected:
    /**
     * The 2D projection used on the canvas.
     */
    const CanvasProjection *canvasProjection;

    simtime_t updateInterval;

    /** @brief Draw the path on the canvas. */
    bool leaveMovementTrail = false;

    /** @brief The list of trail figures representing the movement. */
    TrailFigure *movementTrail = nullptr;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

  public:
    MobilityCanvasVisualizer() {}
    virtual ~MobilityCanvasVisualizer();

    static void setPosition(const Coord& position, cModule* visualRepresentation, const CanvasProjection *canvasProjection);

    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object) override;

};

} // namespace inet

#endif // ifndef __INET_MOBILITYCANVASVISUALIZER_H

