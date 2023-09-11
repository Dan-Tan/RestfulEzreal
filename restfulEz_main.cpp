#include "imgui/imgui.h"


Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Restful Ezreal";

	Walnut::Application* app = new Walnut::Application(spec);

        std::shared_ptr<NavBar> navigBar = std::make_shared<NavBar>();
        std::shared_ptr<restfulEz::RestfulEzreal> _main = std::make_shared<restfulEz::RestfulEzreal>(&navigBar->page_r);

        app->PushLayer(navigBar);
        app->PushLayer(_main);
#if DEBUG_MODE
	app->PushLayer<ExampleLayer>();
#endif
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}
