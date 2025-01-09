#include "Canvas.h"
#include "Application.h"

using namespace Thebe;

ChineseCheckersCanvas::ChineseCheckersCanvas(wxWindow* parent) : wxWindow(parent, wxID_ANY)
{
	this->Bind(wxEVT_PAINT, &ChineseCheckersCanvas::OnPaint, this);
	this->Bind(wxEVT_SIZE, &ChineseCheckersCanvas::OnSize, this);
}

/*virtual*/ ChineseCheckersCanvas::~ChineseCheckersCanvas()
{
}

void ChineseCheckersCanvas::OnPaint(wxPaintEvent& event)
{
	GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();

	DynamicLineRenderer* lineRenderer = wxGetApp().GetLineRenderer();
	if (lineRenderer)
	{
		lineRenderer->ResetLines();

		Vector3 origin = Vector3::Zero();
		Vector3 xAxis = Vector3::XAxis();
		Vector3 yAxis = Vector3::YAxis();
		Vector3 zAxis = Vector3::ZAxis();

		lineRenderer->AddLine(origin, xAxis, &xAxis, &xAxis);
		lineRenderer->AddLine(origin, yAxis, &yAxis, &yAxis);
		lineRenderer->AddLine(origin, zAxis, &zAxis, &zAxis);
	}

	graphicsEngine->Render();
}

void ChineseCheckersCanvas::OnSize(wxSizeEvent& event)
{
	wxSize windowSize = event.GetSize();
	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->Resize(windowSize.x, windowSize.y);
}