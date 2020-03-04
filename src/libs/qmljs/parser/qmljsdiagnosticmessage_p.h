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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qlogging.h>
#include <QtCore/qstring.h>
#include "qmljsglobal_p.h"
#include "qmljs/qmljsconstants.h"

// Include the API version here, to avoid complications when querying it for the
// QmlSourceLocation -> line/column change.


#include "qmljssourcelocation_p.h"

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {
class DiagnosticMessage
{
public:
    QString message;
    Severity::Enum kind = Severity::Error;
    SourceLocation loc;
    static Severity::Enum qtMsgTypeToKind(QtMsgType msgType) {
        switch (msgType) {
        case QtDebugMsg:
            return Severity::Hint;
        case QtWarningMsg:
            return Severity::Warning;
        case QtCriticalMsg:
            return Severity::Error;
        case QtFatalMsg:
            return Severity::Error;
        case QtInfoMsg:
        default:
            return Severity::MaybeWarning;
        }
    }
    DiagnosticMessage() {}
    DiagnosticMessage(Severity::Enum kind, SourceLocation loc, QString message):
        message(message), kind(kind), loc(loc)
    { }

    bool isError() const
    {
        return kind == Severity::Error;
    }

    bool isWarning() const
    {
        return kind == Severity::Warning;
    }

    bool isValid() const
    {
        return !message.isEmpty();
    }
};
} // namespace QmlJS

QT_QML_END_NAMESPACE

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlJS::DiagnosticMessage, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
