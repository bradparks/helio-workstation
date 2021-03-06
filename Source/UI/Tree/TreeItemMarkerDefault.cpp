/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

//[Headers]
#include "Common.h"
//[/Headers]

#include "TreeItemMarkerDefault.h"

//[MiscUserDefs]
#include "HelioTheme.h"
//[/MiscUserDefs]

TreeItemMarkerDefault::TreeItemMarkerDefault()
{

    //[UserPreSize]
    this->setInterceptsMouseClicks(false, false);
    //[/UserPreSize]

    setSize (32, 32);

    //[Constructor]
    //[/Constructor]
}

TreeItemMarkerDefault::~TreeItemMarkerDefault()
{
    //[Destructor_pre]
    //[/Destructor_pre]


    //[Destructor]
    //[/Destructor]
}

void TreeItemMarkerDefault::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x0fffffff));
    g.fillRect (getWidth() - 3, 0, 3, getHeight() - 0);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void TreeItemMarkerDefault::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}


//[MiscUserCode]
//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TreeItemMarkerDefault" template="../../Template"
                 componentName="" parentClasses="public Component" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="1" initialWidth="32" initialHeight="32">
  <BACKGROUND backgroundColour="0">
    <RECT pos="0Rr 0 3 0M" fill="solid: fffffff" hasStroke="0"/>
  </BACKGROUND>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
