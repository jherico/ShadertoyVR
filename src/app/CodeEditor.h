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

#include <QQuickItem>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>

class CodeEditor : public QQuickItem {
    Q_OBJECT


public:
    CodeEditor(QQuickItem* parent = nullptr);
    virtual ~CodeEditor();

    virtual void setHighlighter(QSyntaxHighlighter* highlighter);
    virtual void setText(const QString& text);
    virtual QString text();
    virtual void setErrorText(const QString& text);

private:
    QQuickItem* getTextArea();
    QQuickItem* getErrorTextArea();

    QSyntaxHighlighter* _highlighter{ nullptr };

};
