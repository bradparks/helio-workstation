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

#include "TooltipContainer.h"

//[MiscUserDefs]
#define TIMER_MILLISECONDS 100
//[/MiscUserDefs]

TooltipContainer::TooltipContainer()
    : hideTimeout(-1),
      tooltipComponent(nullptr)
{
    addAndMakeVisible (tooltipComponent = new Component());


    //[UserPreSize]
    this->setVisible(false);
    //[/UserPreSize]

    setSize (450, 80);

    //[Constructor]
    //[/Constructor]
}

TooltipContainer::~TooltipContainer()
{
    //[Destructor_pre]
    //[/Destructor_pre]

    tooltipComponent = nullptr;

    //[Destructor]
    //[/Destructor]
}

void TooltipContainer::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x90000000));
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (getWidth() - 0), static_cast<float> (getHeight() - 0), 15.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void TooltipContainer::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    tooltipComponent->setBounds (0, 0, getWidth() - 0, getHeight() - 0);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void TooltipContainer::parentHierarchyChanged()
{
    //[UserCode_parentHierarchyChanged] -- Add your code here...
    this->updatePosition();
    //[/UserCode_parentHierarchyChanged]
}

void TooltipContainer::parentSizeChanged()
{
    //[UserCode_parentSizeChanged] -- Add your code here...
    this->updatePosition();
    //[/UserCode_parentSizeChanged]
}


//[MiscUserCode]

void TooltipContainer::updatePosition()
{
    this->setCentrePosition(this->getParentWidth() / 2,
                            this->alignedToBottom ?
                            (this->getParentHeight() - int(this->getHeight() / 2) - 15) :
                            ((this->getHeight() / 2) + 15)
                            );
}

void TooltipContainer::timerCallback()
{
    const int clicksCount = Desktop::getInstance().getMouseButtonClickCounter();

    if (clicksCount > this->clicksCountOnStart)
    {
        this->hide();
    }

    this->timeCounter += TIMER_MILLISECONDS;

    if (this->timeCounter > this->hideTimeout)
    {
        this->hide();
    }
}

void TooltipContainer::showWithComponent(Component *newTargetComponent, int timeOutMs)
{
    const Point<int> clickOrigin =
    Desktop::getInstance().getMainMouseSource().getLastMouseDownPosition().toInt();

    this->showWithComponent(newTargetComponent,
                            Rectangle<int>(clickOrigin, clickOrigin.translated(1, 1)),
                            timeOutMs);
}

void TooltipContainer::showWithComponent(Component *newTargetComponent,
                                         Rectangle<int> callerScreenBounds,
                                         int timeOutMs)
{
    if (newTargetComponent == nullptr)
    {
        this->hide();
        return;
    }

    const Point<int> callerOrigin = callerScreenBounds.getCentre();

    const Point<int> topLevelOrigin =
    this->getTopLevelComponent()->getScreenPosition();

    this->alignedToBottom =
    (callerOrigin - topLevelOrigin).getY() < (this->getTopLevelComponent()->getHeight() / 2);

    this->clicksCountOnStart = Desktop::getInstance().getMouseButtonClickCounter();
    this->timeCounter = 0;

    if (!this->isVisible())
    {
        this->setVisible(true);
        this->setAlpha(1.f);
    }

    this->animator.cancelAllAnimations(false);
    this->removeChildComponent(this->tooltipComponent);
    this->stopTimer();

    this->updatePosition();
    this->hideTimeout = (timeOutMs > 0) ? timeOutMs : (1000 * 60 * 60);
    this->startTimer(TIMER_MILLISECONDS);

    this->tooltipComponent = newTargetComponent;
    this->addAndMakeVisible(this->tooltipComponent);
    this->resized();
    this->toFront(false);
}


void TooltipContainer::hide()
{
    this->stopTimer();

    if (this->isVisible())
    {
        this->animator.fadeOut(this, 250);
        this->tooltipComponent = new Component(); // empty, but not nullptr
        this->setVisible(false);
    }
}

//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TooltipContainer" template="../../Template"
                 componentName="" parentClasses="public Component, private Timer"
                 constructorParams="" variableInitialisers="hideTimeout(-1),&#10;tooltipComponent(nullptr)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="450" initialHeight="80">
  <METHODS>
    <METHOD name="parentHierarchyChanged()"/>
    <METHOD name="parentSizeChanged()"/>
  </METHODS>
  <BACKGROUND backgroundColour="0">
    <ROUNDRECT pos="0 0 0M 0M" cornerSize="15" fill="solid: 90000000" hasStroke="0"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="" id="e34a5396d7dac4f8" memberName="tooltipComponent" virtualName=""
                    explicitFocusOrder="0" pos="0 0 0M 0M" class="Component" params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
