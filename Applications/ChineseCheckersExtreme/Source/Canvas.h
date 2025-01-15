#pragma once

#include <wx/window.h>
#include "Thebe/Math/LineSegment.h"

class ChineseCheckersCanvas : public wxWindow
{
public:
	ChineseCheckersCanvas(wxWindow* parent);
	virtual ~ChineseCheckersCanvas();

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseLeftClick(wxMouseEvent& event);

private:
	enum PickingMode
	{
		CHOOSING_SOURCE,
		CHOOSING_TARGET
	};

	PickingMode pickingMode;
};