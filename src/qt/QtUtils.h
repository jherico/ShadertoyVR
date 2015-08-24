/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

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
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickImageProvider>
#include <QSurfaceFormat>


namespace qt {
    inline ivec2 toGlm(const QPoint & pt) {
        return ivec2(pt.x(), pt.y());
    }

    inline uvec2 toGlm(const QSize & size) {
        return uvec2(size.width(), size.height());
    }

    inline vec2 toGlm(const QPointF & pt) {
        return vec2(pt.x(), pt.y());
    }

    QSize sizeFromGlm(const vec2 & size);
    QPointF pointFromGlm(const vec2 & pt);
}

QJsonValue path(const QJsonValue & parent, std::initializer_list<QVariant> elements);

class LambdaThread : public QThread {
  Q_OBJECT
  Lambda f;

  void run() {
    f();
  }

public:
  LambdaThread() {}

  template <typename F>
  LambdaThread(F f) : f(f) {}

  template <typename F>
  void setLambda(F f) { this->f = f; }
};

QSurfaceFormat getDesiredSurfaceFormat();

