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

#ifdef _DEBUG
#define BRAD_DEBUG 1
#endif

#include "Common.h"

#include "Shadertoy.h"
#include "ShadertoyRenderer.h"
#include "ShadertoyIO.h"
#include "ShadertoyFetcher.h"

#include "Resources.h"

#include <QQuickTextDocument>
#include <QGuiApplication>
#include <QQmlContext>
#include <QDeclarativeEngine>

#ifndef _DEBUG
#include "TrackerbirdConfig.h"
#include "ShadertoyConfig.h"
#endif

#ifdef TRACKERBIRD_PRODUCT_ID
#include <Trackerbird.h>
#endif

const char * ORG_NAME = "Oculus Rift in Action";
const char * ORG_DOMAIN = "oculusriftinaction.com";
const char * APP_NAME = "ShadertoyVR";

static const float ROOT_2 = sqrt(2.0f);
static const float INV_ROOT_2 = 1.0f / ROOT_2;
static uvec2 UI_SIZE(1280, 720);
static float UI_ASPECT = aspect(vec2(UI_SIZE));
static float UI_INVERSE_ASPECT = 1.0f / UI_ASPECT;

static const QStringList PRESETS({
  ":/shaders/presets/default.xml",
  ":/shaders/presets/4df3DS.json",
  ":/shaders/presets/4dfGzs.json",
  ":/shaders/presets/4djGWR.json",
  ":/shaders/presets/4ds3zn.json",
  ":/shaders/presets/4dXGRM_flying_steel_cubes.xml",
  ":/shaders/presets/4sBGD1.json",
  ":/shaders/presets/4slGzn.json",
  ":/shaders/presets/4sX3R2.json",
  ":/shaders/presets/4sXGRM_oceanic.xml",
  ":/shaders/presets/4tXGDn.json",
  //":/shaders/presets/ld23DG_crazy.xml",
  ":/shaders/presets/ld2GRz.json",
  ":/shaders/presets/ldfGzr.json",
  ":/shaders/presets/ldj3Dm.json",
  ":/shaders/presets/ldl3zr_mobius_balls.xml",
  ":/shaders/presets/ldSGRW.json",
  ":/shaders/presets/lsl3W2.json",
  ":/shaders/presets/lss3WS_relentless.xml",
  ":/shaders/presets/lts3Wn.json",
  ":/shaders/presets/MdX3Rr.json",
  ":/shaders/presets/MsBGRh.json",
//    ":/shaders/presets/MsSGD1_hand_drawn_sketch.xml",
  ":/shaders/presets/MsXGz4.json",
  ":/shaders/presets/MsXGzM.json",
//    ":/shaders/presets/MtfGR8_snowglobe.xml",
  ":/shaders/presets/XdBSzd.json",
  ":/shaders/presets/Xlf3D8.json",
  ":/shaders/presets/XsBSRG_morning_city.xml",
  ":/shaders/presets/XsjXR1.json",
  ":/shaders/presets/XslXW2.json",
  ":/shaders/presets/XsSSRW.json"
});


QDir CONFIG_DIR;
QSharedPointer<QFile> LOG_FILE;
QtMessageHandler ORIGINAL_MESSAGE_HANDLER;

using namespace oglplus;


class ShadertoyWindow : public ShadertoyRenderer {
  Q_OBJECT
  
  typedef std::atomic<GLuint> AtomicGlTexture;
  typedef std::pair<GLuint, GLsync> SyncPair;
  typedef std::queue<SyncPair> TextureTrashcan;
  typedef std::atomic<QPointF> ActomicVec2;
  typedef std::vector<GLuint> TextureDeleteQueue;
  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> Lock;
  // A cache of all the input textures available 
  QDir configPath;
  QSettings settings;

  st::Shader activeShader;

  //////////////////////////////////////////////////////////////////////////////
  // 
  // Offscreen UI
  //
  QOffscreenUi * uiWindow{ new QOffscreenUi() };
  GlslHighlighter highlighter;

  int activePresetIndex{ 0 };
  float savedEyePosScale{ 1.0f };
  vec2 windowSize;
  //////////////////////////////////////////////////////////////////////////////
  // 
  // Shader Rendering information
  //

  // We actually render the shader to one FBO for dynamic framebuffer scaling,
  // while leaving the actual texture we pass to the Oculus SDK fixed.
  // This allows us to have a clear UI regardless of the shader performance
  FramebufferWrapperPtr shaderFramebuffer;

  // The current mouse position as reported by the main thread
  ActomicVec2 mousePosition;
  bool uiVisible{ false };


  // A wrapper for passing the UI texture from the app to the widget
  AtomicGlTexture uiTexture{ 0 };
  TextureTrashcan textureTrash;
  TextureDeleteQueue textureDeleteQueue;
  Mutex textureLock;
  QTimer timer;

  // GLSL and geometry for the UI
  ProgramPtr uiProgram;
  ShapeWrapperPtr uiShape;
  TexturePtr mouseTexture;
  ShapeWrapperPtr mouseShape;

  // For easy compositing the UI texture and the mouse texture
  FramebufferWrapperPtr uiFramebuffer;

  // Geometry and shader for rendering the possibly low res shader to the main framebuffer
  ProgramPtr planeProgram;
  ShapeWrapperPtr plane;

  // Measure the FPS for use in dynamic scaling
  GLuint exchangeUiTexture(GLuint newUiTexture) {
    return uiTexture.exchange(newUiTexture);
  }

  ShadertoyFetcher fetcher;

public:
  ShadertoyWindow() {
    // Fixes an occasional crash caused by a race condition between the Rift 
    // render thread and the UI thread, triggered when Rift swapbuffers overlaps 
    // with the UI thread binding a new FBO (specifically, generating a texture 
    // for the FBO.  
    // Perhaps I should just create N FBOs and have the UI object iterate over them
    {
      QString configLocation = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
      configPath = QDir(configLocation);
      configPath.mkpath("shaders");
    }

    CONFIG_DIR.mkpath("shadertoy");
    fetcher.setDestination(QDir(CONFIG_DIR.absoluteFilePath("shadertoy")));
    fetcher.fetchShaders();

    connect(&timer, &QTimer::timeout, this, &ShadertoyWindow::onTimer);
    timer.start(100);
    setupOffscreenUi();
    onLoadPreset(0);
    Platform::addShutdownHook([&] {
      shaderFramebuffer.reset();
      uiProgram.reset();
      uiShape.reset();
      uiFramebuffer.reset();
      mouseTexture.reset();
      mouseShape.reset();
      uiFramebuffer.reset();
      planeProgram.reset();
      plane.reset();
    });
  }

  virtual void stop() {
    ShadertoyRenderer::stop();
    delete uiWindow;
    uiWindow = nullptr;
  }

private:
  virtual void setup() {
    ShadertoyRenderer::setup();

    // The geometry and shader for rendering the 2D UI surface when needed
    uiProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    uiShape = oria::loadPlane(uiProgram, UI_ASPECT);

    // The geometry and shader for scaling up the rendered shadertoy effect
    // up to the full offscreen render resolution.  This is then compositied 
    // with the UI window
    planeProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    {
      GLuint vao1;
      glGenVertexArrays(1, &vao1);
      glBindVertexArray(vao1);
      GLenum err = glGetError();
      qDebug() << err;
    }

    plane = oria::loadPlane(planeProgram, 1.0);
    plane->Use();

//    mouseTexture = loadCursor(Resource::IMAGES_CURSOR_PNG);
    mouseShape = oria::loadPlane(uiProgram, UI_INVERSE_ASPECT);

    uiFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    uiFramebuffer->init(UI_SIZE);

    shaderFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    shaderFramebuffer->init(textureSize());

    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
  }

  void setupOffscreenUi() {
#if USE_RIFT
    this->endFrameLock = &uiWindow->renderLock;
#endif
    qApp->setFont(QFont("Arial", 14, QFont::Bold));
    uiWindow->pause();
    uiWindow->setup(QSize(UI_SIZE.x, UI_SIZE.y), context());
    {
      QStringList dataList;
      foreach(const QString path, PRESETS) {
        st::Shader shader = loadShaderFile(path);
        dataList.append(shader.name);
      }
      auto qmlContext = uiWindow->m_qmlEngine->rootContext();
      qmlContext->setContextProperty("presetsModel", QVariant::fromValue(dataList));
      QUrl url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/shaders");
      qmlContext->setContextProperty("userPresetsFolder", url);
    }
    uiWindow->setProxyWindow(this);

    QUrl qml = QUrl("qrc:/layouts/Combined.qml");
    uiWindow->m_qmlEngine->addImportPath("./qml");
    uiWindow->m_qmlEngine->addImportPath("./layouts");
    uiWindow->m_qmlEngine->addImportPath(".");
    uiWindow->loadQml(qml);
    connect(uiWindow, &QOffscreenUi::textureUpdated, this, [&](int textureId) {
      uiWindow->lockTexture(textureId);
      GLuint oldTexture = exchangeUiTexture(textureId);
      if (oldTexture) {
        uiWindow->releaseTexture(oldTexture);
      }
    });

    QQuickItem * editorControl;
    editorControl = uiWindow->m_rootItem->findChild<QQuickItem*>("shaderTextEdit");
    if (editorControl) {
      QVariant tdp = editorControl->property("textDocument");
      QQuickTextDocument* qtd = tdp.value<QQuickTextDocument*>();
      highlighter.setDocument(qtd->textDocument());
    }

    QObject::connect(uiWindow->m_rootItem, SIGNAL(toggleUi()),
      this, SLOT(onToggleUi()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(channelTextureChanged(int, int, QString)),
      this, SLOT(onChannelTextureChanged(int, int, QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(shaderSourceChanged(QString)),
      this, SLOT(onShaderSourceChanged(QString)));

    // FIXME add confirmation for when the user might lose edits.
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadPreset(int)),
      this, SLOT(onLoadPreset(int)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadNextPreset()),
      this, SLOT(onLoadNextPreset()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadPreviousPreset()),
      this, SLOT(onLoadPreviousPreset()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(loadShaderFile(QString)),
      this, SLOT(onLoadShaderFile(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(saveShaderXml(QString)),
      this, SLOT(onSaveShaderXml(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(recenterPose()),
      this, SLOT(onRecenterPosition()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(modifyTextureResolution(double)),
      this, SLOT(onModifyTextureResolution(double)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(modifyPositionScale(double)),
      this, SLOT(onModifyPositionScale(double)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(resetPositionScale()),
      this, SLOT(onResetPositionScale()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(toggleEyePerFrame()),
      this, SLOT(onToggleEyePerFrame()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(epfModeChanged(bool)),
      this, SLOT(onEpfModeChanged(bool)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(startShutdown()),
      this, SLOT(onShutdown()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(restartShader()),
      this, SLOT(onRestartShader()));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(newShaderFilepath(QString)),
      this, SLOT(onNewShaderFilepath(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(newShaderHighlighted(QString)),
      this, SLOT(onNewShaderHighlighted(QString)));
    QObject::connect(uiWindow->m_rootItem, SIGNAL(newPresetHighlighted(int)),
      this, SLOT(onNewPresetHighlighted(int)));

    QObject::connect(this, &ShadertoyWindow::compileSuccess, this, [&] {
      setItemProperty("errorFrame", "height", 0);
      setItemProperty("errorFrame", "visible", false);
      setItemProperty("compileErrors", "text", "");
      setItemProperty("shaderTextFrame", "errorMargin", 0);
    });

    QObject::connect(this, &ShadertoyWindow::compileError, this, [&](const QString& errors){
      setItemProperty("errorFrame", "height", 128);
      setItemProperty("errorFrame", "visible", true);
      setItemProperty("compileErrors", "text", errors);
      setItemProperty("shaderTextFrame", "errorMargin", 8);
    });

    connect(this, &ShadertoyWindow::fpsUpdated, this, [&](float fps) {
      setItemText("fps", QString().sprintf("%0.0f", fps));
    });

    setItemText("res", QString().sprintf("%0.2f", texRes));
  }

  QVariant getItemProperty(const QString & itemName, const QString & property) {
    QQuickItem * item = uiWindow->m_rootItem->findChild<QQuickItem*>(itemName);
    if (nullptr != item) {
      return item->property(property.toLocal8Bit());
    } else {
      qWarning() << "Could not find item " << itemName << " on which to set property " << property;
    }
    return QVariant();
  }

  void setItemProperty(const QString & itemName, const QString & property, const QVariant & value) {
    QQuickItem * item = uiWindow->m_rootItem->findChild<QQuickItem*>(itemName);
    if (nullptr != item) {
      bool result = item->setProperty(property.toLocal8Bit(), value);
      if (!result) {
        qWarning() << "Set property " << property << " on item " << itemName << " returned " << result;
      }
    } else {
      qWarning() << "Could not find item " << itemName << " on which to set property " << property;
    }
  }

  void setItemText(const QString & itemName, const QString & text) {
    setItemProperty(itemName, "text", text);
  }

  QString getItemText(const QString & itemName) {
    return getItemProperty(itemName, "text").toString();
  }

private slots:
  void onToggleUi() {
	  uiVisible = !uiVisible;
	  setItemProperty("shaderTextEdit", "readOnly", !uiVisible);
	  if (uiVisible) {
		savedEyePosScale = eyePosScale;
		eyePosScale = 0.0f;
    uiWindow->resume();
	  } else {
		eyePosScale = savedEyePosScale;
    uiWindow->pause();
	  }
  }

  void onLoadNextPreset() {
    static const int PRESETS_SIZE = PRESETS.size();
    int newPreset = (activePresetIndex + 1) % PRESETS_SIZE;
    onLoadPreset(newPreset);
  }

  void onFontSizeChanged(int newSize) {
    settings.setValue("fontSize", newSize);
  }

  void onLoadPreviousPreset() {
    static const int PRESETS_SIZE = PRESETS.size();
    int newPreset = (activePresetIndex + PRESETS_SIZE - 1) % PRESETS_SIZE;
    onLoadPreset(newPreset);
  }

  void onLoadPreset(int index) {
    activePresetIndex = index;
    loadShader(loadShaderFile(PRESETS.at(index)));
  }

  void onLoadShaderFile(const QString & shaderPath) {
    qDebug() << "Loading shader from " << shaderPath;
    if (shaderPath.endsWith(".xml")) {
      loadShader(loadShaderXml(shaderPath));
    } else if (shaderPath.endsWith(".json")) {
      loadShader(loadShaderJson(shaderPath));
    }
  }

  void onNewShaderFilepath(const QString & shaderPath) {
    QDir newDir(shaderPath);
    QUrl url = QUrl::fromLocalFile(newDir.absolutePath());
//    setItemProperty("userPresetsModel", "folder", url);
    auto qmlContext = uiWindow->m_qmlEngine->rootContext();
    qmlContext->setContextProperty("userPresetsFolder", url);
  }
  
  void onNewShaderHighlighted(const QString & shaderPath) {
    qDebug() << "New shader highlighted " << shaderPath;
    QString previewPath = shaderPath;
    previewPath.replace(QRegularExpression("\\.(json|xml)$"), ".jpg");
    setItemProperty("previewImage", "source", QFile::exists(previewPath) ? QUrl::fromLocalFile(previewPath) : QUrl());
    if (shaderPath.endsWith(".json")) {
      setItemProperty("loadRoot", "activeShaderString", QtUtil::toString(shaderPath));
    } else {
      setItemProperty("loadRoot", "activeShaderString", "");
    }
  }

  void onNewPresetHighlighted(int presetId) {
    if (-1 != presetId && presetId < PRESETS.size()) {
      QString path = PRESETS.at(presetId);
      QString previewPath = path;
      previewPath.replace(QRegularExpression("\\.(json|xml)$"), ".jpg");
      setItemProperty("previewImage", "source", "qrc" + previewPath);
      qDebug() << previewPath;
    }
    //previewPath.replace(QRegularExpression("\\.(json|xml)$"), ".jpg");
    //setItemProperty("previewImage", "source", QFile::exists(previewPath) ? QUrl::fromLocalFile(previewPath) : QUrl());
    //if (shaderPath.endsWith(".json")) {
    //  setItemProperty("loadRoot", "activeShaderString", QtUtil::toString(shaderPath));
    //} else {
    //  setItemProperty("loadRoot", "activeShaderString", "");
    //}
  }

  void onSaveShaderXml(const QString & shaderPath) {
    Q_ASSERT(!shaderPath.isEmpty());
    //st::saveShaderXml(activeShader);
    activeShader.name = shaderPath.toLocal8Bit();
    activeShader.fragmentSource = getItemText("shaderTextEdit").toLocal8Bit();
    QString destinationFile = configPath.absoluteFilePath(QString("shaders/")
      + shaderPath
      + ".xml");
    qDebug() << "Saving shader to " << destinationFile;
    saveShaderXml(destinationFile, activeShader);
  }

  void onChannelTextureChanged(const int & channelIndex, const int & channelType, const QString & texturePath) {
    qDebug() << "Requesting texture from path " << texturePath;
    queueRenderThreadTask([&, channelIndex, channelType, texturePath] {
      qDebug() << texturePath;
      activeShader.channelTypes[channelIndex] = (st::ChannelInputType)channelType;
      activeShader.channelTextures[channelIndex] = texturePath.toLocal8Bit();
      setChannelTextureInternal(channelIndex, 
        (st::ChannelInputType)channelType,
        texturePath);
      updateUniforms();
    });
  }

  void onShaderSourceChanged(const QString & shaderSource) {
    queueRenderThreadTask([&, shaderSource] {
      setShaderSourceInternal(shaderSource);
      updateUniforms();
    });
  }

  void onRecenterPosition() {
#if USE_RIFT
    queueRenderThreadTask([&] {
      ovrHmd_RecenterPose(hmd);
    });
#endif
  }

  void onModifyTextureResolution(double scale) {
    float newRes = scale * texRes;
    newRes = std::max(0.1f, std::min(1.0f, newRes));
    if (newRes != texRes) {
      queueRenderThreadTask([&, newRes] {
        texRes = newRes;
      });
      setItemText("res", QString().sprintf("%0.2f", newRes));
    }
  }

  void onModifyPositionScale(double scale) {
    float newPosScale = scale * eyePosScale;
    queueRenderThreadTask([&, newPosScale] {
      eyePosScale = newPosScale;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", newPosScale));
  }

  void onResetPositionScale() {
    queueRenderThreadTask([&] {
      eyePosScale = 1.0f;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", 1.0f));
  }

  void onToggleEyePerFrame() {
#if USE_RIFT
    onEpfModeChanged(!eyePerFrameMode);
#endif
  }

  void onEpfModeChanged(bool checked) {
    bool newEyePerFrameMode = checked;
#if USE_RIFT
    queueRenderThreadTask([&, newEyePerFrameMode] {
      eyePerFrameMode = newEyePerFrameMode;
    });
#endif
    setItemProperty("epf", "checked", newEyePerFrameMode);
  }

  void onRestartShader() {
    queueRenderThreadTask([&] {
      shaderTime.start();
    });
  }

  void onShutdown() {
    QApplication::instance()->quit();
  }

  void onTimer() {
    TextureDeleteQueue tempTextureDeleteQueue;

    // Scope the lock tightly
    {
      Lock lock(textureLock);
      if (!textureDeleteQueue.empty()) {
        tempTextureDeleteQueue.swap(textureDeleteQueue);
      }
    }

    if (!tempTextureDeleteQueue.empty()) {
      std::for_each(tempTextureDeleteQueue.begin(), tempTextureDeleteQueue.end(), [&] (int usedTexture){
        uiWindow->releaseTexture(usedTexture);
      });
    }
  }

private:
  void loadShader(const st::Shader & shader) {
    assert(!shader.fragmentSource.isEmpty());
    activeShader = shader;
    setItemText("shaderTextEdit", QString(shader.fragmentSource).replace(QString("\t"), QString("  ")));
    setItemText("shaderName", shader.name);
    for (int i = 0; i < 4; ++i) {
      QString texturePath = activeShader.channelTextures[i];
      while (canonicalPathMap.count(texturePath)) {
        texturePath = canonicalPathMap[texturePath];
      }
      qDebug() << "Setting channel " << i << " to texture " << texturePath;
      setItemProperty(QString().sprintf("channel%d", i), "source", "qrc:" + texturePath);
    }
    // FIXME update the channel texture buttons
    queueRenderThreadTask([&, shader] {
      setShaderInternal(shader);
      updateUniforms();
    });
  }

  void updateFps(float fps) {
    emit fpsUpdated(fps);
  }

  ///////////////////////////////////////////////////////
  //
  // Event handling customization
  // 
  void mouseMoveEvent(QMouseEvent * me) { 
    // Make sure we don't show the system cursor over the window
    qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
    // Interpret the mouse position as NDC coordinates
    QPointF mp = me->localPos();
    mp.rx() /= size().width();
    mp.ry() /= size().height();
    mp *= 2.0f;
    mp -= QPointF(1.0f, 1.0f);
    mp.ry() *= -1.0f;
    mousePosition.store(mp);
    QRiftWindow::mouseMoveEvent(me);
  }

  bool event(QEvent * e) {
    static bool dismissedHmd = false;
    switch (e->type()) {
    case QEvent::KeyPress:
#if USE_RIFT
      if (!dismissedHmd) {
        // Allow the user to remove the HSW message early
        ovrHSWDisplayState hswState;
        ovrHmd_GetHSWDisplayState(hmd, &hswState);
        if (hswState.Displayed) {
          ovrHmd_DismissHSWDisplay(hmd);
          dismissedHmd = true;
          return true;
        }
      }
#endif
        
    case QEvent::KeyRelease:
    {
      if (QApplication::sendEvent(uiWindow->m_quickWindow, e)) {
        return true;
      }
    }
    break;

    case QEvent::Wheel:
    {
      QWheelEvent * we = (QWheelEvent*)e;
      QWheelEvent mappedEvent(mapWindowToUi(we->pos()), we->delta(), we->buttons(), we->modifiers(), we->orientation());
      QCoreApplication::sendEvent(uiWindow->m_quickWindow, &mappedEvent);
      return true;
    }
    break;

    case QEvent::MouseMove:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    {
      QMouseEvent * me = (QMouseEvent *)e;
      QMouseEvent mappedEvent(e->type(), mapWindowToUi(me->localPos()), me->screenPos(), me->button(), me->buttons(), me->modifiers());
      QCoreApplication::sendEvent(uiWindow->m_quickWindow, &mappedEvent);
      return QRiftWindow::event(e);
    }

    default: break;
    }

    return QRiftWindow::event(e);
  }

  void resizeEvent(QResizeEvent *e) {
    windowSize = vec2(e->size().width(), e->size().height());
  }

  QPointF mapWindowToUi(const QPointF & p) {
    vec2 pos = vec2(p.x(), p.y());
    pos /= windowSize;
    pos *= UI_SIZE;
    return QPointF(pos.x, pos.y);
  }

  ///////////////////////////////////////////////////////
  //
  // Rendering functionality
  // 
  void perFrameRender() {
    Context::Enable(Capability::Blend);
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::ScissorTest);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    if (uiVisible) {
      static GLuint lastUiTexture = 0;
      static GLsync lastUiSync;
      GLuint currentUiTexture = uiTexture.exchange(0);
      if (0 == currentUiTexture) {
        currentUiTexture = lastUiTexture;
      } else {
        // If the texture has changed, push it into the trash bin for
        // deletion once it's finished rendering
        if (lastUiTexture) {
          textureTrash.push(SyncPair(lastUiTexture, lastUiSync));
        }
        lastUiTexture = currentUiTexture;
      }
      MatrixStack & mv = Stacks::modelview();
      if (currentUiTexture) {
        Texture::Active(0);
        // Composite the UI image and the mouse sprite
        uiFramebuffer->Bound([&] {
          Context::Clear().ColorBuffer();
          oria::viewport(UI_SIZE);
          // Clear out the projection and modelview here.
          Stacks::withIdentity([&] {
            glBindTexture(GL_TEXTURE_2D, currentUiTexture);
            oria::renderGeometry(plane, uiProgram);
            
            // Render the mouse sprite on the UI
            QPointF mp = mousePosition.load();
            mv.translate(vec3(mp.x(), mp.y(), 0.0f));
            mv.scale(vec3(0.1f));
//            mouseTexture->Bind(Texture::Target::_2D);
//            oria::renderGeometry(mouseShape, uiProgram);
          });
        });
        lastUiSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      }
    }
    
    TextureDeleteQueue tempTextureDeleteQueue;
    while (!textureTrash.empty()) {
      SyncPair & top = textureTrash.front();
      GLuint & texture = top.first;
      GLsync & sync = top.second;
      GLenum result = glClientWaitSync(sync, 0, 0);
      if (GL_ALREADY_SIGNALED == result || GL_CONDITION_SATISFIED == result) {
        tempTextureDeleteQueue.push_back(texture);
        textureTrash.pop();
      } else {
        break;
      }
    }
    
    if (!tempTextureDeleteQueue.empty()) {
      Lock lock(textureLock);
      textureDeleteQueue.insert(textureDeleteQueue.end(),
                                tempTextureDeleteQueue.begin(), tempTextureDeleteQueue.end());
    }
  }

  void perEyeRender() {
    // Render the shadertoy effect into a framebuffer, possibly at a 
    // smaller resolution than recommended
    shaderFramebuffer->Bound([&] {
      Context::Clear().ColorBuffer();
      oria::viewport(renderSize());
      renderShadertoy();
    });
    oria::viewport(textureSize());

    // Now re-render the shader output to the screen.
    shaderFramebuffer->BindColor(Texture::Target::_2D);
#if USE_RIFT
    if (vrMode) {
#endif
      // In VR mode, we want to cover the entire surface
      Stacks::withIdentity([&] {
        oria::renderGeometry(plane, planeProgram, LambdaList({ [&] {
          Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
        } }));
      });
#if USE_RIFT
    } else {
      // In 2D mode, we want to render it as a window behind the UI
      Context::Clear().ColorBuffer();
      MatrixStack & mv = Stacks::modelview();
      static vec3 scale = vec3(3.0f, 3.0f / (textureSize().x / textureSize().y), 3.0f);
      static vec3 trans = vec3(0, 0, -3.5);
      static mat4 rot = glm::rotate(mat4(), PI / 2.0f, Vectors::Y_AXIS);


      for (int i = 0; i < 4; ++i) {
        mv.withPush([&] {
          for (int j = 0; j < i; ++j) {
            mv.postMultiply(rot);
          }
          mv.translate(trans);
          mv.scale(scale);
          oria::renderGeometry(plane, planeProgram, LambdaList({ [&] {
            Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
          } }));
          oria::renderGeometry(plane, planeProgram);
        });
      }
    }
#endif

    if (uiVisible) {
      MatrixStack & mv = Stacks::modelview();
      Texture::Active(0);
//      oria::viewport(textureSize());
      mv.withPush([&] {
        mv.translate(vec3(0, 0, -1));
        uiFramebuffer->BindColor();
        oria::renderGeometry(uiShape, uiProgram);
      });
    }
  }

public slots:

  void onSixDofMotion(const vec3 & tr, const vec3 & mo) {
    qDebug() << QString().sprintf("%f, %f, %f", tr.x, tr.y, tr.z);
    queueRenderThreadTask([&, tr, mo] {
      position += tr;
    });
  }

signals:
  void fpsUpdated(float);
};



void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  ORIGINAL_MESSAGE_HANDLER(type, context, msg);
  QByteArray localMsg = msg.toLocal8Bit();
  QString now = QDateTime::currentDateTime().toString("yyyy.dd.MM_hh:mm:ss");
  switch (type) {
  case QtDebugMsg:
    LOG_FILE->write(QString().sprintf("%s Debug:    %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
    break;
  case QtWarningMsg:
    LOG_FILE->write(QString().sprintf("%s Warning:  %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
    break;
  case QtCriticalMsg:
    LOG_FILE->write(QString().sprintf("%s Critical: %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
    break;
  case QtFatalMsg:
    LOG_FILE->write(QString().sprintf("%s Fatal:    %s (%s:%u, %s)\n", now.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function).toLocal8Bit());
    LOG_FILE->flush();
    abort();
  }
  LOG_FILE->flush();
}

class ShadertoyApp : public QApplication {
  Q_OBJECT

  QWidget desktopWindow;

public:
  ShadertoyApp(int argc, char ** argv) : QApplication(argc, argv) {
    Q_INIT_RESOURCE(ShadertoyVR);
    QCoreApplication::setOrganizationName(ORG_NAME);
    QCoreApplication::setOrganizationDomain(ORG_DOMAIN);
    QCoreApplication::setApplicationName(APP_NAME);
#if (!defined(_DEBUG) && defined(TRACKERBIRD_PRODUCT_ID))
    QCoreApplication::setApplicationVersion(QString::fromWCharArray(TRACKERBIRD_PRODUCT_VERSION));
#endif
    CONFIG_DIR = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    CONFIG_DIR.mkpath("shadertoy");
    QString currentLogName = CONFIG_DIR.absoluteFilePath("ShadertoyVR.log");
    LOG_FILE = QSharedPointer<QFile>(new QFile(currentLogName));
    if (LOG_FILE->exists()) {
      QFile::rename(currentLogName, 
        CONFIG_DIR.absoluteFilePath("ShadertoyVR_" + 
          QDateTime::currentDateTime().toString("yyyy.dd.MM_hh.mm.ss") + ".log"));
    }
    if (!LOG_FILE->open(QIODevice::WriteOnly | QIODevice::Append)) {
      qWarning() << "Could not open log file";
    }
    ORIGINAL_MESSAGE_HANDLER = qInstallMessageHandler(myMessageOutput);
  }

  virtual ~ShadertoyApp() {
    qInstallMessageHandler(ORIGINAL_MESSAGE_HANDLER);
    LOG_FILE->close();
  }

private:
  void setupDesktopWindow() {
    desktopWindow.setLayout(new QFormLayout());
    QLabel * label = new QLabel("Your Oculus Rift is now active.  Please put on your headset.  Share and enjoy");
    desktopWindow.layout()->addWidget(label);
    desktopWindow.show();
  }
};

int main(int argc, char ** argv) {
  try {
#if USE_RIFT
    ovr_Initialize();
#endif

#if (!defined(_DEBUG) && defined(TRACKERBIRD_PRODUCT_ID))
    tbCreateConfig(TRACKERBIRD_URL, TRACKERBIRD_PRODUCT_ID, 
      TRACKERBIRD_PRODUCT_VERSION, TRACKERBIRD_BUILD_NUMBER, 
      TRACKERBIRD_MULTISESSION_ENABLED);
    tbStart();
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", "./plugins");
#endif

    ShadertoyApp app(argc, argv);
    ShadertoyWindow * riftRenderWidget;
    riftRenderWidget = new ShadertoyWindow();
    riftRenderWidget->start();
    riftRenderWidget->requestActivate();
    int result = app.exec(); 

#if (!defined(_DEBUG) && defined(TRACKERBIRD_PRODUCT_ID))
    tbStop(TRUE);
#endif

    riftRenderWidget->stop();
    riftRenderWidget->makeCurrent();
    Platform::runShutdownHooks();
    delete riftRenderWidget;
#if USE_RIFT
    ovr_Shutdown();
#endif
    return result;
  } catch (std::exception & error) { 
    qWarning() << error.what();
  } catch (const std::string & error) { 
    qWarning() << error.c_str();
  } 
  return -1; 
} 



#include "ShadertoyVR.moc"




#if 0
SiHdl si;
SiGetEventData getEventData;
bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) {
  // On Windows, eventType is set to "windows_generic_MSG" for messages sent 
  // to toplevel windows, and "windows_dispatcher_MSG" for system - wide 
  // messages such as messages from a registered hot key.In both cases, the 
  // message can be casted to a MSG pointer.The result pointer is only used 
  // on Windows, and corresponds to the LRESULT pointer.
  MSG & msg = *(MSG*)message;
  SiSpwEvent siEvent;
  vec3 tr, ro;
  long * md = siEvent.u.spwData.mData;
  SiGetEventWinInit(&getEventData, msg.message, msg.wParam, msg.lParam);
  if (SiGetEvent(si, 0, &getEventData, &siEvent) == SI_IS_EVENT) {
    switch (siEvent.type) {
    case SI_MOTION_EVENT:
      emit sixDof(
        vec3(md[SI_TX], md[SI_TY], -md[SI_TZ]),
        vec3(md[SI_RX], md[SI_RY], md[SI_RZ]));
      break;

    default:
      break;
    }
    return true;
  }
  return false;
}

SpwRetVal result = SiInitialize();
int cnt = SiGetNumDevices();
SiDevID devId = SiDeviceIndex(0);
SiOpenData siData;
SiOpenWinInit(&siData, (HWND)riftRenderWidget.effectiveWinId());
si = SiOpen("app", devId, SI_NO_MASK, SI_EVENT, &siData);
installNativeEventFilter(this);

#endif



/* 
yes: 
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xlf3D8.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XsjXR1.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XslXW2.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ld2GRz.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldSGRW.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldfGzr.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4slGzn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldj3Dm.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/lsl3W2.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/lts3Wn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XdBSzd.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/MsBGRh.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4ds3zn.json"

maybe:

Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xll3Wn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xsl3Rl.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldSGzR.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldX3Ws.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldsGWB.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/lsBXD1.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Xds3zN.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4sl3zn.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4sjXzG.json"

debug:
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/ldjXD3.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/XdfXDB.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/Msl3Rr.json"
Loading shader from  "c:/Users/bdavis/AppData/Local/Oculus Rift in Action/ShadertoyVR/shadertoy/4sXGDs.json"

*/
