// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runscenegraphtest.h"

#include <QTest>
#include <QString>
#include <QOpenGLContext>
#include <QOffscreenSurface>

namespace Timeline {

void runSceneGraphTest(QSGNode *node)
{
    Q_UNUSED(node)

    QSurfaceFormat format;
    format.setStencilBufferSize(8);
    format.setDepthBufferSize(24);

    QOpenGLContext context;
    context.setFormat(format);
    QVERIFY(context.create());

    QOffscreenSurface surface;
    surface.setFormat(format);
    surface.create();

    QVERIFY(context.makeCurrent(&surface));

    context.doneCurrent();
}

} // namespace Timeline
