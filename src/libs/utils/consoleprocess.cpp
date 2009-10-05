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

#include "consoleprocess.h"

namespace Utils {

QString ConsoleProcess::modeOption(Mode m)
{
    switch (m) {
        case Debug:
        return QLatin1String("debug");
        case Suspend:
        return QLatin1String("suspend");
        case Run:
        break;
    }
    return QLatin1String("run");
}

QString ConsoleProcess::msgCommChannelFailed(const QString &error)
{
    return tr("Cannot set up communication channel: %1").arg(error);
}

QString ConsoleProcess::msgPromptToClose()
{
    //! Showed in a terminal which might have
    //! a different character set on Windows.
    return tr("Press <RETURN> to close this window...");
}

QString ConsoleProcess::msgCannotCreateTempFile(const QString &why)
{
    return tr("Cannot create temporary file: %1").arg(why);
}

QString ConsoleProcess::msgCannotCreateTempDir(const QString & dir, const QString &why)
{
    return tr("Cannot create temporary directory '%1': %2").arg(dir, why);
}

QString ConsoleProcess::msgUnexpectedOutput()
{
    return tr("Unexpected output from helper program.");
}

QString ConsoleProcess::msgCannotChangeToWorkDir(const QString & dir, const QString &why)
{
    return tr("Cannot change to working directory '%1': %2").arg(dir, why);
}

QString ConsoleProcess::msgCannotExecute(const QString & p, const QString &why)
{
    return tr("Cannot execute '%1': %2").arg(p, why);
}

}
