//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QtCommon.h"

OffscreenGlSurface::OffscreenGlSurface() {
}

OffscreenGlSurface::~OffscreenGlSurface() {
}

void OffscreenGlSurface::create(QOpenGLContext* sharedContext) {
    if (nullptr != sharedContext) {
        _context.setShareContext(sharedContext);
    } 
    _context.setFormat(getDesiredSurfaceFormat());
    _context.create();

    QOffscreenSurface::setFormat(_context.format());
    QOffscreenSurface::create();
}

bool OffscreenGlSurface::makeCurrent() {
    return _context.makeCurrent(this);
}

void OffscreenGlSurface::doneCurrent() {
    _context.doneCurrent();
}

QOpenGLContext* OffscreenGlSurface::context() {
    return &_context;
}

