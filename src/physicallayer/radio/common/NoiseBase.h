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

#ifndef __INET_NOISEBASE_H_
#define __INET_NOISEBASE_H_

#include "INoise.h"

namespace radio
{

class INET_API NoiseBase : public INoise
{
    protected:
        const simtime_t startTime;
        const simtime_t endTime;

    public:
        NoiseBase(simtime_t startTime, simtime_t endTime) :
            startTime(startTime),
            endTime(endTime)
        {}

        virtual const simtime_t getStartTime() const { return startTime; }
        virtual const simtime_t getEndTime() const { return endTime; }
};

}

#endif