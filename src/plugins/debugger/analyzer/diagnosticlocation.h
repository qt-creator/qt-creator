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

#include <debugger/debugger_global.h>

#include <QDebug>
#include <QMetaType>
#include <QString>

namespace Debugger {

class DEBUGGER_EXPORT DiagnosticLocation
{
public:
    DiagnosticLocation();
    DiagnosticLocation(const QString &filePath, int line, int column);

    bool isValid() const;

    QString filePath;

    // Both values start at 1.
    int line;
    int column;
};

DEBUGGER_EXPORT bool operator==(const DiagnosticLocation &first, const DiagnosticLocation &second);
DEBUGGER_EXPORT QDebug operator<<(QDebug dbg, const DiagnosticLocation &location);

} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::DiagnosticLocation)
