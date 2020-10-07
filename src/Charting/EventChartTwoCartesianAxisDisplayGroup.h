#ifndef __EVENT_CHART_TWO_CARTESIAN_AXIS_DISPLAY_GROUP_H__
#define __EVENT_CHART_TWO_CARTESIAN_AXIS_DISPLAY_GROUP_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2020 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/



#include <memory>
#include "DisplayGroupEnum.h"
#include "Event.h"



namespace caret {

    class ChartTwoCartesianAxis;
    
    class EventChartTwoCartesianAxisDisplayGroup : public Event {
        
    public:
        EventChartTwoCartesianAxisDisplayGroup(const DisplayGroupEnum::Enum displayGroup);
        
        virtual ~EventChartTwoCartesianAxisDisplayGroup();
        
        DisplayGroupEnum::Enum getDisplayGroup() const;
        
        ChartTwoCartesianAxis* getAxis();
        
        const ChartTwoCartesianAxis* getAxis() const;
        
        void setAxis(ChartTwoCartesianAxis* axis);
        
        EventChartTwoCartesianAxisDisplayGroup(const EventChartTwoCartesianAxisDisplayGroup&) = delete;

        EventChartTwoCartesianAxisDisplayGroup& operator=(const EventChartTwoCartesianAxisDisplayGroup&) = delete;
        

        // ADD_NEW_METHODS_HERE

    private:
        const DisplayGroupEnum::Enum m_displayGroup;
        
        ChartTwoCartesianAxis* m_axis = NULL;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __EVENT_CHART_TWO_CARTESIAN_AXIS_DISPLAY_GROUP_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __EVENT_CHART_TWO_CARTESIAN_AXIS_DISPLAY_GROUP_DECLARE__

} // namespace
#endif  //__EVENT_CHART_TWO_CARTESIAN_AXIS_DISPLAY_GROUP_H__
