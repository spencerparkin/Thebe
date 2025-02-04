#pragma once

#include <wx/window.h>
#include "Thebe/Math/LineSegment.h"
#include "Thebe/EngineParts/CollisionObject.h"

class ChineseCheckersCanvas : public wxWindow
{
public:
	ChineseCheckersCanvas(wxWindow* parent);
	virtual ~ChineseCheckersCanvas();

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseLeftClick(wxMouseEvent& event);
	void OnMouseRightClick(wxMouseEvent& event);

	void SetDebugDraw(bool debugDraw);
	bool GetDebugDraw() const;

private:
	Thebe::CollisionObject* PickCollisionObject(const wxPoint& mousePoint);

	bool debugDraw;
};