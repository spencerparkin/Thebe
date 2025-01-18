#include "Main.h"
#include "App.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow)
{
	DebugRendererApplication app;

	int exitCode = 0;
	if (app.Setup(instance, cmdShow, 1280, 760))
		exitCode = app.Run();

	app.Shutdown(instance);

	return exitCode;
}