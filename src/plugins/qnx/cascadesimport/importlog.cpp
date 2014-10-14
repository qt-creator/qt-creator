/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry Limited (blackberry-qt@qnx.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "importlog.h"

namespace Qnx {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// ImportLogEntry
//
//////////////////////////////////////////////////////////////////////////////
static const char S_ERROR[] = "error";
static const char S_WARNING[] = "warning";
static const char S_INFO[] = "info";
static const char S_DEBUG[] = "debug";
static const char S_UNDEFINED[] = "undefined";

ImportLogEntry::ImportLogEntry(int sev, const QString &msg, const QString &context)
: QVariantList()
{
    reserve(UnusedIx);
    append(sev);
    append(msg);
    append(context);
}

static const char* severityEnumToStr(ImportLogEntry::Severity sev)
{
    switch (sev) {
        case ImportLogEntry::Error:
            return S_ERROR;
        case ImportLogEntry::Warning:
            return S_WARNING;
        case ImportLogEntry::Info:
            return S_INFO;
        case ImportLogEntry::Debug:
            return S_DEBUG;
        default:
            break;
    }
    return S_UNDEFINED;
}

ImportLogEntry::Severity ImportLogEntry::severity() const
{
    if (isValid())
        return (Severity)(at(SeverityIx).toInt());
    return Undefined;
}

QString ImportLogEntry::severityStr() const
{
    return QLatin1String(severityEnumToStr(severity()));
}

QChar ImportLogEntry::severityAbbr() const
{
    return severityStr().at(0).toUpper();
}

QString ImportLogEntry::message() const
{
    if (isValid())
        return at(MessageIx).toString();
    return QString();
}

QString ImportLogEntry::context() const
{
    if (isValid())
        return at(ContextIx).toString();
    return QString();
}

QString ImportLogEntry::toString() const
{
    QString ret;
    if (severity() == Undefined)
        ret = message();
    else
        ret = QString::fromLatin1("[%1]%2 %3").arg(severityAbbr()).arg(context()).arg(message());
    return ret;
}

//////////////////////////////////////////////////////////////////////////////
//
// ImportLog
//
//////////////////////////////////////////////////////////////////////////////
QString ImportLog::toString() const
{
    QString ret;
    foreach (const ImportLogEntry &sle, *this)
        ret += sle.toString() + QLatin1Char('\n');
    return ret;
}

ImportLog& ImportLog::logError(const QString &msg, const QString &context)
{
    append(ImportLogEntry(ImportLogEntry::Error, msg, context));
    return *this;
}

ImportLog& ImportLog::logWarning(const QString &msg, const QString &context)
{
    append(ImportLogEntry(ImportLogEntry::Warning, msg, context));
    return *this;
}

ImportLog& ImportLog::logInfo(const QString &msg, const QString &context)
{
    append(ImportLogEntry(ImportLogEntry::Info, msg, context));
    return *this;
}

ImportLog& ImportLog::logDebug(const QString &msg, const QString &context)
{
    append(ImportLogEntry(ImportLogEntry::Debug, msg, context));
    return *this;
}

} // namespace Internal
} // namespace Qnx

