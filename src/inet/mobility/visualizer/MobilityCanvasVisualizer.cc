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

#include "inet/common/ModuleAccess.h"
#include "inet/mobility/visualizer/MobilityCanvasVisualizer.h"

namespace inet {

Define_Module(MobilityCanvasVisualizer);

MobilityCanvasVisualizer::~MobilityCanvasVisualizer()
{
    if (hasGUI())
        visualizerTargetModule->unsubscribe(IMobility::mobilityStateChangedSignal, this);
}

void MobilityCanvasVisualizer::initialize(int stage)
{
    VisualizerBase::initialize(stage);
    if (hasGUI()) {
        if (stage == INITSTAGE_LOCAL) {
            updateInterval = par("updateInterval");
            leaveMovementTrail = par("leaveMovementTrail");
            if (leaveMovementTrail) {
                movementTrail = new TrailFigure(100, true, "movement trail");
                cCanvas *canvas = visualizerTargetModule->getCanvas();
                canvas->addFigureBelow(movementTrail, canvas->getSubmodulesLayer());
            }
            visualizerTargetModule->subscribe(IMobility::mobilityStateChangedSignal, this);
        }
        else if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT)
            canvasProjection = CanvasProjection::getCanvasProjection(visualizerTargetModule->getCanvas());
    }
}

void MobilityCanvasVisualizer::setPosition(const Coord& position, cModule* visualRepresentation, const CanvasProjection *canvasProjection)
{
    cFigure::Point point = canvasProjection->computeCanvasPoint(position);
    char buf[32];
    snprintf(buf, sizeof(buf), "%lf", point.x);
    buf[sizeof(buf) - 1] = 0;
    visualRepresentation->getDisplayString().setTagArg("p", 0, buf);
    snprintf(buf, sizeof(buf), "%lf", point.y);
    buf[sizeof(buf) - 1] = 0;
    visualRepresentation->getDisplayString().setTagArg("p", 1, buf);
}

void MobilityCanvasVisualizer::receiveSignal(cComponent *source, simsignal_t signal, cObject *object)
{
    if (hasGUI() && signal == IMobility::mobilityStateChangedSignal) {
        auto mobility = dynamic_cast<IMobility *>(object);
        auto module = dynamic_cast<cModule*>(mobility);
        setPosition(mobility->getCurrentPosition(), getModuleFromPar<cModule>(module->par("visualizerTargetModule"), module), canvasProjection);
        if (leaveMovementTrail) {
            const Coord position = mobility->getCurrentPosition();
            const Coord speed = mobility->getCurrentSpeed();
            cFigure::Point startPosition;
            if (movementTrail->getNumFigures() == 0)
                startPosition = canvasProjection->computeCanvasPoint(position);
            else
                startPosition = static_cast<cLineFigure *>(movementTrail->getFigure(movementTrail->getNumFigures() - 1))->getEnd();
            cFigure::Point endPosition = canvasProjection->computeCanvasPoint(position);
            double dx = startPosition.x - endPosition.x;
            double dy = startPosition.y - endPosition.y;
            if (movementTrail->getNumFigures() == 0 || dx * dx + dy * dy > (speed * updateInterval.dbl()).squareLength()) {
                cLineFigure *movementLine = new cLineFigure();
                movementLine->setTags("movement_trail recent_history");
                movementLine->setStart(startPosition);
                movementLine->setEnd(endPosition);
                movementLine->setLineWidth(1);
#if OMNETPP_CANVAS_VERSION >= 0x20140908
                cFigure::Color color = cFigure::GOOD_DARK_COLORS[getId() % (sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color))];
                movementLine->setLineColor(color);
                movementLine->setScaleLineWidth(false);
#else
                movementLine->setLineColor(cFigure::BLACK);
#endif
                movementTrail->addFigure(movementLine);
            }
        }
    }
}

} // namespace inet

