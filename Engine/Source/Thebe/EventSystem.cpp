#include "Thebe/EventSystem.h"

using namespace Thebe;

//---------------------------------- EventSystem ----------------------------------

EventSystem::EventSystem()
{
	this->nextEventHandlerID = 1;
}

/*virtual*/ EventSystem::~EventSystem()
{
	for (auto& pair : this->categoryHandlerMap)
		delete pair.second;
}

EventHandlerID EventSystem::RegisterEventHandler(const std::string& eventCategory, EventHandler eventHandler)
{
	EventHandlerID eventHandlerID = this->nextEventHandlerID++;

	this->eventHandlerMap.insert(std::pair(eventHandlerID, eventHandler));

	std::list<EventHandlerID>* eventHandlerIDList = nullptr;
	CategoryHandlerMap::iterator iter = this->categoryHandlerMap.find(eventCategory);
	if (iter != this->categoryHandlerMap.end())
		eventHandlerIDList = iter->second;
	else
	{
		eventHandlerIDList = new std::list<EventHandlerID>();
		this->categoryHandlerMap.insert(std::pair(eventCategory, eventHandlerIDList));
	}

	eventHandlerIDList->push_back(eventHandlerID);

	return eventHandlerID;
}

bool EventSystem::UnregisterEventHandler(EventHandlerID eventHandlerID)
{
	EventHandlerMap::iterator iter = this->eventHandlerMap.find(eventHandlerID);
	if (iter == this->eventHandlerMap.end())
		return false;

	this->eventHandlerMap.erase(iter);
	return true;
}

void EventSystem::EnqueueEvent(Event* event)
{
	std::lock_guard<std::mutex> lock(this->eventQueueMutex);
	this->eventQueue.push_back(event);
}

Event* EventSystem::DequeueEvent()
{
	Event* event = nullptr;

	if (this->eventQueue.size() > 0)
	{
		std::lock_guard<std::mutex> lock(this->eventQueueMutex);
		if (this->eventQueue.size() > 0)
		{
			event = *this->eventQueue.begin();
			this->eventQueue.pop_front();
		}
	}

	return event;
}

void EventSystem::SendEvent(Event* event)
{
	this->EnqueueEvent(event);
}

void EventSystem::DispatchAllEvents()
{
	while (true)
	{
		Event* event = this->DequeueEvent();
		if (!event)
			break;

		this->DispatchEvent(event);
		delete event;
	}
}

void EventSystem::DispatchEvent(Event* event)
{
	const std::string& eventCategory = event->GetCategory();
	CategoryHandlerMap::iterator categoryPair = this->categoryHandlerMap.find(eventCategory);
	if (categoryPair == this->categoryHandlerMap.end())
		return;

	std::vector<EventHandlerID> doomedHandlerIDArray;
	std::list<EventHandlerID>* eventHandlerIDList = categoryPair->second;
	for(auto eventHandlerID : *eventHandlerIDList)
	{
		EventHandlerMap::iterator handlerPair = this->eventHandlerMap.find(eventHandlerID);
		if (handlerPair == this->eventHandlerMap.end())
			doomedHandlerIDArray.push_back(eventHandlerID);
		else
		{
			EventHandler eventHandler = handlerPair->second;
			eventHandler(event);
		}
	}

	for (auto eventHandlerID : doomedHandlerIDArray)
		this->eventHandlerMap.erase(eventHandlerID);
}

//---------------------------------- Event ----------------------------------
	
Event::Event()
{
}

/*virtual*/ Event::~Event()
{
}

const std::string& Event::GetCategory() const
{
	return this->category;
}

void Event::SetCategory(const std::string& category)
{
	this->category = category;
}