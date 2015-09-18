//
// Copyright (C) 2006 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

package inet.examples.visualization;

import inet.common.visualizer.CanvasVisualizer;
import inet.common.visualizer.OsgVisualizer;
import inet.environment.common.PhysicalEnvironment;
import inet.environment.visualizer.PhysicalEnvironmentCanvasVisualizer;
import inet.environment.visualizer.PhysicalEnvironmentOsgVisualizer;
import inet.mobility.single.RandomWPMobility;
import inet.mobility.visualizer.MobilityCanvasVisualizer;
import inet.mobility.visualizer.MobilityOsgVisualizer;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.AdhocHost;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.physicallayer.visualizer.MediumCanvasVisualizer;
import inet.physicallayer.visualizer.MediumOsgVisualizer;


network PhysicalEnvironmentVisualizationTest
{
    parameters:
        @display("bgb=0,0");
    submodules:
        environment: PhysicalEnvironment {
            parameters:
                @display("p=150,50");
        }
        osg: PhysicalEnvironmentOsgVisualizer {
            parameters:
                @display("p=150,150");
        }
        isometric: PhysicalEnvironmentCanvasVisualizer {
            parameters:
                @display("p=250,150");
        }
        top: PhysicalEnvironmentCanvasVisualizer {
            parameters:
                @display("p=150,250");
        }
        front: PhysicalEnvironmentCanvasVisualizer {
            parameters:
                @display("p=250,250");
        }
        side: PhysicalEnvironmentCanvasVisualizer {
            parameters:
                @display("p=350,250");
        }
}

network MobilityVisualizationTest
{
    parameters:
        @display("bgb=0,0");
        int numMobilities;
    submodules:
        mobility[numMobilities]: RandomWPMobility {
            parameters:
                @display("r=,,#707070;p=250,50;i=block/cogwheel");
        }
        osg: MobilityOsgVisualizer {
            parameters:
                @display("p=150,150");
        }
        isometric: MobilityCanvasVisualizer {
            parameters:
                @display("p=250,150");
        }
        top: MobilityCanvasVisualizer {
            parameters:
                @display("p=150,250");
        }
        front: MobilityCanvasVisualizer {
            parameters:
                @display("p=250,250");
        }
        side: MobilityCanvasVisualizer {
            parameters:
                @display("p=350,250");
        }
}

network MediumVisualizationTest
{
    parameters:
        @display("bgb=0,0");
        int numHosts;
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=50,50");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                @display("p=50,150");
        }
        host[numHosts]: AdhocHost {
            parameters:
                @display("p=250,50");
        }
        osg: MediumOsgVisualizer {
            parameters:
                @display("p=150,150");
        }
        isometric: MediumCanvasVisualizer {
            parameters:
                @display("p=250,150");
        }
        top: MediumCanvasVisualizer {
            parameters:
                @display("p=150,250");
        }
        front: MediumCanvasVisualizer {
            parameters:
                @display("p=250,250");
        }
        side: MediumCanvasVisualizer {
            parameters:
                @display("p=350,250");
        }
}

network IntegratedVisualizationTest
{
    parameters:
        @display("bgb=0,0");
        int numHosts;
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=50,50");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                @display("p=50,150");
        }
        host[numHosts]: AdhocHost {
            parameters:
                @display("p=250,50");
        }
        osg: OsgVisualizer {
            parameters:
                @display("p=150,150");
        }
        isometric: CanvasVisualizer {
            parameters:
                @display("p=250,150");
        }
        top: CanvasVisualizer {
            parameters:
                @display("p=150,250");
        }
        front: CanvasVisualizer {
            parameters:
                @display("p=250,250");
        }
        side: CanvasVisualizer {
            parameters:
                @display("p=350,250");
        }
}
