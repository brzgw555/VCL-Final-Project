#include "Labs/Final_Project/App.h"
#include "Assets/bundled.h"

namespace VCX::Labs::Rendering {
    using namespace Assets;

    App::App():
        _ui(Labs::Common::UIOptions {}),

        _caseRayTracing({ ExampleScene::CornellBox, ExampleScene::CornellBoxWithSphere }),
        _casePathTracing({ ExampleScene::CornellBox, ExampleScene::CornellBoxWithSphere }) {
    }

    void App::OnFrame() {
        _ui.Setup(_cases, _caseId);
    }
}
