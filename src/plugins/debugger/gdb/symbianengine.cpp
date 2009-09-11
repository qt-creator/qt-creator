/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#define QT_NO_CAST_FROM_ASCII

#include "gdb/gdbengine.h"
#include "symbianadapter.h"
#include "debuggermanager.h"

//#include "debuggerdialogs.h"

#include <utils/qtcassert.h>
#include <texteditor/itexteditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QDebug>


namespace Debugger {
namespace Internal {

IDebuggerEngine *createSymbianEngine(DebuggerManager *parent,
    QList<Core::IOptionsPage*> *opts)
{
    Q_UNUSED(opts);
    //opts->push_back(new GdbOptionsPage);
    SymbianAdapter *adapter = new SymbianAdapter;
    GdbEngine *engine = new GdbEngine(parent, adapter);
    QObject::connect(adapter, SIGNAL(output(QString)),
        parent, SLOT(showDebuggerOutput(QString)));
    return engine;
}

} // namespace Internal
} // namespace Debugger


