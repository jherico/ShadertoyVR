/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#include "qt/QtCommon.h"
#include "MainWindow.h"
#include "ShadertoyApp.h"
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include "display/DisplayPlugins.h"

extern Plugins::Display::Plugin** DISPLAY_PLUGINS;
extern size_t DISPLAY_PLUGIN_COUNT;

MainWindow::MainWindow(QQuickItem* parent) {
    Plugins::Display::list(DISPLAY_PLUGINS);
    for (size_t i = 0; i < DISPLAY_PLUGIN_COUNT; ++i) {
        Plugins::Display::Plugin* plugin = DISPLAY_PLUGINS[i];
        _displayPlugins << plugin->name();
    }
}

MainWindow::~MainWindow() {
}

const QStringList& MainWindow::displayPlugins() {
    return _displayPlugins;
}

void MainWindow::activatePlugin(int index) {
    qApp->activatePlugin(index);
}
