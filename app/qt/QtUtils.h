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
#include "Common.h"

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


class QtUtil {
  QtUtil() {}

public:

  static vec2 toGlm(const QSize & size);
  static vec2 toGlm(const QPointF & pt);
  static QSize sizeFromGlm(const vec2 & size);
  static QPointF pointFromGlm(const vec2 & pt);

  static QByteArray toByteArray(const QString & path) {
    QFile f(path);
    f.open(QFile::ReadOnly);
    return f.readAll();
  }

  static QString toString(const QString & path) {
    return QString(toByteArray(path));
  }

  static QImage toImage(const QString & path);
};

class QMyQuickRenderControl : public QQuickRenderControl {
public:
  QWindow * m_renderWindow{ nullptr };

  QWindow * renderWindow(QPoint * offset) Q_DECL_OVERRIDE {
    if (nullptr == m_renderWindow) {
      return QQuickRenderControl::renderWindow(offset);
    }
    if (nullptr != offset) {
      offset->rx() = offset->ry() = 0;
    }
    return m_renderWindow;
  }
};

class QOffscreenUi : public QObject {
  Q_OBJECT

  bool m_paused;
public:
  QOffscreenUi();
  ~QOffscreenUi();
  void setup(const QSize & size, QOpenGLContext * context);
  void loadQml(const QUrl & qmlSource, std::function<void(QQmlContext*)> f = [](QQmlContext*){});
  QQmlContext * qmlContext();

  void pause() {
    m_paused = true;
  }

  void resume() {
    m_paused = false;
    requestRender();
  }

  void setProxyWindow(QWindow * window) {
    m_renderControl->m_renderWindow = window;
  }

private slots:
  void updateQuick();
  void run();

public slots:
  void requestUpdate();
  void requestRender();
  void lockTexture(int texture);
  void releaseTexture(int texture);

signals:
  void textureUpdated(int texture);

private:
  QMap<int, QSharedPointer<QOpenGLFramebufferObject>> m_fboMap;
  QMap<int, int> m_fboLocks;
  QQueue<QOpenGLFramebufferObject*> m_readyFboQueue;
  
  QOpenGLFramebufferObject* getReadyFbo();

public:
  QOpenGLContext *m_context{ new QOpenGLContext };
  QOffscreenSurface *m_offscreenSurface{ new QOffscreenSurface };
  QMyQuickRenderControl  *m_renderControl{ new QMyQuickRenderControl };
  QQuickWindow *m_quickWindow{ nullptr };
  QQmlEngine *m_qmlEngine{ nullptr };
  QQmlComponent *m_qmlComponent{ nullptr };
  QQuickItem * m_rootItem{ nullptr };
  QTimer m_updateTimer;
  QSize m_size;
  bool m_polish{ true };
  std::mutex renderLock;
};

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


