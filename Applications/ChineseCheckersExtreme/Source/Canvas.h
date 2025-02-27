#pragma once

#include <wx/window.h>
#include "Thebe/Math/LineSegment.h"
#include "Thebe/EngineParts/CollisionObject.h"
#include "ChineseCheckers/MoveSequence.h"

class ChineseCheckersCanvas : public wxWindow
{
public:
	ChineseCheckersCanvas(wxWindow* parent);
	virtual ~ChineseCheckersCanvas();

#if defined THEBE_USE_IMGUI
	virtual bool MSWHandleMessage(WXLRESULT* result, WXUINT message, WXWPARAM wParam, WXLPARAM lParam) override;
#endif //THEBE_USE_IMGUI

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseLeftUp(wxMouseEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseMiddleDown(wxMouseEvent& event);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
	void OnMouseWheelMoved(wxMouseEvent& event);

	void SetDebugDraw(bool debugDraw);
	bool GetDebugDraw() const;

private:
	ChineseCheckers::Node* PickNode(const wxPoint& mousePoint);

	void UpdateRings();

	wxPoint mousePointA, mousePointB;
	bool mouseDragging;
	bool debugDraw;
	ChineseCheckers::MoveSequence moveSequence;
};