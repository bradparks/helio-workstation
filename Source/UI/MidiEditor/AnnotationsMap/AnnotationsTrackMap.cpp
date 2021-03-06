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

#include "Common.h"
#include "AnnotationsTrackMap.h"
#include "ProjectTreeItem.h"
#include "Transport.h"
#include "MidiLayer.h"
#include "ProjectAnnotations.h"
#include "PianoLayer.h"
#include "PlayerThread.h"
#include "MidiRoll.h"
#include "HelioCallout.h"
#include "AnnotationCommandPanel.h"
#include "TrackStartIndicator.h"
#include "TrackEndIndicator.h"

template<typename T> AnnotationsTrackMap<T>::AnnotationsTrackMap(ProjectTreeItem &parentProject, MidiRoll &parentRoll) :
    project(parentProject),
    roll(parentRoll),
    projectFirstBeat(0.f),
    projectLastBeat(16.f), // non zero!
    rollFirstBeat(0.f),
    rollLastBeat(16.f)
{
    this->setOpaque(false);
    this->setAlwaysOnTop(true);
    this->setInterceptsMouseClicks(false, true);
    
    this->trackStartIndicator = new TrackStartIndicator();
    this->addAndMakeVisible(this->trackStartIndicator);

    this->trackEndIndicator = new TrackEndIndicator();
    this->addAndMakeVisible(this->trackEndIndicator);
    
    this->updateTrackRangeIndicatorsAnchors();
    
    this->reloadTrackMap();
    
    this->project.addListener(this);
}

template<typename T> AnnotationsTrackMap<T>::~AnnotationsTrackMap()
{
    this->project.removeListener(this);
}

template<typename T> void AnnotationsTrackMap<T>::updateTrackRangeIndicatorsAnchors()
{
    const float rollLengthInBeats = (this->rollLastBeat - this->rollFirstBeat);
    const float absStart = ((this->projectFirstBeat - this->rollFirstBeat) / rollLengthInBeats);
    const float absEnd = ((this->projectLastBeat - this->rollFirstBeat) / rollLengthInBeats);
    //Logger::writeToLog("updateTrackRangeIndicatorsAnchors: " + String(absStart) + ":" + String(absEnd));
    this->trackStartIndicator->setAnchoredAt(absStart);
    this->trackEndIndicator->setAnchoredAt(absEnd);
}


//===----------------------------------------------------------------------===//
// Component
//===----------------------------------------------------------------------===//

template<typename T> void AnnotationsTrackMap<T>::resized()
{
    //Logger::writeToLog("AnnotationsTrackMap<T>::resized");

    this->rollFirstBeat = this->roll.getFirstBeat();
    this->rollLastBeat = this->roll.getLastBeat();

    this->setVisible(false);

    T *previous = nullptr;

    for (int i = 0; i < this->annotationComponents.size(); ++i)
    {
        T *current = this->annotationComponents.getUnchecked(i);
        current->updateContent();

        if (previous != nullptr)
        {
            this->applyAnnotationBounds(previous, current);
        }

        if (i == (this->annotationComponents.size() - 1))
        {
            this->applyAnnotationBounds(current, nullptr);
        }

        previous = current;
    }

    this->updateTrackRangeIndicatorsAnchors();
    
    this->setVisible(true);
}


//===----------------------------------------------------------------------===//
// ProjectListener
//===----------------------------------------------------------------------===//

template<typename T> void AnnotationsTrackMap<T>::onEventChanged(const MidiEvent &oldEvent, const MidiEvent &newEvent)
{
    if (newEvent.getLayer() == this->project.getAnnotationsTrack()->getLayer())
    {
        const AnnotationEvent &annotation = static_cast<const AnnotationEvent &>(oldEvent);
        const AnnotationEvent &newAnnotation = static_cast<const AnnotationEvent &>(newEvent);

        if (T *component = this->annotationsHash[annotation])
        {
            this->alignAnnotationComponent(component);

            this->annotationsHash.remove(annotation);
            this->annotationsHash.set(newAnnotation, component);
        }
    }
}

template<typename T> void AnnotationsTrackMap<T>::alignAnnotationComponent(T *component)
{
    this->annotationComponents.sort(*component);
    const int indexOfSorted = this->annotationComponents.indexOfSorted(*component, component);
    T *previousEventComponent(this->getPreviousEventComponent(indexOfSorted));
    T *nextEventComponent(this->getNextEventComponent(indexOfSorted));
    
    if (previousEventComponent)
    {
        this->applyAnnotationBounds(previousEventComponent, component);
        
        T *oneMorePrevious = this->getPreviousEventComponent(indexOfSorted - 1);
        
        if (oneMorePrevious)
        { this->applyAnnotationBounds(oneMorePrevious, previousEventComponent); }
    }
    
    if (nextEventComponent)
    {
        T *oneMoreNext = this->getNextEventComponent(indexOfSorted + 1);
        this->applyAnnotationBounds(nextEventComponent, oneMoreNext);
    }
    
    component->updateContent();
    this->applyAnnotationBounds(component, nextEventComponent);
}

template<typename T> void AnnotationsTrackMap<T>::onEventAdded(const MidiEvent &event)
{
    if (event.getLayer() == this->project.getAnnotationsTrack()->getLayer())
    {
        const AnnotationEvent &annotation = static_cast<const AnnotationEvent &>(event);

        auto component = new T(*this, annotation);
        this->addChildComponent(component);

        const int indexOfSorted = this->annotationComponents.addSorted(*component, component);
        T *previousEventComponent(this->getPreviousEventComponent(indexOfSorted));
        T *nextEventComponent(this->getNextEventComponent(indexOfSorted));

        component->updateContent();
        this->applyAnnotationBounds(component, nextEventComponent);
        component->toFront(false);

        if (previousEventComponent)
        { this->applyAnnotationBounds(previousEventComponent, component); }

        this->annotationsHash.set(annotation, component);

        component->setAlpha(0.f);
        const Rectangle<int> bounds(component->getBounds());
        component->setBounds(bounds.translated(0, -component->getHeight()));
        this->animator.animateComponent(component, bounds, 1.f, 250, false, 0.0, 0.0);
    }
}

template<typename T> void AnnotationsTrackMap<T>::onEventRemoved(const MidiEvent &event)
{
    if (event.getLayer() == this->project.getAnnotationsTrack()->getLayer())
    {
        const AnnotationEvent &annotation = static_cast<const AnnotationEvent &>(event);

        if (T *component = this->annotationsHash[annotation])
        {
            this->animator.animateComponent(component,
                                            component->getBounds().translated(0, -component->getHeight()),
                                            0.f, 250, true, 0.0, 0.0);

            this->removeChildComponent(component);
            this->annotationsHash.remove(annotation);

            const int indexOfSorted = this->annotationComponents.indexOfSorted(*component, component);
            T *previousEventComponent(this->getPreviousEventComponent(indexOfSorted));
            T *nextEventComponent(this->getNextEventComponent(indexOfSorted));

            if (previousEventComponent)
            { this->applyAnnotationBounds(previousEventComponent, nextEventComponent); }

            this->annotationComponents.removeObject(component, true);
        }
    }
}

template<typename T> void AnnotationsTrackMap<T>::onLayerChanged(const MidiLayer *layer)
{
    if (this->project.getAnnotationsTrack() != nullptr)
    {
        if (layer == this->project.getAnnotationsTrack()->getLayer())
        {
            this->reloadTrackMap();
        }
    }
}

template<typename T> void AnnotationsTrackMap<T>::onLayerAdded(const MidiLayer *layer)
{
    if (this->project.getAnnotationsTrack() != nullptr)
    {
        if (layer == this->project.getAnnotationsTrack()->getLayer())
        {
            if (layer->size() > 0)
            {
                this->reloadTrackMap();
            }
        }
    }
}

template<typename T> void AnnotationsTrackMap<T>::onLayerRemoved(const MidiLayer *layer)
{
    if (this->project.getAnnotationsTrack() != nullptr)
    {
        if (layer == this->project.getAnnotationsTrack()->getLayer())
        {
            for (int i = 0; i < layer->size(); ++i)
            {
                const AnnotationEvent &annotation = static_cast<const AnnotationEvent &>(*layer->getUnchecked(i));

                if (T *component = this->annotationsHash[annotation])
                {
                    this->removeChildComponent(component);
                    this->annotationsHash.removeValue(component);
                    this->annotationComponents.removeObject(component, true);
                }
            }
        }
    }
}

template<typename T> void AnnotationsTrackMap<T>::onProjectBeatRangeChanged(float firstBeat, float lastBeat)
{
    this->projectFirstBeat = firstBeat;
    this->projectLastBeat = lastBeat;
    this->updateTrackRangeIndicatorsAnchors();
}


//===----------------------------------------------------------------------===//
// Private
//===----------------------------------------------------------------------===//

template<typename T> void AnnotationsTrackMap<T>::onAnnotationMoved(T *nc)
{
    this->roll.grabKeyboardFocus();
}

template<typename T> void AnnotationsTrackMap<T>::onAnnotationTapped(T *nc)
{
    const AnnotationEvent *annotationUnderSeekCursor = nullptr;
    const ProjectAnnotations *annotations = this->project.getAnnotationsTrack();
    const double seekPosition = this->project.getTransport().getSeekPosition();

    for (int i = 0; i < annotations->getLayer()->size(); ++i)
    {
        if (AnnotationEvent *annotation = dynamic_cast<AnnotationEvent *>(annotations->getLayer()->getUnchecked(i)))
        {
            const float seekBeat = this->roll.getBeatByTransportPosition(seekPosition);
            
            if (fabs(annotation->getBeat() - seekBeat) < 0.1)
            {
                annotationUnderSeekCursor = annotation;
                break;
            }
        }
    }

    const double newSeekPosition = this->roll.getTransportPositionByBeat(nc->getBeat() - 0.01f);
    const bool wasPlaying = this->project.getTransport().isPlaying();

    if (wasPlaying)
    {
        this->project.getTransport().stopPlayback();
    }

    this->project.getTransport().seekToPosition(newSeekPosition);

    //if (wasPlaying)
    //{
    //    this->project.getTransport().startPlayback();
    //}

    // если аннотация уже выбрана - покажем ее меню
    if (annotationUnderSeekCursor == &nc->getEvent() && !wasPlaying)
    {
        this->showContextMenuFor(nc);
    }
    else
    {
        this->roll.grabKeyboardFocus();
    }
}

template<typename T> void AnnotationsTrackMap<T>::showContextMenuFor(T *nc)
{
    if (! this->project.getTransport().isPlaying())
    {
        HelioCallout::emit(new AnnotationCommandPanel(this->project, nc->getEvent()), nc);
    }
}

template<typename T> void AnnotationsTrackMap<T>::alternateActionFor(T *nc)
{
    // Selects everything within the range of this annotation
    this->annotationComponents.sort(*nc);
    const int indexOfSorted = this->annotationComponents.indexOfSorted(*nc, nc);
    T *nextEventComponent(this->getNextEventComponent(indexOfSorted));
    
    const float startBeat = nc->getBeat();
    const float endBeat = (nextEventComponent != nullptr) ? nextEventComponent->getBeat() : FLT_MAX;
    const bool isShiftPressed = Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown();
    const bool shouldClearSelection = !isShiftPressed;
    
    this->roll.selectEventsInRange(startBeat, endBeat, shouldClearSelection);
}

template<typename T> float AnnotationsTrackMap<T>::getBeatByXPosition(int x) const
{
    const int xRoll = int(float(x) / float(this->getWidth()) * float(this->roll.getWidth()));
    const float targetBeat = this->roll.getRoundBeatByXPosition(xRoll);
    return jmin(jmax(targetBeat, this->rollFirstBeat), this->rollLastBeat);
}

template<typename T> void AnnotationsTrackMap<T>::reloadTrackMap()
{
    //Logger::writeToLog("AnnotationsTrackMap<T>::reloadTrackMap");

    for (int i = 0; i < this->annotationComponents.size(); ++i)
    {
        this->removeChildComponent(this->annotationComponents.getUnchecked(i));
    }

    this->annotationComponents.clear();
    this->annotationsHash.clear();

    this->setVisible(false);

    MidiLayer *layer = this->project.getAnnotationsTrack()->getLayer();

    for (int j = 0; j < layer->size(); ++j)
    {
        MidiEvent *event = layer->getUnchecked(j);

        if (AnnotationEvent *annotation = dynamic_cast<AnnotationEvent *>(event))
        {
            auto component = new T(*this, *annotation);
            this->addAndMakeVisible(component);

            this->annotationComponents.addSorted(*component, component);
            this->annotationsHash.set(*annotation, component);
        }
    }

    this->resized();
    this->setVisible(true);
}

template<typename T> void AnnotationsTrackMap<T>::applyAnnotationBounds(T *nc, T *nextOne)
{
    const float rollLengthInBeats = (this->rollLastBeat - this->rollFirstBeat);
    const float projectLengthInBeats = (this->projectLastBeat - this->projectFirstBeat);

    const float beat = (nc->getBeat() - this->rollFirstBeat);
    const float mapWidth = float(this->getWidth()) * (projectLengthInBeats / rollLengthInBeats);

    const float x = (mapWidth * (beat / projectLengthInBeats));

    const float nextBeat = ((nextOne ? nextOne->getBeat() : this->rollLastBeat) - this->rollFirstBeat);
    const float nextX = mapWidth * (nextBeat / projectLengthInBeats);

    const float minWidth = 10.f;
    const float oneBeatWidth = jmax(minWidth, (mapWidth * (1.f / projectLengthInBeats)));

    const float widthMargin = 25.f;
    const float componentsPadding = 10.f;
    const float maxWidth = nextX - x;
    const float w = jmax(minWidth, jmin((maxWidth - componentsPadding), (nc->getTextWidth() + widthMargin)));

    nc->setRealBounds(Rectangle<float>(x, 0.f, w, float(nc->getHeight())));
}

template<typename T> T *AnnotationsTrackMap<T>::getPreviousEventComponent(int indexOfSorted) const
{
    const int indexOfPrevious = indexOfSorted - 1;

    return
        isPositiveAndBelow(indexOfPrevious, this->annotationComponents.size()) ?
        this->annotationComponents.getUnchecked(indexOfPrevious) :
        nullptr;
}

template<typename T> T *AnnotationsTrackMap<T>::getNextEventComponent(int indexOfSorted) const
{
    const int indexOfNext = indexOfSorted + 1;

    return
        isPositiveAndBelow(indexOfNext, this->annotationComponents.size()) ?
        this->annotationComponents.getUnchecked(indexOfNext) :
        nullptr;
}
