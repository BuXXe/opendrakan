
#include <odCore/state/StateManager.h>

#include <odCore/Level.h>
#include <odCore/LevelObject.h>

namespace odState
{

    static const TickNumber MAX_BACKLOG = 64;

    class ApplyEventsVisitor
    {
    public:

        void operator()(const Event &event)
        {
            (void)event;
            // catch-all, doesn't do anything
        }

        void operator()(const ObjectTransformEvent &event)
        {
        }

    };

    StateManager::StateManager(od::Level &level)
    : mLevel(level)
    , mEvents(0)
    {
    }

    /*void StateManager::addActionEvent(odInput::ActionCode actionCode, bool down)
    {
        mEvents.push(ActionEvent(actionCode, down));
    }*/

    void StateManager::objectTransformed(od::LevelObject &object, const ObjectTransform &tf)
    {
        auto &objEvent = mNextStateTransitionMap[object.getObjectId()];
        objEvent.transformed = true;
        objEvent.transform = tf;
    }

    void StateManager::objectVisibilityChanged(od::LevelObject &object, bool visible)
    {
        auto &objEvent = mNextStateTransitionMap[object.getObjectId()];
        objEvent.visibilityChanged = true;
        objEvent.visibility = visible;
    }

    void StateManager::apply(TickNumber tick)
    {
        auto begin = mEvents.getTickframeBegin(tick);
        auto end = mEvents.getTickframeEnd(tick);
        if(begin == end)
        {
            throw od::Exception("Tick is out of range/not stored");
        }

        ApplyEventsVisitor visitor;

        for(auto it = begin; it != end; ++it)
        {
            it->visit(visitor);
        }
    }

    /**
     * Applied on timeline in reverse.
     */
    class CombineEventsVisitor
    {
    public:

        CombineEventsVisitor()
        {
        }

        void operator()(const Event &event)
        {
            (void)event; // non-state events are universally ignored
        }

        void operator()(const StateEvent &event)
        {
            throw od::Exception("CombineEventsVisitor is missing a handler for at least one state event type");
        }

        void operator()(const ObjectTransformEvent &event)
        {
            auto &objectEventSet = mTransitionMap[event.object.getObjectId()];
            if(!objectEventSet.transformed) // only want to record the newest transform
            {
                objectEventSet.transformed = true;
                objectEventSet.transform = event.transform;
            }
        }

        void operator()(const ObjectVisibilityChangedEvent &event)
        {
            auto &objectEventSet = mTransitionMap[event.object.getObjectId()];
            if(!objectEventSet.visibilityChanged)
            {
                objectEventSet.visibilityChanged = true;
                objectEventSet.visibility = event.visible;
            }
        }

        void operator()(const ObjectAnimFrameEvent &event)
        {
            auto &objectEventSet = mTransitionMap[event.object.getObjectId()];
            if(!objectEventSet.animationFrame)
            {
                objectEventSet.animationFrame = true;
                objectEventSet.animFrameTime = event.frameTime;
            }
        }

    private:

        StateManager::StateTransitionMap mTransitionMap;

    };

    void StateManager::commit()
    {
        auto firstTick = mEvents.getFirstTick();
        combine(firstTick, firstTick+1, mBaseStateTransitionMap);

        // turn state transition map into a list of events and push it onto the timeline
        for(auto &objEvent : mNextStateTransitionMap)
        {
            od::LevelObject *obj = mLevel.getLevelObjectById(objEvent.first);
            if(obj == nullptr) continue;
            if(objEvent.second.transformed) mEvents.push(ObjectTransformEvent(*obj, objEvent.second.transform));
            //if(objEvent.second.visibilityChanged) mEvents.push(ObjectVisibilityChangedEvent(*obj, obj.second.visibility);
            //if(objEvent.second.animationFrame) mEvents.push(ObjectAnimFrameEvent(*obj, obj.second.transform));
        }
    }

    void StateManager::combine(TickNumber begin, TickNumber end, StateTransitionMap &map)
    {
        CombineEventsVisitor combineVisitor;

        auto tickCount = (end - begin);
        for(size_t i = 0; i < tickCount; ++i)
        {
            auto tick = end - i;
            for(auto event : mEvents.getTickframe(tick))
            {
                event.visit(combineVisitor);
            }
        }
    }

}
