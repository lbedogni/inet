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

#ifndef __INET_CANVASPROJECTION_H
#define __INET_CANVASPROJECTION_H

#include "inet/common/geometry/common/Rotation.h"

namespace inet {

class INET_API CanvasProjection
{
  protected:
    Rotation rotation;
    cFigure::Point translation;

    static CanvasProjection defaultCanvasProjection;
    static std::map<const cCanvas *, const CanvasProjection *> canvasProjections;

  public:
    CanvasProjection() {}
    CanvasProjection(Rotation rotation, cFigure::Point translation);

    const Rotation& getRotation() { return rotation; }
    void setRotation(const Rotation& rotation) { this->rotation = rotation; }

    const cFigure::Point& getTranslation() { return translation; }
    void setTranslation(const cFigure::Point& translation) { this->translation = translation; }

    cFigure::Point computeCanvasPoint(const Coord& point) const;

    static const CanvasProjection *getCanvasProjection(const cCanvas *canvas);
    static void setCanvasProjection(const cCanvas *canvas, const CanvasProjection *canvasProjection);
};

} // namespace inet

#endif // ifndef __INET_CANVASPROJECTION_H

