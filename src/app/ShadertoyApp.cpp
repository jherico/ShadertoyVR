#include "Common.h"

#include "ShadertoyApp.h"

#include <QtQml/QQmlContext>

#include "qt/QtUtils.h"
#include "qt/GlslEditor.h"

#include "CodeEditor.h"
#include "ChannelsColumn.h"
#include "LoadWindow.h"
#include "EditWindow.h"


using namespace oglplus;
using namespace Plugins::Display;

extern QDir CONFIG_DIR;
static const float ROOT_2 = sqrt(2.0f);
static const float INV_ROOT_2 = 1.0f / ROOT_2;
static uvec2 UI_SIZE(1280, 720);
static float UI_ASPECT = aspect(vec2(UI_SIZE));
static float UI_INVERSE_ASPECT = 1.0f / UI_ASPECT;

const QStringList PRESETS({
    ":/shadertoys/default.xml",
    ":/shadertoys/4df3DS.json",
    ":/shadertoys/4dfGzs.json",
    ":/shadertoys/4djGWR.json",
    ":/shadertoys/4ds3zn.json",
    ":/shadertoys/4dXGRM_flying_steel_cubes.xml",
    // ":/shadertoys/4sBGD1.json",
    // ":/shadertoys/4slGzn.json",
    ":/shadertoys/4sX3R2.json", // Monster
    ":/shadertoys/4sXGRM_oceanic.xml",
    ":/shadertoys/4tXGDn.json", // Morphing
    //":/shadertoys/ld23DG_crazy.xml",
    ":/shadertoys/ld2GRz.json", // meta-balls
    // ":/shadertoys/ldfGzr.json",
    // ":/shadertoys/ldj3Dm.json", // fish swimming
    ":/shadertoys/ldl3zr_mobius_balls.xml",
    // ":/shadertoys/ldSGRW.json",
    // ":/shadertoys/lsl3W2.json",
    ":/shadertoys/lss3WS_relentless.xml",
    // ":/shadertoys/lts3Wn.json",
    ":/shadertoys/MdX3Rr.json", // elevated
    // ":/shadertoys/MsBGRh.json",
    // ":/shadertoys/MsSGD1_hand_drawn_sketch.xml",
    ":/shadertoys/MsXGz4.json", // cubemap
    ":/shadertoys/MsXGzM.json", // voronoi rocks
    // ":/shadertoys/MtfGR8_snowglobe.xml",
    // ":/shadertoys/XdBSzd.json",
    ":/shadertoys/Xlf3D8.json", // sci-fi
    ":/shadertoys/XsBSRG_morning_city.xml",
    ":/shadertoys/XsjXR1.json", // worms
    ":/shadertoys/XslXW2.json", // mechanical (2D)
    // ":/shadertoys/XsSSRW.json"
});


const char * ORG_NAME = "Oculus Rift in Action";
const char * ORG_DOMAIN = "oculusriftinaction.com";
const char * APP_NAME = "ShadertoyVR";

QDir CONFIG_DIR;
QSharedPointer<QFile> LOG_FILE;
QtMessageHandler ORIGINAL_MESSAGE_HANDLER;
static const QUrl SOURCE{ "file:///C:/Users/bdavis/Git/ShadertoyVR/res/qml/DesktopWindow.qml" };
QQuickView* MAIN_WINDOW{ nullptr };
Plugin** DISPLAY_PLUGINS{ nullptr };
size_t DISPLAY_PLUGIN_COUNT;

struct TextureContainer {
    GLuint texture{ 0 };
    GLsync writeSync{ 0 };
    GLsync readSync{ 0 };
    QMutex lock;

    template <typename F>
    void executeReadOperation(F f) {
        lock.lock();
        if (writeSync) {
            glWaitSync(writeSync, 0, GL_TIMEOUT_IGNORED);
        }
        f();
        if (readSync) {
            glDeleteSync(readSync);
        }
        readSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        lock.unlock();
    }
};

static TextureContainer _uiTextureContainer;
static TextureContainer _shaderTextureContainer;

TexturePtr _testTexture;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
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

ShadertoyApp::ShadertoyApp(int argc, char ** argv) : QGuiApplication(argc, argv) {
    Q_INIT_RESOURCE(ShadertoyVR);
    initTracker();
    initAppInformation();
    initResources();
    initSettings();
    initLogging();
    initTypes();
    initDisplayPlugins();
    initUi();

    setupGlContext();
    setupOffscreenUi();
    setupRenderer();
}

ShadertoyApp::~ShadertoyApp() {
    shutdownTracker();
    shutdownLogging();
}

void ShadertoyApp::initTracker() {
#if (!defined(_DEBUG) && defined(TRACKERBIRD_PRODUCT_ID))
    tbCreateConfig(TRACKERBIRD_URL, TRACKERBIRD_PRODUCT_ID,
        TRACKERBIRD_PRODUCT_VERSION, TRACKERBIRD_BUILD_NUMBER,
        TRACKERBIRD_MULTISESSION_ENABLED);
    tbStart();
#endif
}

void ShadertoyApp::initDisplayPlugins() {
    DISPLAY_PLUGIN_COUNT = Plugins::Display::list(nullptr);
    DISPLAY_PLUGINS = new Plugins::Display::Plugin*[DISPLAY_PLUGIN_COUNT];
}

void ShadertoyApp::shutdownTracker() {
#if (!defined(_DEBUG) && defined(TRACKERBIRD_PRODUCT_ID))
    tbStop(TRUE);
#endif
}

void ShadertoyApp::initAppInformation() {
    QCoreApplication::setOrganizationName(ORG_NAME);
    QCoreApplication::setOrganizationDomain(ORG_DOMAIN);
    QCoreApplication::setApplicationName(APP_NAME);
#if (!defined(_DEBUG) && defined(TRACKERBIRD_PRODUCT_ID))
    QCoreApplication::setApplicationVersion(QString::fromWCharArray(TRACKERBIRD_PRODUCT_VERSION));
#endif
}

void ShadertoyApp::initResources() {
    Q_INIT_RESOURCE(ShadertoyVR);
}

void ShadertoyApp::initSettings() {
    CONFIG_DIR = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
}

void ShadertoyApp::initLogging() {
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
    ORIGINAL_MESSAGE_HANDLER = qInstallMessageHandler(messageHandler);
}

void ShadertoyApp::shutdownLogging() {
    qInstallMessageHandler(ORIGINAL_MESSAGE_HANDLER);
    LOG_FILE->close();
}

void ShadertoyApp::initTypes() {
    qmlRegisterType<MainWindow>("ShadertoyVR", 1, 0, "MainWindow");
    qmlRegisterType<CodeEditor>("ShadertoyVR", 1, 0, "CodeEditor");
    qmlRegisterType<ChannelsColumn>("ShadertoyVR", 1, 0, "ChannelsColumn");
    qmlRegisterType<LoadWindow>("ShadertoyVR", 1, 0, "LoadWindow");
    qmlRegisterType<EditWindow>("ShadertoyVR", 1, 0, "EditWindow");
}


void ShadertoyApp::initUi() {
    static QQmlEngine engine;
    MAIN_WINDOW = new QQuickView;
    MAIN_WINDOW->setSource(SOURCE);
    MAIN_WINDOW->show();
}

void ShadertoyApp::setupGlContext() {
    _surface.create(nullptr);
    _surface.makeCurrent();
    glewExperimental = true;
    glewInit();
    glGetError();
    _testTexture = oria::load2dTexture(Resource::IMAGES_CUBE_TEXTURE_PNG);
    _testTexture->Bind(oglplus::Texture::Target::_2D);
    shaderFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());

    qDebug() << "GL Version: " << QString((const char*)glGetString(GL_VERSION));
    qDebug() << "GL Shader Language Version: " << QString((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    qDebug() << "GL Vendor: " << QString((const char*)glGetString(GL_VENDOR));
    qDebug() << "GL Renderer: " << QString((const char*)glGetString(GL_RENDERER));

    _surface.doneCurrent();
}

void ShadertoyApp::setupOffscreenUi() {
    _uiSurface.pause();
    _uiSurface.create(_surface.context());
    {
        QStringList dataList;
        foreach(const QString path, PRESETS) {
            shadertoy::Shader shader = shadertoy::loadShaderFile(path);
            dataList.append(shader.name);
        }
        auto qmlContext = _uiSurface.getEngine()->rootContext();
        qmlContext->setContextProperty("presetsModel", QVariant::fromValue(dataList));
        qmlContext->setContextProperty("activeShader", &_activeShader);
        QUrl url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/shaders");
        qmlContext->setContextProperty("userPresetsFolder", url);
    }


    QUrl qml = QUrl("qrc:/qml/Combined.qml");
    //_uiSurface.m_qmlEngine->addImportPath("./qml");
    //_uiSurface.m_qmlEngine->addImportPath("./layouts");
    //_uiSurface.m_qmlEngine->addImportPath(".");
    _uiSurface.load(qml);
    connect(&_uiSurface, &OffscreenQmlSurface::textureUpdated, this, [=](int textureId) {
        onUiTextureReady(textureId, 0);
    });

    auto rootItem = _uiSurface.getRootItem();

    _codeEditor = rootItem->findChild<CodeEditor*>();
    _codeEditor->setHighlighter(new GlslHighlighter());
    _codeEditor->setText("Test text");
    _codeEditor->setErrorText("Test error text");
    
    // FIXME add confirmation for when the user might lose edits.
    QObject::connect(rootItem, SIGNAL(loadPreset(int)), this, SLOT(loadPreset(int)));
    QObject::connect(rootItem, SIGNAL(loadNextPreset()), this, SLOT(loadNextPreset()));
    QObject::connect(rootItem, SIGNAL(loadPreviousPreset()), this, SLOT(loadPreviousPreset()));
    QObject::connect(rootItem, SIGNAL(loadShaderFile(QString)), this, SLOT(loadShaderFile(QString)));

//    QObject::connect(rootItem, SIGNAL(channelTextureChanged(int, int, QString)),
//        this, SLOT(onChannelTextureChanged(int, int, QString)));
    /*
    QObject::connect(rootItem, SIGNAL(toggleUi()),
        this, SLOT(onToggleUi()));
    QObject::connect(rootItem, SIGNAL(shaderSourceChanged(QString)),
        this, SLOT(onShaderSourceChanged(QString)));

    QObject::connect(rootItem, SIGNAL(saveShaderXml(QString)),
        this, SLOT(onSaveShaderXml(QString)));
    QObject::connect(rootItem, SIGNAL(recenterPose()),
        this, SLOT(onRecenterPosition()));
    QObject::connect(rootItem, SIGNAL(modifyTextureResolution(double)),
        this, SLOT(onModifyTextureResolution(double)));
    QObject::connect(rootItem, SIGNAL(modifyPositionScale(double)),
        this, SLOT(onModifyPositionScale(double)));
    QObject::connect(rootItem, SIGNAL(resetPositionScale()),
        this, SLOT(onResetPositionScale()));
    QObject::connect(rootItem, SIGNAL(toggleEyePerFrame()),
        this, SLOT(onToggleEyePerFrame()));
    QObject::connect(rootItem, SIGNAL(epfModeChanged(bool)),
        this, SLOT(onEpfModeChanged(bool)));
    QObject::connect(rootItem, SIGNAL(startShutdown()),
        this, SLOT(onShutdown()));
    QObject::connect(rootItem, SIGNAL(restartShader()),
        this, SLOT(onRestartShader()));
    QObject::connect(rootItem, SIGNAL(newShaderFilepath(QString)),
        this, SLOT(onNewShaderFilepath(QString)));
    QObject::connect(rootItem, SIGNAL(newShaderHighlighted(QString)),
        this, SLOT(onNewShaderHighlighted(QString)));
    QObject::connect(rootItem, SIGNAL(newPresetHighlighted(int)),
        this, SLOT(onNewPresetHighlighted(int)));
    */
    loadPreset(0);
}


void ShadertoyApp::setupRenderer() {
    _surface.makeCurrent();
    _renderer.setup();
    _surface.doneCurrent();

    QObject::connect(&_renderer, &Renderer::compileSuccess, this, [&] {
        _codeEditor->setErrorText("");
    });

    QObject::connect(&_renderer, &Renderer::compileError, this, [&](const QString& errors) {
        _codeEditor->setErrorText(errors);
    });

    QObject::connect(&animation, &QVariantAnimation::valueChanged, this, [&](const QVariant & val) {
        animationValue = val.toFloat();
    });

    //connect(this, &ShadertoyApp::fpsUpdated, this, [&](float fps) {
    //    _uiSurface.setItemText("fps", QString().sprintf("%0.0f", fps));
    //});
    //_uiSurface.setItemText("res", QString().sprintf("%0.2f", texRes));
}

bool ShadertoyApp::eventFilter(QObject* receiver, QEvent* event) {
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
        case Qt::Key_Q:
            if (ke->modifiers() & Qt::ControlModifier) {
                quit();
                return true;
            }
            break;

        case Qt::Key_Escape:
        case Qt::Key_F1:
            toggleUi();
            return true;

        case Qt::Key_F3:
            toggleEyePerFrame();
            return true;

        case Qt::Key_F4:
            if (ke->modifiers() == Qt::ShiftModifier) {
                restartShader();
            } else {
                buildShader();
            }
            break;

        case Qt::Key_F5:
            //if (Qt::ShiftModifier == ke->modifiers()) {
            //    modifyPositionScale(Math.SQRT1_2);
            //} else {
            //    modifyTextureResolution(Math.SQRT1_2);
            //}
            break;

        case Qt::Key_F6:
            //if (Qt::ShiftModifier == ke->modifiers()) {
            //    resetPositionScale();
            //} else {
            //    modifyTextureResolution(0.95);
            //}
            break;

        case Qt::Key_F7:
            //if (Qt::ShiftModifier == ke->modifiers()) {
            //    resetPositionScale();
            //} else {
            //    modifyTextureResolution(1.05);
            //}
            break;

        case Qt::Key_F8:
            //if (Qt::ShiftModifier == ke->modifiers()) {
            //    modifyPositionScale(Math.SQRT2);
            //} else {
            //    modifyTextureResolution(Math.SQRT2);
            //}
            break;

        case Qt::Key_F9:
            loadPreviousPreset();
            break;

        case Qt::Key_F10:
            loadNextPreset();
            break;

        case Qt::Key_F11:
            //saveShader();
            break;

        case Qt::Key_F2:
        case Qt::Key_F12:
            recenterPose();
            break;

        default:
            break;
        }
    }
    return false;
}

void ShadertoyApp::activatePlugin(int index) {
    Plugin* newPlugin = DISPLAY_PLUGINS[index];
    newPlugin->setShareContext(_surface.context());
    if (!newPlugin->start()) {
        qDebug() << "Failed to init plugin";
        Q_ASSERT(false);
        return;
    }

    if (_activePlugin) {
        disconnect(
            _activePlugin, &Plugins::Display::Plugin::requestFrame,
            this, &ShadertoyApp::onFrameRequested);
        disconnect(
            _activePlugin, &Plugins::Display::Plugin::sizeChanged,
            this, &ShadertoyApp::onSizeChanged);
        _activePlugin->stop();
    }

    _activePlugin = newPlugin;
    QWindow* window = _activePlugin->window();
    _uiSurface.setProxyWindow(window);

    if (window) {
        window->installEventFilter(&_uiSurface);
        window->installEventFilter(this);
    }

    connect(
        _activePlugin, &Plugins::Display::Plugin::requestFrame,
        this, &ShadertoyApp::onFrameRequested);
    connect(
        _activePlugin, &Plugins::Display::Plugin::sizeChanged,
        this, &ShadertoyApp::onSizeChanged);
    onSizeChanged();

}

void ShadertoyApp::onSizeChanged() {
    auto uiSize = _activePlugin->preferredUiSize();
    _uiSurface.resize(QSize(uiSize.x, uiSize.y));
    auto surfaceSize = _activePlugin->preferredSurfaceSize();
    _surface.makeCurrent();
    shaderFramebuffer->init(surfaceSize);
    _surface.doneCurrent();
}

void ShadertoyApp::onUiTextureReady(GLuint texture, GLsync sync) {
    _uiSurface.lockTexture(texture);
    _uiTextureContainer.lock.lock();
    if (_uiTextureContainer.texture) {
        _uiSurface.releaseTexture(_uiTextureContainer.texture, _uiTextureContainer.readSync);
        _uiTextureContainer.readSync = 0;
        _uiTextureContainer.texture = 0;
        _uiTextureContainer.writeSync = 0;
    }
    _uiTextureContainer.texture = texture;
    _uiTextureContainer.writeSync = sync;
    _uiTextureContainer.lock.unlock();
}

void ShadertoyApp::onFrameRequested() {
    if (!_activePlugin) {
        return;
    }

    _surface.makeCurrent();
    shaderFramebuffer->Bind(Framebuffer::Target::Draw);
    Context::Clear().ColorBuffer();

    auto fboSize = shaderFramebuffer->size;
    auto baseProjection = glm::perspective<float>(HALF_TAU / 3.0f, aspect(fboSize), 0.01f, 100.0f);
    Stacks::withPush([&] {
        if (_activePlugin->type() == Type::_2D) {
            Stacks::modelview().top() = glm::inverse(_activePlugin->pose(Eye::MONO));
            Stacks::projection().top() = _activePlugin->projection(Eye::MONO, baseProjection);
            Context::Viewport(fboSize.x, fboSize.y);
            _renderer.render();
        } else {
            Stacks::modelview().top() = glm::inverse(_activePlugin->pose(Eye::LEFT));
            Stacks::projection().top() = _activePlugin->projection(Eye::LEFT, baseProjection);
            Context::Viewport(0, 0, fboSize.x / 2, fboSize.y);
            _renderer.render();
            Stacks::modelview().top() = glm::inverse(_activePlugin->pose(Eye::RIGHT));
            Stacks::projection().top() = _activePlugin->projection(Eye::RIGHT, baseProjection);
            Context::Viewport(fboSize.x / 2, 0, fboSize.x / 2, fboSize.y);
            _renderer.render();
        }
    });
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);

    // get the latest shader texture & size
    _activePlugin->preRender();
    GLuint shaderTexture = GetName(shaderFramebuffer->color);
    // get the latest UI texture & size
    if (_uiSurface.isPaused() || !_uiTextureContainer.texture) {
        _activePlugin->render(shaderTexture, fboSize, 0, uvec2());
    } else {
        _uiTextureContainer.executeReadOperation([=] {
            _activePlugin->render(shaderTexture, fboSize, _uiTextureContainer.texture, qt::toGlm(_uiSurface.size()));
        });
    }
    _activePlugin->postRender();
}

//MainWindow * riftRenderWidget = new MainWindow();
//riftRenderWidget->start();
//riftRenderWidget->requestActivate();
//riftRenderWidget->stop();
//riftRenderWidget->makeCurrent();
//delete riftRenderWidget;

//void ShadertoyApp::renderLoop() {
//    _context.makeCurrent(&_surface);
//
//    glewExperimental = true;
//    glewInit();
//    glGetError();
//
//    setup();
//
//    while (!_shutdown) {
//        if (QCoreApplication::hasPendingEvents())
//            QCoreApplication::processEvents();
//        _tasks.drainTaskQueue();
//        _context.makeCurrent(&_surface);
//        drawFrame();
//        static RateCounter rateCounter;
//        rateCounter.increment();
//        if (rateCounter.count() > 60) {
//            float fps = rateCounter.getRate();
//            //updateFps(fps);
//            rateCounter.reset();
//        }
//    }
//    _context.doneCurrent();
//    _context.moveToThread(QCoreApplication::instance()->thread());
//}

void ShadertoyApp::toggleUi() {
    bool uiVisible = _uiSurface.isPaused();
    _uiSurface.setItemProperty("shaderTextEdit", "readOnly", !uiVisible);
    if (uiVisible) {
        animation.stop();
        animation.setStartValue(0.0);
        animation.setEndValue(1.0);
        animation.setEasingCurve(QEasingCurve::OutBack);
        animation.setDuration(500);
        animation.start();
        savedEyeOffsetScale = eyeOffsetScale;
        eyeOffsetScale = 0.0f;
        _uiSurface.resume();
    } else {
        animation.stop();
        animation.setStartValue(1.0);
        animation.setEndValue(0.0);
        animation.setEasingCurve(QEasingCurve::InBack);
        animation.setDuration(500);
        animation.start();
        eyeOffsetScale = savedEyeOffsetScale;
        _uiSurface.pause();
    }
}

void ShadertoyApp::recenterPose() {
    _activePlugin->resetPose();
}

void ShadertoyApp::queueRenderThreadTask(std::function<void()> f) {
    f();
}

void ShadertoyApp::toggleEyePerFrame() {
//#ifdef USE_RIFT
//    onEpfModeChanged(!eyePerFrameMode);
//#endif
//    bool newEyePerFrameMode = checked;
//#ifdef USE_RIFT
//    queueRenderThreadTask([&, newEyePerFrameMode] {
//        eyePerFrameMode = newEyePerFrameMode;
//    });
//#endif
//    setItemProperty("epf", "checked", newEyePerFrameMode);
}

void ShadertoyApp::restartShader() {
    queueRenderThreadTask([&] {
        _renderer.restart();
    });
}


void ShadertoyApp::loadShader(const shadertoy::Shader & shader) {
    assert(!shader.fragmentSource.isEmpty());
//    _activeShader = shader;
    _codeEditor->setText(QString(shader.fragmentSource).replace(QString("\t"), QString("  ")));
    _uiSurface.setItemText("shaderName", shader.name);
    for (int i = 0; i < 4; ++i) {
        QString texturePath = _renderer.canonicalTexturePath(shader.channelTextures[i]);
        qDebug() << "Setting channel " << i << " to texture " << texturePath;
        _uiSurface.setItemProperty(QString().sprintf("channel%d", i), "source", "qrc:" + texturePath);
    }

    // FIXME update the channel texture buttons
    queueRenderThreadTask([&, shader] {
        _renderer.setShaderInternal(shader);
        _renderer.updateUniforms();
    });
}

void ShadertoyApp::loadShaderFile(const QString & file) {
    qDebug() << "Loading shader from " << file;
    loadShader(shadertoy::loadShaderFile(file));
}

static int activePresetIndex = 0;
void ShadertoyApp::loadNextPreset() {
    static const int PRESETS_SIZE = PRESETS.size();
    int newPreset = (activePresetIndex + 1) % PRESETS_SIZE;
    loadPreset(newPreset);
}

void ShadertoyApp::loadPreviousPreset() {
    static const int PRESETS_SIZE = PRESETS.size();
    int newPreset = (activePresetIndex + PRESETS_SIZE - 1) % PRESETS_SIZE;
    loadPreset(newPreset);
}

void ShadertoyApp::loadPreset(int index) {
    activePresetIndex = index;
    loadShader(shadertoy::loadShaderFile(PRESETS.at(index)));
}

void ShadertoyApp::buildShader() {
    QString shaderSource = _codeEditor->text();
    queueRenderThreadTask([=] {
        _renderer.setShaderSourceInternal(shaderSource);
        _renderer.updateUniforms();
    });
}

#if 0

struct Preset {
    const Resource res;
    const char * name;
    Preset(Resource res, const char * name) : res(res), name(name) {};
};


ShadertoyApp::ShadertoyApp() {
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

    fetcher.fetchNetworkShaders();

    connect(&timer, &QTimer::timeout, this, &ShadertoyApp::onTimer);
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

void ShadertoyApp::stop() {
    QRiftWindow::stop();
    delete uiWindow;
    uiWindow = nullptr;
}


void ShadertoyApp::onFontSizeChanged(int newSize) {
    settings.setValue("fontSize", newSize);
}


void ShadertoyApp::onNewShaderFilepath(const QString & shaderPath) {
    QDir newDir(shaderPath);
    QUrl url = QUrl::fromLocalFile(newDir.absolutePath());
    //    setItemProperty("userPresetsModel", "folder", url);
    auto qmlContext = uiWindow->m_qmlEngine->rootContext();
    qmlContext->setContextProperty("userPresetsFolder", url);
}

void ShadertoyApp::onNewShaderHighlighted(const QString & shaderPath) {
    qDebug() << "New shader highlighted " << shaderPath;
    QString previewPath = shaderPath;
    previewPath.replace(QRegularExpression("\\.(json|xml)$"), ".jpg");
    setItemProperty("previewImage", "source", QFile::exists(previewPath) ? QUrl::fromLocalFile(previewPath) : QUrl());
    if (shaderPath.endsWith(".json")) {
        setItemProperty("loadRoot", "activeShaderString", readFileToString(shaderPath));
    } else {
        setItemProperty("loadRoot", "activeShaderString", "");
    }
}

void ShadertoyApp::onNewPresetHighlighted(int presetId) {
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
    //  setItemProperty("loadRoot", "activeShaderString", readFileToString(shaderPath));
    //} else {
    //  setItemProperty("loadRoot", "activeShaderString", "");
    //}
}

void ShadertoyApp::onSaveShaderXml(const QString & shaderPath) {
    Q_ASSERT(!shaderPath.isEmpty());
    //shadertoy::saveShaderXml(activeShader);
    activeShader.name = shaderPath.toLocal8Bit();
    activeShader.fragmentSource = getItemText("shaderTextEdit").toLocal8Bit();
    QString destinationFile = configPath.absoluteFilePath(QString("shaders/")
        + shaderPath
        + ".xml");
    qDebug() << "Saving shader to " << destinationFile;
    shadertoy::saveShaderXml(destinationFile, activeShader);
}
#endif

void ShadertoyApp::onChannelTextureChanged(const int & channelIndex, const int & channelType, const QString & texturePath) {
    qDebug() << "Requesting texture from path " << texturePath;
    queueRenderThreadTask([&, channelIndex, channelType, texturePath] {
        qDebug() << texturePath;
        _renderer.setChannelTextureInternal(channelIndex, (shadertoy::ChannelInputType)channelType, texturePath);
        //activeShader.channelTypes[channelIndex] = channelIndex;
        //activeShader.channelTextures[channelIndex] = texturePath.toLocal8Bit();
        //renderer.setChannelTextureInternal(channelIndex,
        //    (shadertoy::ChannelInputType)channelType,
        //    texturePath);
        //renderer.updateUniforms();
    });
}
#if 0
void ShadertoyApp::onModifyTextureResolution(double scale) {
    float newRes = scale * texRes;
    newRes = std::max(0.1f, std::min(1.0f, newRes));
    if (newRes != texRes) {
        queueRenderThreadTask([&, newRes] {
            texRes = newRes;
        });
        setItemText("res", QString().sprintf("%0.2f", newRes));
    }
}

void ShadertoyApp::onModifyPositionScale(double scale) {
    float newPosScale = scale * eyeOffsetScale;
    queueRenderThreadTask([&, newPosScale] {
        eyeOffsetScale = newPosScale;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", newPosScale));
}

void ShadertoyApp::onResetPositionScale() {
    queueRenderThreadTask([&] {
        eyeOffsetScale = 1.0f;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", 1.0f));
}


void ShadertoyApp::onTimer() {
    TextureDeleteQueue tempTextureDeleteQueue;
    // Scope the lock tightly
    {
        Lock lock(textureLock);
        if (!textureDeleteQueue.empty()) {
            tempTextureDeleteQueue.swap(textureDeleteQueue);
        }
    }

    if (!tempTextureDeleteQueue.empty()) {
        std::for_each(tempTextureDeleteQueue.begin(), tempTextureDeleteQueue.end(), [&](int usedTexture) {
            uiWindow->releaseTexture(usedTexture);
        });
    }
}


void ShadertoyApp::updateFps(float fps) {
    emit fpsUpdated(fps);
}

///////////////////////////////////////////////////////
//
// Event handling customization
//


///////////////////////////////////////////////////////
//
// Event handling customization
// 
void ShadertoyApp::mouseMoveEvent(QMouseEvent * me) {
    // Make sure we don't show the system cursor over the window
    qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
    QRiftWindow::mouseMoveEvent(me);
}

bool ShadertoyApp::event(QEvent * e) {
#ifdef USE_RIFT
    static bool dismissedHmd = false;
    switch (e->type()) {
    case QEvent::KeyPress:
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
    }
#endif
    if (uiWindow) {
        if (uiWindow->interceptEvent(e)) {
            return true;
        }
    }

    return QRiftWindow::event(e);
}

void ShadertoyApp::resizeEvent(QResizeEvent *e) {
    uiWindow->setSourceSize(e->size());
}

///////////////////////////////////////////////////////
//
// Rendering functionality
//
void ShadertoyApp::perFrameRender() {
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
                    vec2 mp = uiWindow->getMousePosition().load();
                    mv.translate(vec3(mp, 0.0f));
                    mv.scale(vec3(0.1f));
                    mouseTexture->Bind(Texture::Target::_2D);
                    oria::renderGeometry(mouseShape, uiProgram);
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

void ShadertoyApp::perEyeRender() {
    // Render the shadertoy effect into a framebuffer, possibly at a
    // smaller resolution than recommended
    shaderFramebuffer->Bound([&] {
        Context::Clear().ColorBuffer();
        oria::viewport(renderSize());
        renderer.setResolution(renderSize());
#ifdef USE_RIFT
        renderer.setPosition(ovr::toGlm(getEyePose().Position) * eyeOffsetScale);
#endif
        renderer.render();
    });
    oria::viewport(textureSize());

    // Now re-render the shader output to the screen.
    shaderFramebuffer->BindColor(Texture::Target::_2D);
#ifdef USE_RIFT
    if (activeShader.vrEnabled) {
#endif
        // In VR mode, we want to cover the entire surface
        Stacks::withIdentity([&] {
            oria::renderGeometry(plane, planeProgram, LambdaList({ [&] {
                Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
            } }));
        });
#ifdef USE_RIFT
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

    if (animationValue > 0.0f) {
        MatrixStack & mv = Stacks::modelview();
        Texture::Active(0);
        //      oria::viewport(textureSize());
        mv.withPush([&] {
            mv.translate(vec3(0, 0, -1));
            mv.scale(vec3(1.0f, animationValue, 1.0f));
            uiFramebuffer->BindColor();
            oria::renderGeometry(uiShape, uiProgram);
        });
    }
}

void ShadertoyApp::onSixDofMotion(const vec3 & tr, const vec3 & mo) {
    SAY("%f, %f, %f", tr.x, tr.y, tr.z);
    queueRenderThreadTask([&, tr, mo] {
        // FIXME 
        // renderer.setPosition( position += tr;
    });
}
#endif
