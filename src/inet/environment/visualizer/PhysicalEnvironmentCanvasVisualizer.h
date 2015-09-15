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

#ifndef __INET_PHYSICALENVIRONMENTCANVASVISUALIZER_H
#define __INET_PHYSICALENVIRONMENTCANVASVISUALIZER_H

#include "inet/common/geometry/common/CanvasProjection.h"
#include "inet/common/visualizer/VisualizerBase.h"
#include "inet/environment/common/PhysicalEnvironment.h"

namespace inet {

namespace physicalenvironment {

class INET_API PhysicalEnvironmentCanvasVisualizer : public VisualizerBase
{
  protected:
    class ObjectPositionComparator
    {
      protected:
        const Rotation &viewRotation;

      public:
        ObjectPositionComparator(const Rotation &viewRotation) : viewRotation(viewRotation) {}

        bool operator() (const PhysicalObject *left, const PhysicalObject *right) const
        {
            return viewRotation.rotateVectorClockwise(left->getPosition()).z < viewRotation.rotateVectorClockwise(right->getPosition()).z;
        }
    };

  protected:
    /** @name Context */
    //@{
    const PhysicalEnvironment *physicalEnvironment = nullptr;
    double axisLength = NaN;
    //@}

    /** @name Internal state */
    //@{
    /**
     * The 2D projection used on the canvas.
     */
    CanvasProjection canvasProjection;
    //@}

    /** @name Graphics */
    //@{
    cGroupFigure *objectsLayer = nullptr;
    //@}

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleParameterChange(const char *name) override;

    virtual void updateCanvas();

    virtual EulerAngles computeViewAngle(const char *viewAngle);
    virtual cFigure::Point computeViewTranslation(const char *viewTranslation);
    virtual void computeFacePoints(const PhysicalObject *object, std::vector<std::vector<Coord> >& faces, const Rotation& rotation);

  public:
    PhysicalEnvironmentCanvasVisualizer() {}
    virtual ~PhysicalEnvironmentCanvasVisualizer() {}
};

} // namespace physicalenvironment

} // namespace inet

#endif // ifndef __INET_PHYSICALENVIRONMENTCANVASVISUALIZER_H

