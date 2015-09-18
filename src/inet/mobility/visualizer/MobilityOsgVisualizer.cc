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
#include "inet/common/OSGUtils.h"
#include "inet/mobility/visualizer/MobilityOsgVisualizer.h"
#include <osg/AutoTransform>
#include <osg/Material>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgText/Text>

namespace inet {

Define_Module(MobilityOsgVisualizer);

MobilityOsgVisualizer::~MobilityOsgVisualizer()
{
    if (hasGUI())
        visualizerTargetModule->unsubscribe(IMobility::mobilityStateChangedSignal, this);
}

void MobilityOsgVisualizer::initialize(int stage)
{
    VisualizerBase::initialize(stage);
    if (hasGUI()) {
        if (stage == INITSTAGE_LOCAL)
            visualizerTargetModule->subscribe(IMobility::mobilityStateChangedSignal, this);
    }
}

osg::PositionAttitudeTransform* MobilityOsgVisualizer::createOsgNode(const IMobility *mobility)
{
    auto module = const_cast<cModule *>(check_and_cast<const cModule *>(mobility));
    auto networkNode = getModuleFromPar<cModule>(module->par("visualizerTargetModule"), module);
    osg::Node *osgNode = nullptr;
    if (networkNode->hasPar("visualization") && strlen(networkNode->par("visualization")) != 0) {
        const char *visualization = networkNode->par("visualization");
        osgNode = osgDB::readNodeFile(networkNode->par("visualization"));
        if (osgNode == nullptr)
            throw cRuntimeError("Model file \"%s\" not found", visualization);
    }
    else {
        // TODO: get real images path
        std::string path("/home/levy/workspace/omnetpp/images/");
        path += networkNode->getDisplayString().getTagArg("i", 0);
        path += ".png";
        auto image = osgDB::readImageFile(path.c_str());
        auto texture = new osg::Texture2D();
        texture->setImage(image);
        auto geometry = osg::createTexturedQuadGeometry(osg::Vec3(0.0, 0.0, 0.0), osg::Vec3(image->s(), 0.0, 0.0),
                osg::Vec3(0.0, image->t(), 0.0), 0.0, 0.0, 1.0, 1.0);
        geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture);
        geometry->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        geometry->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        geometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        auto text = new osgText::Text();
        text->setColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        text->setCharacterSize(18);
        text->setText(networkNode->getFullName());
        auto geode = new osg::Geode();
        geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        geode->addDrawable(geometry);
        geode->addDrawable(text);
        auto autoTransform = new osg::AutoTransform();
        autoTransform->setPivotPoint(osg::Vec3d(0.5, 0.5, 0.0));
        autoTransform->setAutoScaleToScreen(true);
        autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
        autoTransform->addChild(geode);
        autoTransform->setPosition(osg::Vec3d(0.0, 0.0, 0.0));
        osgNode = autoTransform;
    }
    auto objectNode = cOsgCanvas::createOmnetppObjectNode(module);
    objectNode->addChild(osgNode);
    auto positionAttitudeTransform = new osg::PositionAttitudeTransform();
    positionAttitudeTransform->addChild(objectNode);
    return positionAttitudeTransform;
}

osg::PositionAttitudeTransform* MobilityOsgVisualizer::ensureOsgNode(const IMobility *mobility)
{
    auto it = positionAttitudeTransforms.find(mobility);
    if (it != positionAttitudeTransforms.end())
        return it->second;
    else {
        auto positionAttitudeTransform = createOsgNode(mobility);
        auto scene = inet::osg::getScene(visualizerTargetModule);
        scene->addChild(positionAttitudeTransform);
        positionAttitudeTransforms[mobility] = positionAttitudeTransform;
        return positionAttitudeTransform;
    }
}

void MobilityOsgVisualizer::setPosition(const Coord& position, osg::PositionAttitudeTransform *positionAttitudeTransform)
{
    positionAttitudeTransform->setPosition(osg::Vec3d(position.x, position.y, position.z));
}

void MobilityOsgVisualizer::receiveSignal(cComponent *source, simsignal_t signal, cObject *object)
{
    if (hasGUI() && signal == IMobility::mobilityStateChangedSignal) {
        auto mobility = dynamic_cast<IMobility *>(object);
        auto positionAttitudeTransform = ensureOsgNode(mobility);
        setPosition(mobility->getCurrentPosition(), positionAttitudeTransform);
    }
}

} // namespace inet

