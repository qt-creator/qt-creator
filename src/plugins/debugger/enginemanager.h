// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/treemodel.h>

#include <QPointer>

namespace Debugger::Internal {

class DebuggerEngine;
struct PerspectiveItem;

class EngineManager final : public QObject
{
    Q_OBJECT

public:
    explicit EngineManager();
    ~EngineManager() final;

    static EngineManager *instance();
    static QAbstractItemModel *model();
    static QAbstractItemModel *dapModel();

    static QString registerEngine(DebuggerEngine *engine);
    static void unregisterEngine(DebuggerEngine *engine);

    static QString registerDefaultPerspective(const QString &name,
                                              const QString &type,
                                              const QString &id);

    static void activateDebugMode();
    static void deactivateDebugMode();
    static void activateByIndex(int index);

    static QList<QPointer<DebuggerEngine> > engines();
    static QPointer<DebuggerEngine> currentEngine();

    static QWidget *engineChooser();
    static QWidget *dapEngineChooser();

    static void updatePerspectives();

    static bool shutDown(); // Return true if some engine is being forced to shut down.

signals:
    void engineStateChanged(DebuggerEngine *engine);
    void currentEngineChanged();
};

} // Debugger::Internal
