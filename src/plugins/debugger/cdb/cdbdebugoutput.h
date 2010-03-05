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

#ifndef DEBUGGER_CDBOUTPUT_H
#define DEBUGGER_CDBOUTPUT_H

#include "debugoutputbase.h"

#include <QtCore/QObject>

namespace Debugger {
namespace Internal {

// Standard CDB output handler
class CdbDebugOutput : public QObject, public CdbCore::DebugOutputBase
{
    Q_OBJECT
public:
    CdbDebugOutput();

protected:
    virtual void output(ULONG mask, const QString &message);

signals:
    void debuggerOutput(int channel, const QString &message);
    void debuggerInputPrompt(int channel, const QString &message);
    void debuggeeOutput(const QString &message);
    void debuggeeInputPrompt(const QString &message);
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBOUTPUT_H
