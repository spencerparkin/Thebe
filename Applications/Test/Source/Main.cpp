#include "Main.h"
#include "TestApplication.h"
#include "Thebe/Log.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow)
{
#if defined THEBE_LOGGING
	Thebe::Reference<Thebe::Log> log(new Thebe::Log());
	log->AddSink(new Thebe::LogConsoleSink());
	Thebe::Log::Set(log);
#endif //THEBE_LOGGING

	TestApplication app;

	int exitCode = 0;
	if (app.Setup(instance, cmdShow, 1280, 760))
		exitCode = app.Run();

	app.Shutdown(instance);

	return exitCode;
}