//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickRenderControl>
#include <QtQuick/QQuickImageProvider>

#include "OffscreenGlSurface.h"
#include "FboCache.h"

class OffscreenQmlSurface : public OffscreenGlSurface {
    Q_OBJECT
protected:
    class QMyQuickRenderControl : public QQuickRenderControl {
    protected:
        QWindow* renderWindow(QPoint* offset) Q_DECL_OVERRIDE{
            if (nullptr == _renderWindow) {
                return QQuickRenderControl::renderWindow(offset);
            }
            if (nullptr != offset) {
                offset->rx() = offset->ry() = 0;
            }
            return _renderWindow;
        }

    private:
        QWindow* _renderWindow{ nullptr };
        friend class OffscreenQmlSurface;
    };
public:
    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPointF(const QPointF&)>;

    void create(QOpenGLContext* context);
    void resize(const QSize& size);
    QSize size();
    QObject* load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    QObject* load(const QString& qmlSourceFile, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}) {
        return load(QUrl(qmlSourceFile), f);
    }

    QVariant getItemProperty(const QString & itemName, const QString & property);
    void setItemProperty(const QString & itemName, const QString & property, const QVariant & value);
    void setItemText(const QString & itemName, const QString & text);

    // Optional values for event handling
    void setProxyWindow(QWindow* window);
    void setMouseTranslator(MouseTranslator mouseTranslator) {
        _mouseTranslator = mouseTranslator;
    }

    void pause();
    void resume();
    bool isPaused() const;

    void setBaseUrl(const QUrl& baseUrl);
    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QQmlEngine* getEngine() { return _qmlEngine;  }

    virtual bool eventFilter(QObject* originalDestination, QEvent* event);

signals:
    void textureUpdated(GLuint texture);

public slots:
    void requestUpdate();
    void requestRender();
    void lockTexture(int texture);
    void releaseTexture(int texture, GLsync readSync = 0);

private:
    QObject* finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);

private slots:
    void updateQuick();

protected:
    QQuickWindow* _quickWindow{ nullptr };

private:
    QMyQuickRenderControl* _renderControl{ new QMyQuickRenderControl };
    QQmlEngine* _qmlEngine{ nullptr };
    QQmlComponent* _qmlComponent{ nullptr };
    QQuickItem* _rootItem{ nullptr };
    QTimer _updateTimer;
    QSize _size;
    FboCache _fboCache;
    bool _polish{ true };
    bool _paused{ true };
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p;  } };
};
