//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QOffscreenSurface>

class OffscreenGlSurface : public QOffscreenSurface {
public:
    OffscreenGlSurface();
    virtual ~OffscreenGlSurface();
    void create(QOpenGLContext* sharedContext = nullptr);
    bool makeCurrent();
    void doneCurrent();
    QOpenGLContext* context();
protected:
    QOpenGLContext _context;
};
