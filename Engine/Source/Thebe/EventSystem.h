#pragma once

#include "Common.h"
#include <string>
#include <functional>

namespace Thebe
{
	class Event;

	typedef std::function<void(const Event*)> EventHandler;
	typedef uint32_t EventHandlerID;

	/**
	 * The goal here is to decouple event senders from event responders,
	 * so that no sub-system or part of the code has to embed knowledge of
	 * some other part of the code, thus maintaining separation of concerns.
	 */
	class THEBE_API EventSystem
	{
	public:
		EventSystem();
		virtual ~EventSystem();

		/**
		 * Add a lambda function to the system to be called to process events of the given category.
		 * Note that if multiple handlers are registered for the same category, then the order
		 * in which those handlers are called for an event in that category is left undefined.
		 * 
		 * @param[in] eventCategory Events under this category will be handled by the given event handler.
		 * @param[in] eventHandler This function will get called to process events under the given category.
		 * @return An ID is returned for the registered event handler which can be used in the @ref UnregisterEventHandler function.
		 */
		EventHandlerID RegisterEventHandler(const std::string& eventCategory, EventHandler eventHandler);

		/**
		 * Remove an event handler from the system.  Note that an event handler should be removed
		 * before it is used for further event processing if it has capture values that have gone
		 * out of scope (i.e., become stale.)
		 */
		bool UnregisterEventHandler(EventHandlerID eventHandlerID);

		/**
		 * Enqueue the given event to be dispatched/handled later.
		 * 
		 * @param[in] event This should be a point to an event allocated on the heap.  The event system takes ownership of the memory.
		 */
		void SendEvent(Event* event);

		/**
		 * Process the event queue until it's empty.
		 */
		void DispatchAllEvents();

	private:

		void DispatchEvent(Event* event);

		typedef std::unordered_map<std::string, std::list<EventHandlerID>*> CategoryHandlerMap;
		typedef std::unordered_map<EventHandlerID, EventHandler> EventHandlerMap;

		std::list<Event*> eventQueue;
		CategoryHandlerMap categoryHandlerMap;
		EventHandlerMap eventHandlerMap;
		EventHandlerID nextEventHandlerID;
	};

	/**
	 * This is the base class for any type of event that can flow through the system.
	 * It can be useful by itself or a derivative can carry extra info.
	 */
	class Event
	{
	public:
		Event();
		virtual ~Event();

		const std::string& GetCategory() const;
		void SetCategory(const std::string& category);

	protected:
		std::string category;
	};
}