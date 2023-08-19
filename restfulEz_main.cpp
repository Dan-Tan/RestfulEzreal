#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "src/RestfulEzreal/RestfulEzreal.h"

class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::ShowDemoWindow();
	}
};

class NavBar : public Walnut::Layer {
public:
	NavBar() {};
	current_page page_r = current_page::HOME;

	virtual void OnUIRender() override {

		ImGui::Begin("Navigation");

		if (ImGui::Selectable("Home", this->page_r == current_page::HOME)) {
			this->page_r = current_page::HOME; // todo ad page switch
		}
		if (ImGui::Selectable("Guide", this->page_r == current_page::GUIDE)) {
			this->page_r = current_page::GUIDE; // todo ad page switch
		}
		if (ImGui::Selectable("Advanced Requests", this->page_r == current_page::ADVANCED_REQUESTS)) {
			this->page_r = current_page::ADVANCED_REQUESTS; // todo ad page switch
		}
		if (ImGui::Selectable("Client Configuration", this->page_r == current_page::CLIENT_CONFIG)) {
			this->page_r = current_page::CLIENT_CONFIG; // todo ad page switch
		}
		if (ImGui::Selectable("Request History", this->page_r == current_page::REQUEST_HISTORY)) {
			this->page_r = current_page::REQUEST_HISTORY; // todo ad page switch
		}
		if (ImGui::Selectable("Request Status", this->page_r == current_page::REQUEST_STATUS)) {
			this->page_r = current_page::REQUEST_STATUS; // todo ad page switch
		}

		ImGui::End();
	};
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Restful Ezreal";

	Walnut::Application* app = new Walnut::Application(spec);

        std::shared_ptr<NavBar> navigBar = std::make_shared<NavBar>();
        std::shared_ptr<restfulEz::RestfulEzreal> _main = std::make_shared<restfulEz::RestfulEzreal>(&navigBar->page_r);

        app->PushLayer(navigBar);
        app->PushLayer(_main);
	app->PushLayer<ExampleLayer>();
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
