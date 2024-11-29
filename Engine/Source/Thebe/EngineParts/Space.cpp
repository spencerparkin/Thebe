#include "Thebe/EngineParts/Space.h"
#include "Thebe/Log.h"

using namespace Thebe;

Space::Space()
{
}

/*virtual*/ Space::~Space()
{
}

std::vector<Reference<Space>>& Space::GetSubSpaceArray()
{
	return this->subSpaceArray;
}

/*virtual*/ void Space::AppendAllChildRenderObjects(std::list<RenderObject*>& renderObjectList)
{
	for (Reference<Space>& space : this->subSpaceArray)
		renderObjectList.push_back(space);
}

/*virtual*/ bool Space::Render(ID3D12GraphicsCommandList* commandList, Camera* camera)
{
	return true;
}

void Space::UpdateObjectToWorldTransform(const Transform& parentToWorld)
{
	this->objectToWorld = parentToWorld * this->childToParent;

	for (Reference<Space>& space : this->subSpaceArray)
		space->UpdateObjectToWorldTransform(this->objectToWorld);
}

void Space::SetChildToParentTransform(const Transform& childToParent)
{
	this->childToParent = childToParent;
}

void Space::AddSubSpace(Space* space)
{
	this->subSpaceArray.push_back(space);
}

void Space::ClearAllSubSpaces()
{
	this->subSpaceArray.clear();
}