// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/treemodel.h>

#include <QPointer>

namespace Debugger::Internal {

class DebuggerEngine;

class EngineManager final : public QObject
{
    Q_OBJECT

public:
    explicit EngineManager();
    ~EngineManager() final;

    static EngineManager *instance();
    static QAbstractItemModel *model();

    static QString registerEngine(DebuggerEngine *engine);
    static void unregisterEngine(DebuggerEngine *engine);

    static void activateDebugMode();
    static void deactivateDebugMode();

    static QList<QPointer<DebuggerEngine> > engines();
    static QPointer<DebuggerEngine> currentEngine();

    static QWidget *engineChooser();
    static void updatePerspectives();

    static bool shutDown(); // Return true if some engine is being forced to shut down.

signals:
    void engineStateChanged(DebuggerEngine *engine);
    void currentEngineChanged();
};

} // Debugger::Internal
