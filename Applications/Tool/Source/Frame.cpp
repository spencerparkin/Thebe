#include "Frame.h"
#include "Canvas.h"
#include "App.h"
#include "SceneBuilder.h"
#include "CubeMapBuilder.h"
#include "FontBuilder.h"
#include "RBDBuilder.h"
#include "Thebe/EngineParts/Scene.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/aboutdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/dirdlg.h>
#include <wx/busyinfo.h>
#include <wx/textdlg.h>
#include <wx/choicdlg.h>
#include <filesystem>

GraphicsToolFrame::GraphicsToolFrame(const wxPoint& pos, const wxSize& size) : wxFrame(nullptr, wxID_ANY, "Thebe Graphics Tool", pos, size), timer(this, ID_Timer)
{
	wxMenu* fileMenu = new wxMenu();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_BuildScene, "Build Scene", "Build a scene for the Thebe graphics engine using a file exported from 3Ds Max, Maya or Blender."));
	fileMenu->Append(new wxMenuItem(fileMenu, ID_BuildCubeMap, "Build Cube Map", "Build a cube map that can be used for a sky-dome or environment lighting."));
	fileMenu->Append(new wxMenuItem(fileMenu, ID_BuildFont, "Build Font", "Build a font asset that can be used to render text in the engine."));
	fileMenu->Append(new wxMenuItem(fileMenu, ID_BuildRigidBody, "Build Rigid Body", "Build an RBD object as the convex hull of a specified mesh."));
	fileMenu->AppendSeparator();
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
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnBuildCubeMap, this, ID_BuildCubeMap);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnBuildFont, this, ID_BuildFont);
	this->Bind(wxEVT_MENU, &GraphicsToolFrame::OnBuildRigidBody, this, ID_BuildRigidBody);
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

bool GraphicsToolFrame::FlagsDialog(const std::unordered_map<std::string, uint32_t>& flagMap, uint32_t& flags, const wxString& prompt)
{
	wxArrayInt selectionsArray;

	wxArrayString choiceArray;
	int i = 0;
	for (const auto& pair : flagMap)
	{
		wxString choiceLabel(pair.first.c_str());
		choiceArray.Add(choiceLabel);
		if ((flags & pair.second) != 0)
			selectionsArray.Add(i);
		i++;
	}

	wxMultiChoiceDialog choiceDialog(this, "Choose build options.", "Options", choiceArray);
	choiceDialog.SetSelections(selectionsArray);
	if (choiceDialog.ShowModal() != wxID_OK)
		return false;

	flags = 0;
	selectionsArray = choiceDialog.GetSelections();
	for (int i = 0; i < (int)selectionsArray.size(); i++)
	{
		std::string choiceKey((const char*)choiceArray[selectionsArray[i]].c_str());
		auto pair = flagMap.find(choiceKey);
		THEBE_ASSERT(pair != flagMap.end());
		flags |= pair->second;
	}

	return true;
}

void GraphicsToolFrame::OnBuildScene(wxCommandEvent& event)
{
	wxFileDialog inputFileDialog(this, "Choose input file.", wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST);
	if (inputFileDialog.ShowModal() != wxID_OK)
		return;

	std::unordered_map<std::string, uint32_t> flagsMap;
	flagsMap.insert(std::pair("Collapse", SCENE_BUILDER_FLAG_COLLAPSE_TREE));
	flagsMap.insert(std::pair("Apply Mesh Transform", SCENE_BUILDER_FLAG_APPLY_MESH_TRANSFORM));
	uint32_t flags = 0;		// TODO: Maybe remember this value from the registry or something?
	if (!this->FlagsDialog(flagsMap, flags, "Choose scene builder options."))
		return;

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	std::filesystem::path inputSceneFile((const char*)inputFileDialog.GetPath().c_str());
	std::filesystem::path outputAssetsFolder;
	if (!graphicsEngine->GleanAssetsFolderFromPath(inputSceneFile, outputAssetsFolder))
	{
		wxMessageBox(wxString::Format("Could not glean assets folder from file path: %s", inputSceneFile.string().c_str()), "Error!", wxICON_ERROR | wxOK, this);
		return;
	}

	SceneBuilder sceneBuilder;
	sceneBuilder.SetAssetsFolder(outputAssetsFolder);
	sceneBuilder.SetFlags(flags);
	bool sceneBuilt = false;
	{
		wxBusyCursor busyCursor;
		sceneBuilt = sceneBuilder.BuildScene(inputSceneFile);
	}

	if (!sceneBuilt)
		wxMessageBox("Scene build failed!", "Error!", wxICON_ERROR | wxOK, this);
	else
		wxMessageBox("Scene build succeeded!", "Success!", wxICON_INFORMATION | wxOK, this);
}

void GraphicsToolFrame::OnBuildCubeMap(wxCommandEvent& event)
{
	wxFileDialog inputFileDialog(this, "Choose input files.", wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (inputFileDialog.ShowModal() != wxID_OK)
		return;

	wxArrayString inputFilePathArray;
	inputFileDialog.GetPaths(inputFilePathArray);
	if (inputFilePathArray.size() == 0)
		return;

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	std::filesystem::path outputAssetsFolder;
	if (!graphicsEngine->GleanAssetsFolderFromPath(std::filesystem::path((const char*)inputFilePathArray[0].c_str()), outputAssetsFolder))
		return;

	graphicsEngine->RemoveAllAssetFolders();
	graphicsEngine->AddAssetFolder(outputAssetsFolder);

	CubeMapBuilder cubeMapBuilder;
	bool cubeMapBuilt = false;
	{
		wxBusyCursor busyCursor;
		cubeMapBuilt = cubeMapBuilder.BuildCubeMap(inputFilePathArray);
	}

	if (!cubeMapBuilt)
		wxMessageBox("Cube map build failed!", "Error!", wxICON_ERROR | wxOK, this);
	else
		wxMessageBox("Cube map build succeeded!", "Success!", wxICON_INFORMATION | wxOK, this);
}

void GraphicsToolFrame::OnBuildFont(wxCommandEvent& event)
{
	wxFileDialog inputFileDialog(this, "Choose font files.", wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (inputFileDialog.ShowModal() != wxID_OK)
		return;

	bool succeeded = true;
	wxArrayString inputFilePathArray;
	inputFileDialog.GetPaths(inputFilePathArray);
	for (const wxString& inputFontFile : inputFilePathArray)
	{
		Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
		std::filesystem::path outputAssetsFolder;
		if (!graphicsEngine->GleanAssetsFolderFromPath(std::filesystem::path((const char*)inputFontFile.c_str()), outputAssetsFolder))
			return;

		graphicsEngine->RemoveAllAssetFolders();
		graphicsEngine->AddAssetFolder(outputAssetsFolder);

		FontBuilder fontBuilder;
		if (!fontBuilder.GenerateFont(inputFontFile, outputAssetsFolder))
		{
			wxMessageBox(wxString::Format("Failed to build font: %s", inputFontFile.c_str()), "Error!", wxICON_ERROR | wxOK, this);
			succeeded = false;
			break;
		}
	}

	if (succeeded)
		wxMessageBox(wxString::Format("Built %d font(s) successfully!", (int)inputFilePathArray.size()), "Success!", wxICON_INFORMATION | wxOK, this);
}

void GraphicsToolFrame::OnBuildRigidBody(wxCommandEvent& event)
{
	wxFileDialog inputFileDialog(this, "Choose input file.", wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST);
	if (inputFileDialog.ShowModal() != wxID_OK)
		return;

	wxTextEntryDialog meshNameDialog(this, "Specify mesh name.", "Mesh Name");
	if (meshNameDialog.ShowModal() != wxID_OK)
		return;

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	std::filesystem::path inputSceneFile((const char*)inputFileDialog.GetPath().c_str());
	std::filesystem::path outputAssetsFolder;
	if (!graphicsEngine->GleanAssetsFolderFromPath(inputSceneFile, outputAssetsFolder))
	{
		wxMessageBox(wxString::Format("Could not glean assets folder from file path: %s", inputSceneFile.string().c_str()), "Error!", wxICON_ERROR | wxOK, this);
		return;
	}

	RBDBuilder rbdBuilder;
	rbdBuilder.SetDesiredMeshName(aiString((const char*)meshNameDialog.GetValue().c_str()));

	if (wxMessageBox("Will the physics object remain stationary?", "Stationary?", wxYES_NO | wxICON_QUESTION, this) == wxID_YES)
		rbdBuilder.SetStationary(true);
	else
		rbdBuilder.SetStationary(false);

	bool bodyBuilt = false;
	{
		wxBusyCursor busyCursor;
		bodyBuilt = rbdBuilder.BuildRigidBody(inputSceneFile, outputAssetsFolder);
	}

	if (!bodyBuilt)
		wxMessageBox("Scene build RBD object!", "Error!", wxICON_ERROR | wxOK, this);
	else
		wxMessageBox("Scene build succeeded!", "Success!", wxICON_INFORMATION | wxOK, this);
}

void GraphicsToolFrame::OnPreviewScene(wxCommandEvent& event)
{
	wxFileDialog fileDialog(this, "Choose scene file to open.", wxEmptyString, wxEmptyString, "(*.scene)|*.scene", wxFD_FILE_MUST_EXIST);
	if (fileDialog.ShowModal() != wxID_OK)
		return;

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	std::filesystem::path sceneFilePath((const char*)fileDialog.GetPath().c_str());
	std::filesystem::path inputAssetsFolder;
	if (!graphicsEngine->GleanAssetsFolderFromPath(sceneFilePath, inputAssetsFolder))
	{
		wxMessageBox(wxString::Format("Could not glean assets folder from file path: %s", sceneFilePath.string().c_str()), "Error!", wxICON_ERROR | wxOK, this);
		return;
	}
	
	graphicsEngine->WaitForGPUIdle();
	graphicsEngine->PurgeCache();
	graphicsEngine->RemoveAllAssetFolders();
	graphicsEngine->AddAssetFolder(inputAssetsFolder);

	Thebe::Reference<Thebe::Scene> scene;
	if (!graphicsEngine->LoadEnginePartFromFile(sceneFilePath, scene))
	{
		wxMessageBox("Failed to load scene: " + fileDialog.GetFilename(), "Error!", wxICON_ERROR | wxOK);
		return;
	}

	graphicsEngine->SetRenderObject(scene.Get());
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
}

void GraphicsToolFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
}

void GraphicsToolFrame::OnCloseWindow(wxCloseEvent& event)
{
	wxFrame::OnCloseWindow(event);
}