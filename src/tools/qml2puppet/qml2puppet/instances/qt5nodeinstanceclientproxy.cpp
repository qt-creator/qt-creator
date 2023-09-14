// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt5nodeinstanceclientproxy.h"

#include <QCoreApplication>

#include "capturenodeinstanceserverdispatcher.h"
#include "qt5bakelightsnodeinstanceserver.h"
#include "qt5captureimagenodeinstanceserver.h"
#include "qt5capturepreviewnodeinstanceserver.h"
#include "qt5informationnodeinstanceserver.h"
#include "qt5previewnodeinstanceserver.h"
#include "qt5rendernodeinstanceserver.h"
#include "qt5testnodeinstanceserver.h"
#include "quickitemnodeinstance.h"

#if defined(Q_OS_UNIX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace QmlDesigner {
static void prioritizeDown()
{
#if defined(Q_OS_UNIX)
    nice(19);
#elif defined(Q_OS_WIN)
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#endif
}

Qt5NodeInstanceClientProxy::Qt5NodeInstanceClientProxy(QObject *parent) :
    NodeInstanceClientProxy(parent)
{
    prioritizeDown();

    const bool unifiedRenderPath = qEnvironmentVariableIsSet("QMLPUPPET_UNIFIED_RENDER_PATH");

    if (unifiedRenderPath)
        Internal::QuickItemNodeInstance::enableUnifiedRenderPath(true);

    if (QCoreApplication::arguments().at(1) == QLatin1String("--readcapturedstream")) {
        qputenv("DESIGNER_DONT_USE_SHARED_MEMORY", "1");
        setNodeInstanceServer(std::make_unique<Qt5TestNodeInstanceServer>(this));
        initializeCapturedStream(QCoreApplication::arguments().at(2));
        readDataStream();
        QCoreApplication::exit();
    } else if (QCoreApplication::arguments().at(2).contains(',')) {
        const QStringList serverNames = QCoreApplication::arguments().at(2).split(',');
        setNodeInstanceServer(std::make_unique<CaptureNodeInstanceServerDispatcher>(serverNames, this));
        initializeSocket();
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("previewmode")) {
        setNodeInstanceServer(std::make_unique<Qt5PreviewNodeInstanceServer>(this));
        initializeSocket();
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("editormode")) {
        ViewConfig::enableParticleView(true);
        setNodeInstanceServer(std::make_unique<Qt5InformationNodeInstanceServer>(this));
        initializeSocket();
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("rendermode")) {
        setNodeInstanceServer(std::make_unique<Qt5RenderNodeInstanceServer>(this));
        initializeSocket();
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("capturemode")) {
        setNodeInstanceServer(std::make_unique<Qt5CapturePreviewNodeInstanceServer>(this));
        initializeSocket();
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("captureiconmode")) {
        setNodeInstanceServer(std::make_unique<Qt5CaptureImageNodeInstanceServer>(this));
        initializeSocket();
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("bakelightsmode")) {
        setNodeInstanceServer(std::make_unique<Qt5BakeLightsNodeInstanceServer>(this));
        initializeSocket();
    }
}

} // namespace QmlDesigner
