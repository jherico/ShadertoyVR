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
#include <QThread>

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

inline QByteArray readFileToByteArray(const QString & fileName) {
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    return f.readAll();
}

inline std::vector<uint8_t> readFileToVector(const QString & fileName) {
    QByteArray ba = readFileToByteArray(fileName);
    return std::vector<uint8_t>(ba.constData(), ba.constData() + ba.size());
}

inline QString readFileToString(const QString & fileName) {
    return QString(readFileToByteArray(fileName));
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


#ifdef OS_WIN
#define QT_APP_WITH_ARGS(AppClass) \
  int argc = 1; \
  char ** argv = &lpCmdLine;  \
  AppClass app(argc, argv);
#else
#define QT_APP_WITH_ARGS(AppClass) AppClass app(argc, argv);
#endif

#define RUN_QT_APP(AppClass) \
MAIN_DECL { \
  try { \
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", "."); \
    QT_APP_WITH_ARGS(AppClass); \
    return app.exec(); \
  } catch (std::exception & error) { \
    SAY_ERR(error.what()); \
  } catch (const std::string & error) { \
    SAY_ERR(error.c_str()); \
  } \
  return -1; \
}
