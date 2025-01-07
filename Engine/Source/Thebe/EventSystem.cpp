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

void EventSystem::SendEvent(Event* event)
{
	this->eventQueue.push_back(event);
}

void EventSystem::DispatchAllEventHandlers()
{
	while (this->eventQueue.size() > 0)
	{
		Event* event = *this->eventQueue.begin();
		this->eventQueue.pop_front();
		this->DispatchEvent(event);
		delete event;
	}
}

void EventSystem::DispatchEvent(Event* event)
{
	const std::string& eventCategory = event->GetCategory();
	CategoryHandlerMap::iterator pair = this->categoryHandlerMap.find(eventCategory);
	if (pair == this->categoryHandlerMap.end())
		return;

	std::list<EventHandlerID>* eventHandlerIDList = pair->second;
	std::list<EventHandlerID>::iterator listIter = eventHandlerIDList->begin();
	while (listIter != eventHandlerIDList->end())
	{
		std::list<EventHandlerID>::iterator nextListIter(listIter);
		nextListIter++;

		EventHandlerID eventHandlerID = *listIter;
		EventHandlerMap::iterator mapIter = this->eventHandlerMap.find(eventHandlerID);
		if (mapIter == this->eventHandlerMap.end())
			eventHandlerIDList->erase(listIter);
		else
		{
			EventHandler eventHandler = mapIter->second;
			eventHandler(event);
		}

		listIter = nextListIter;
	}
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