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
#pragma once

#include <QStringList>


class MainWindow : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QStringList displayPlugins READ displayPlugins CONSTANT)
public:
    MainWindow(QQuickItem* parent = nullptr);
    virtual ~MainWindow();

protected:
    Q_INVOKABLE void activatePlugin(int index);
    const QStringList& displayPlugins();

private:
    QStringList _displayPlugins;
};
