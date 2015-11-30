/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "runscenegraph.h"

#include <QTest>
#include <QString>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QSGEngine>
#include <QSGAbstractRenderer>

void renderMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (type > QtDebugMsg)
        QTest::qFail(message.toLatin1().constData(), context.file, context.line);
    else
        QTest::qWarn(message.toLatin1().constData(), context.file, context.line);
}

void runSceneGraph(QSGNode *node)
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
