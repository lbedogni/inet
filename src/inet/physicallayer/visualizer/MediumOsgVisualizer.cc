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
#include "inet/physicallayer/common/packetlevel/RadioMedium.h"
#include "inet/physicallayer/visualizer/MediumOsgVisualizer.h"
#include <osg/Depth>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Material>
#include <osg/ShapeDrawable>

namespace inet {

namespace physicallayer {

Define_Module(MediumOsgVisualizer);

MediumOsgVisualizer::~MediumOsgVisualizer()
{
    cancelAndDelete(updateSceneTimer);
}

void MediumOsgVisualizer::initialize(int stage)
{
    VisualizerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        radioMedium = getModuleFromPar<RadioMedium>(par("mediumModule"), this);
        radioMedium->addListener(this);
        displayCommunication = par("displayCommunication");
        leaveCommunicationTrail = par("leaveCommunicationTrail");
        updateSceneInterval = par("updateSceneInterval");
        updateSceneTimer = new cMessage("updateScene");
    }
    else if (stage == INITSTAGE_LAST) {
        auto scene = inet::osg::getScene(visualizerTargetModule);
        scene->addChild(new osg::Group());
    }
}

void MediumOsgVisualizer::handleMessage(cMessage *message)
{
    if (message == updateSceneTimer) {
        updateScene();
        scheduleUpdateSceneTimer();
    }
    else
        throw cRuntimeError("Unknown message");
}

void MediumOsgVisualizer::mediumChanged()
{
    if (displayCommunication)
        updateScene();
}

void MediumOsgVisualizer::transmissionAdded(const ITransmission *transmission)
{
    if (displayCommunication) {
        transmissions.push_back(transmission);
        ICommunicationCache *communicationCache = const_cast<ICommunicationCache *>(radioMedium->getCommunicationCache());
        auto transmissionStart = transmission->getStartPosition();
        auto drawable = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(transmissionStart.x, transmissionStart.y, transmissionStart.z), 1));
        auto depth = new osg::Depth();
        depth->setWriteMask(false);
        auto stateSet = drawable->getOrCreateStateSet();
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(depth, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        cFigure::Color color = cFigure::GOOD_DARK_COLORS[transmission->getId() % (sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color))];
#else
        cFigure::Color color(64 + rand() % 64, 64 + rand() % 64, 64 + rand() % 64);
#endif
        auto materialColor = osg::Vec4(color.red / 255.0, color.green / 255.0, color.blue / 255.0, 0.5);
        auto material = new osg::Material();
        material->setEmission(osg::Material::FRONT_AND_BACK, materialColor);
        material->setDiffuse(osg::Material::FRONT_AND_BACK, materialColor);
        material->setAmbient(osg::Material::FRONT_AND_BACK, materialColor);
        material->setAlpha(osg::Material::FRONT_AND_BACK, 0.5);
        stateSet->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE);
        auto scene = inet::osg::getScene(visualizerTargetModule);
        auto geode = new osg::Geode();
        geode->addDrawable(drawable);
        scene->addChild(geode);
        communicationCache->setCachedOsgNode(transmission, geode);
        updateScene();
        if (updateSceneInterval > 0 && !updateSceneTimer->isScheduled())
            scheduleUpdateSceneTimer();
    }
}

void MediumOsgVisualizer::transmissionRemoved(const ITransmission *transmission)
{
    if (displayCommunication) {
        transmissions.erase(std::remove(transmissions.begin(), transmissions.end(), transmission));
        ICommunicationCache *communicationCache = const_cast<ICommunicationCache *>(radioMedium->getCommunicationCache());
        // TODO: this is too late to remove the transmission sphere because the end transmission is completely ignored
        auto node = communicationCache->getCachedOsgNode(transmission);
        if (node != nullptr)
            node->getParent(0)->removeChild(node);
    }
}

void MediumOsgVisualizer::packetReceived(const IReceptionDecision *decision)
{
    if (decision->isReceptionSuccessful()) {
        auto scene = inet::osg::getScene(visualizerTargetModule);
        const ITransmission *transmission = decision->getReception()->getTransmission();
        const IReception *reception = decision->getReception();
        if (leaveCommunicationTrail) {
            Coord transmissionPosition = transmission->getStartPosition();
            Coord receptionPosition = reception->getStartPosition();
            Coord centerPosition = (transmissionPosition + receptionPosition) / 2;
            osg::Geometry* linesGeom = new osg::Geometry();
            osg::Vec3Array* vertexData = new osg::Vec3Array();
            vertexData->push_back(osg::Vec3(transmissionPosition.x, transmissionPosition.y, transmissionPosition.z));
            vertexData->push_back(osg::Vec3(receptionPosition.x, receptionPosition.y, receptionPosition.z));
            osg::DrawArrays* drawArrayLines = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP);
            drawArrayLines->setFirst(0);
            drawArrayLines->setCount(vertexData->size());
            linesGeom->setVertexArray(vertexData);
            linesGeom->addPrimitiveSet(drawArrayLines);
            osg::Geode* geode = new osg::Geode();
            geode->addDrawable(linesGeom);
            scene->addChild(geode);
        }
    }
    if (displayCommunication)
        updateScene();
}

void MediumOsgVisualizer::updateScene() const
{
    const IPropagation *propagation = radioMedium->getPropagation();
    ICommunicationCache *communicationCache = const_cast<ICommunicationCache *>(radioMedium->getCommunicationCache());
    for (const auto transmission : transmissions) {
        double startRadius = propagation->getPropagationSpeed().get() * (simTime() - transmission->getStartTime()).dbl();
        // TODO: use endRadius
        double endRadius = propagation->getPropagationSpeed().get() * (simTime() - transmission->getEndTime()).dbl();
        auto node = communicationCache->getCachedOsgNode(transmission);
        if (node != nullptr) {
            auto drawable = static_cast<osg::Geode *>(node)->getDrawable(0);
            auto shape = static_cast<osg::Sphere *>(drawable->getShape());
            shape->setRadius(startRadius);
            drawable->dirtyDisplayList();
            drawable->dirtyBound();
        }
    }
}

void MediumOsgVisualizer::scheduleUpdateSceneTimer()
{
    simtime_t now = simTime();
    ICommunicationCache *communicationCache = const_cast<ICommunicationCache *>(radioMedium->getCommunicationCache());
    const IMediumLimitCache *mediumLimitCache = radioMedium->getMediumLimitCache();
    for (const auto transmission : transmissions) {
        const simtime_t startTime = transmission->getStartTime();
        const simtime_t endTime = transmission->getEndTime();
        simtime_t maxPropagationTime = communicationCache->getCachedInterferenceEndTime(transmission) - endTime - mediumLimitCache->getMaxTransmissionDuration();
        if ((startTime <= now && now <= startTime + maxPropagationTime) ||
            (endTime <= now && now <= endTime + maxPropagationTime))
        {
            scheduleAt(simTime() + updateSceneInterval, updateSceneTimer);
            return;
        }
    }
    simtime_t nextEndTime = -1;
    for (const auto transmission : transmissions) {
        const simtime_t endTime = transmission->getEndTime();
        if (endTime > now && (nextEndTime == -1 || endTime < nextEndTime))
            nextEndTime = endTime;
    }
    if (nextEndTime > now)
        scheduleAt(nextEndTime, updateSceneTimer);
}

} // namespace physicallayer

} // namespace inet

