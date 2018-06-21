/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "runscenegraphtest.h"

#include <QTest>
#include <QString>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QSGEngine>
#include <QSGAbstractRenderer>

namespace Timeline {

static void renderMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                 const QString &message)
{
    if (type > QtDebugMsg)
        QTest::qFail(message.toLatin1().constData(), context.file, context.line);
    else
        QTest::qWarn(message.toLatin1().constData(), context.file, context.line);
}

void runSceneGraphTest(QSGNode *node)
{
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

    QSGEngine engine;
    QSGRootNode root;
    root.appendChildNode(node);
    engine.initialize(&context);

    QSGAbstractRenderer *renderer = engine.createRenderer();
    QVERIFY(renderer != 0);
    renderer->setRootNode(&root);
    QtMessageHandler originalHandler = qInstallMessageHandler(renderMessageHandler);
    renderer->renderScene();
    qInstallMessageHandler(originalHandler);
    delete renderer;

    // Unfortunately we cannot check the results of the rendering. But at least we know the shaders
    // have not crashed here.

    context.doneCurrent();
}

} // namespace Timeline
