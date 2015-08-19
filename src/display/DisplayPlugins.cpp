#include "DisplayPlugins.h"


#include <memory>
#include <vector>
#include <mutex>

#include "oculus/OculusWin32.h"
#include "Window.h"
#include "Null.h"

using namespace Plugins::Display;

using PluginPtr = std::shared_ptr<Plugins::Display::Plugin>;
using PluginsVec = std::vector<Plugins::Display::Plugin*>;

static PluginsVec LOADED_PLUGINS;
static PluginsVec SUPPORTED_PLUGINS;

void initPlugins() {
    static std::once_flag once;

    std::call_once(once, [] {
        LOADED_PLUGINS.push_back(buildWindowPlugin());
        LOADED_PLUGINS.push_back(buildOculusPlugin());
        LOADED_PLUGINS.push_back(buildNullPlugin());

        for (size_t i = 0; i < LOADED_PLUGINS.size(); ++i) {
            Plugin* loadedPlugin = LOADED_PLUGINS[i];
            if (loadedPlugin->supported() && loadedPlugin->init()) {
                SUPPORTED_PLUGINS.push_back(loadedPlugin);
            }
        }
    });
}


size_t Plugins::Display::list(Plugins::Display::Plugin** plugins) {
    initPlugins();
    size_t result = SUPPORTED_PLUGINS.size();
    if (result && plugins != nullptr) {
        memcpy(plugins, &SUPPORTED_PLUGINS[0], result * sizeof(Plugin*));
    }
    return result;
}
