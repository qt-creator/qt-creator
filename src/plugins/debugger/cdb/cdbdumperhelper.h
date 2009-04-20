/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CDBDUMPERHELPER_H
#define CDBDUMPERHELPER_H

#include <QtCore/QString>

namespace Debugger {
namespace Internal {

struct CdbComInterfaces;

// For code clarity, all the stuff related to custom dumpers
// goes here.
// "Custom dumper" is a library compiled against the current
// Qt containing functions to evaluate values of Qt classes
// (such as QString, taking pointers to their addresses).
// The library must be loaded into the debuggee.

class CdbDumperHelper
{
public:
   enum State {
        Disabled,
        NotLoaded,
        Loading,
        Loaded,
        Failed
    };

    explicit CdbDumperHelper(CdbComInterfaces *cif);

    // Call before starting the debugger
    void reset(const QString &library, bool enabled);

    // Call from the module loaded event handler.
    // It will load the dumper library and resolve the required symbols
    // when appropriate.
    bool moduleLoadHook(const QString &name, bool *ignoreNextBreakPoint);

    State state() const { return m_state; }

    QString errorMessage() const { return m_errorMessage; }
    QString library() const { return m_library; }

private:
    bool resolveSymbols(QString *errorMessage);

    State m_state;
    CdbComInterfaces *m_cif;

    QString m_library;
    QString m_dumpObjectSymbol;
    QString m_errorMessage;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBDUMPERHELPER_H
