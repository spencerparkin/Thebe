#pragma once

#include <wx/window.h>

class GraphicsToolCanvas : public wxWindow
{
public:
	GraphicsToolCanvas(wxWindow* parent);
	virtual ~GraphicsToolCanvas();

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
};