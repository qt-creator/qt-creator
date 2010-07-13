/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_SESSIONENGINE_H
#define DEBUGGER_SESSIONENGINE_H

#include "debuggerengine.h"

namespace Debugger {
namespace Internal {

class SnapshotHandler;

// This class contains data serving as a template for debugger engines
// started during a session.

class SessionEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    SessionEngine();

    void setupEngine() {}
    void setupInferior() {}
    void runEngine() {}
    void shutdownEngine() {}
    void shutdownInferior() {}

    bool isSessionEngine() const { return true; }

    void loadSessionData();
    void saveSessionData();

public:
    // The global set of snapshots.
    SnapshotHandler *m_snapshotHandler;
    // The model serving as command dispatcher.
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SESSIONENGINE_H
