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

#ifndef __INET_MEDIUMOSGVISUALIZER_H
#define __INET_MEDIUMOSGVISUALIZER_H

#include "inet/common/visualizer/VisualizerBase.h"
#include "inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h"
#include "inet/physicallayer/contract/packetlevel/IRadioFrame.h"
#include "inet/physicallayer/contract/packetlevel/IReceptionDecision.h"
#include "inet/physicallayer/contract/packetlevel/ITransmission.h"

namespace inet {

namespace physicallayer {

class RadioMedium;

/**
 * This class provides the visualization of the communication on the radio medium.
 */
class INET_API MediumOsgVisualizer : public VisualizerBase, public RadioMedium::IMediumListener, public cListener
{
  protected:
    /** @name Parameters */
    //@{
    /**
     * The corresponding radio medium is never nullptr.
     */
    RadioMedium *radioMedium = nullptr;
    /**
     * Determines whether the visualizer displays the ongoing communications or not.
     */
    bool displayCommunication = false;
    /**
     * Displays a circle around the host submodule representing the communication range.
     */
    bool displayCommunicationRange = false;
    /**
     * Displays a circle around the host submodule representing the interference range.
     */
    bool displayInterferenceRange = false;
    /**
     * Specifies whether successful communication between radios leave a trail or not.
     */
    bool leaveCommunicationTrail = false;
    //@}

    /** @name State */
    //@{
    /**
     * Update scene interval when ongoing communication exists.
     */
    simtime_t updateSceneInterval = NaN;
    /**
     * The list of ongoing transmissions.
     */
    std::vector<const ITransmission *> transmissions;
    /**
     * The list of ongoing radio osg nodes.
     */
    std::map<const IRadio *, osg::Node *> radioOsgNodes;
    /**
     * The list of ongoing transmission osg nodes.
     */
    std::map<const ITransmission *, osg::Node *> transmissionOsgNodes;
    //@}

    /** @name Timer */
    //@{
    /**
     * The message that is used to update the scene when ongoing communications exist.
     */
    cMessage *updateSceneTimer = nullptr;
    //@}

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual osg::Node *getCachedOsgNode(const IRadio *radio) const;
    virtual void setCachedOsgNode(const IRadio *radio, osg::Node *node);
    virtual osg::Node *removeCachedOsgNode(const IRadio *radio);

    virtual osg::Node *getCachedOsgNode(const ITransmission *transmission) const;
    virtual void setCachedOsgNode(const ITransmission *transmission, osg::Node *node);
    virtual osg::Node *removeCachedOsgNode(const ITransmission *transmission);

    virtual void updateScene() const;
    virtual void scheduleUpdateSceneTimer();

  public:
    MediumOsgVisualizer() {}
    virtual ~MediumOsgVisualizer();

    virtual void mediumChanged() override;
    virtual void radioAdded(const IRadio *radio) override;
    virtual void radioRemoved(const IRadio *radio) override;
    virtual void transmissionAdded(const ITransmission *transmission) override;
    virtual void transmissionRemoved(const ITransmission *transmission) override;
    virtual void packetReceived(const IReceptionDecision *decision) override;

    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object) override;
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_MEDIUMOSGVISUALIZER_H

