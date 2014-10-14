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

#ifndef QNX_BLACKBERRY_CASCADESPROJECTIMPORT_IMPORTLOG_H
#define QNX_BLACKBERRY_CASCADESPROJECTIMPORT_IMPORTLOG_H

#include <QVariantList>

namespace Qnx {
namespace Internal {

class ImportLogEntry : public QVariantList
{
public:
    enum Severity {Undefined = 0, Debug, Info, Warning, Error};

    ImportLogEntry() : QVariantList() {}
    ImportLogEntry(int sev, const QString &msg, const QString &context = QString());
    ImportLogEntry(const QVariantList &l) : QVariantList(l) {}

    bool isValid() const {return (count() >= UnusedIx);}
    Severity severity() const;
    QString severityStr() const;
    QChar severityAbbr() const;
    QString message() const;
    QString context() const;
    QString toString() const;
private:
    enum FieldIndexes {SeverityIx = 0, MessageIx, ContextIx, UnusedIx};
};

class ImportLog : public QList<ImportLogEntry>  ///< error list
{
public:
    QString toString() const;

    ImportLog& logError(const QString &msg, const QString &context = QString());
    ImportLog& logWarning(const QString &msg, const QString &context = QString());
    ImportLog& logInfo(const QString &msg, const QString &context = QString());
    ImportLog& logDebug(const QString &msg, const QString &context = QString());
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_BLACKBERRY_CASCADESPROJECTIMPORT_IMPORTLOG_H
