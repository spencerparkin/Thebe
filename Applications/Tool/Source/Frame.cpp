#include "Frame.h"
#include "Canvas.h"
#include "App.h"
#include "SceneBuilder.h"
#include "Thebe/EngineParts/Scene.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/aboutdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/dirdlg.h>
#include <filesystem>

GraphicsToolFrame::GraphicsToolFrame(const wxPoint& pos, const wxSize& size) : wxFrame(nullptr, wxID_ANY, "Thebe Graphics Tool", pos, size), timer(this, ID_Timer)
{
	wxMenu* fileMenu = new wxMenu();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_BuildScene, "Build Scene", "Build a scene for the Thebe graphics engine using a file exported from 3Ds Max, Maya or Blender."));
	fileMenu->Append(new wxMenuItem(fileMenu, ID_PreviewScene, "Preview Scene", "Load and render a Thebe graphics engine scene file."));
	fileMenu->AppendSeparator();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_Exit, "Exit", "Go ski."));

	wxMenu* helpMenu = new wxMenu();
	helpMenu->Append(new wxMenuItem(helpMenu, ID_About, "About", "Show the about-box."));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(fileMenu, "File");
	menuBar->Append(helpMenu, "Help");
	this->SetMenuBar(menuBar);

	this->CreateStatusBar();

	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnBuildScene, this, ID_BuildScene);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnPreviewScene, this, ID_PreviewScene);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnAbout, this, ID_About);
	this->Bind(wxEVT_CLOSE_WINDOW, &GraphicsToolFrame::OnCloseWindow, this);
	this->Bind(wxEVT_TIMER, &GraphicsToolFrame::OnTimer, this, ID_Timer);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	this->canvas = new GraphicsToolCanvas(this);
	sizer->Add(this->canvas, 1, wxGROW);
	this->SetSizer(sizer);

	this->timer.Start(0);
}

/*virtual*/ GraphicsToolFrame::~GraphicsToolFrame()
{
}

GraphicsToolCanvas* GraphicsToolFrame::GetCanvas()
{
	return this->canvas;
}

void GraphicsToolFrame::OnBuildScene(wxCommandEvent& event)
{
	wxFileDialog inputFileDialog(this, "Choose input file.", wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST);
	if (inputFileDialog.ShowModal() != wxID_OK)
		return;

	wxDirDialog outputFolderDialog(this, "Specify output assets folder location.", wxEmptyString, wxDD_DIR_MUST_EXIST);
	if (outputFolderDialog.ShowModal() != wxID_OK)
		return;

	std::filesystem::path inputSceneFile((const char*)inputFileDialog.GetPath().c_str());
	std::filesystem::path outputAssetsFolder((const char*)outputFolderDialog.GetPath().c_str());

	SceneBuilder sceneBuilder;
	sceneBuilder.SetAssetsFolder(outputAssetsFolder);
	if (!sceneBuilder.BuildScene(inputSceneFile))
		wxMessageBox("Scene build failed!", "Error!", wxICON_ERROR | wxOK, this);
	else
		wxMessageBox("Scene build succeeded!", "Success!", wxICON_INFORMATION | wxOK, this);
}

void GraphicsToolFrame::OnPreviewScene(wxCommandEvent& event)
{
	wxFileDialog fileDialog(this, "Choose scene file to open.", wxEmptyString, wxEmptyString, "(*.scene)|*.scene", wxFD_FILE_MUST_EXIST);
	if (fileDialog.ShowModal() != wxID_OK)
		return;

	std::filesystem::path sceneFilePath((const char*)fileDialog.GetPath().c_str());

	wxDirDialog inputFolderDialog(this, "Specify input assets folder location.", wxEmptyString, wxDD_DIR_MUST_EXIST);
	if (inputFolderDialog.ShowModal() != wxID_OK)
		return;

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->WaitForGPUIdle();
	graphicsEngine->PurgeCache();

	std::filesystem::path inputAssetsFolder((const char*)inputFolderDialog.GetPath().c_str());

	graphicsEngine->RemoveAllAssetFolders();
	graphicsEngine->AddAssetFolder(inputAssetsFolder);

	Thebe::Reference<Thebe::Scene> scene;
	if (!graphicsEngine->LoadEnginePartFromFile(sceneFilePath, scene))
	{
		wxMessageBox("Failed to load scene: " + fileDialog.GetFilename(), "Error!", wxICON_ERROR | wxOK);
		return;
	}

	graphicsEngine->SetInputToAllRenderPasses(scene.Get());
}

void GraphicsToolFrame::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo aboutDialogInfo;

	aboutDialogInfo.SetName("Thebe Graphics Tool");
	aboutDialogInfo.SetDescription("This tool is used to create content for the Thebe Graphics Engine.");

	wxAboutBox(aboutDialogInfo);
}

void GraphicsToolFrame::OnExit(wxCommandEvent& event)
{
	this->Close(true);
}

void GraphicsToolFrame::OnTimer(wxTimerEvent& event)
{
	this->canvas->Refresh();
	double deltaTimeSeconds = wxGetApp().GetGraphicsEngine()->GetDeltaTime();
	wxGetApp().GetFreeCam()->Update(deltaTimeSeconds);
}

void GraphicsToolFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
}

void GraphicsToolFrame::OnCloseWindow(wxCloseEvent& event)
{
	wxFrame::OnCloseWindow(event);
}