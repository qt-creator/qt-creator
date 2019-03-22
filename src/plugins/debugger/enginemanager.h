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

#pragma once

#include <utils/treemodel.h>

#include <QPointer>

namespace Debugger {
namespace Internal {

class DebuggerEngine;

class EngineManager : public QObject
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

} // namespace Internal
} // namespace Debugger
