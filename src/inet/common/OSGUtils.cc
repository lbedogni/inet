//
// Copyright (C) 2006-2015 Opensim Ltd
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

#include "inet/common/OSGUtils.h"

namespace inet {

namespace osg {

Group *getScene(cModule *module)
{
    auto osgCanvas = module->getOsgCanvas();
    if (osgCanvas->getScene() == nullptr) {
        auto scene = new osg::Group();
        osgCanvas->setCameraManipulatorType(cOsgCanvas::CAM_TRACKBALL);
        osgCanvas->setScene(scene);
    }
    return static_cast<Group *>(osgCanvas->getScene());
}

Geometry *createCircleGeometry(const Coord& center, double radius, int polygonSize)
{
    auto geometry = new Geometry();
    auto vertices = new Vec3Array();
    double theta, px, py;
    for (int i = 1; i <= polygonSize; i++) {
        theta = 2.0 * M_PI / polygonSize * i;
        px = center.x + radius * cos(theta);
        py = center.y + radius * sin(theta);
        vertices->push_back(Vec3d(px, py, center.z));
    }
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new DrawArrays(PrimitiveSet::LINE_LOOP, 0, polygonSize));
    return geometry;
}

Geometry *createPolygonGeometry(const std::vector<Coord>& points, StateSet *stateSet, const Coord& translation)
{
    auto geometry = new Geometry();
    auto vertices = new Vec3Array();
    for (auto point : points)
        vertices->push_back(Vec3d(point.x + translation.x, point.y + translation.y, point.z + translation.z));
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new DrawArrays(PrimitiveSet::POLYGON, 0, points.size()));
    if (stateSet != nullptr)
        geometry->setStateSet(stateSet);
    return geometry;
}

AutoTransform *createAutoTransform(Drawable *drawable, AutoTransform::AutoRotateMode mode)
{
    auto geode = new Geode();
    geode->addDrawable(drawable);
    auto autoTransform = new AutoTransform();
    autoTransform->setAutoRotateMode(mode);
    autoTransform->addChild(geode);
    return autoTransform;
}

PositionAttitudeTransform *createPositionAttitudeTransform(const Coord& position, const EulerAngles& orientation)
{
    auto pat = new PositionAttitudeTransform();
    pat->setPosition(Vec3d(position.x, position.y, position.z));
    pat->setAttitude(Quat(orientation.alpha, Vec3d(1.0, 0.0, 0.0)) *
                     Quat(orientation.beta, Vec3d(0.0, 1.0, 0.0)) *
                     Quat(orientation.gamma, Vec3d(0.0, 0.0, 1.0)));
    return pat;
}

StateSet *createStateSet(const cFigure::Color& color, double opacity)
{
    auto stateSet = new StateSet();
    auto material = new Material();
    Vec4 colorVec((double)color.red / 255.0, (double)color.green / 255.0, (double)color.blue / 255.0, opacity);
    material->setDiffuse(Material::FRONT_AND_BACK, colorVec);
    material->setAlpha(Material::FRONT_AND_BACK, opacity);
    stateSet->setAttribute(material);
    stateSet->setMode(GL_BLEND, StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    return stateSet;
}

} // namespace osg

} // namespace inet

