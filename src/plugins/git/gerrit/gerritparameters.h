/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GERRITPARAMETERS_H
#define GERRITPARAMETERS_H

#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace Gerrit {
namespace Internal {

class GerritParameters
{
public:
    GerritParameters();

    QStringList baseCommandArguments() const;
    QString sshHostArgument() const;
    bool isValid() const;
    bool equals(const GerritParameters &rhs) const;
    void toSettings(QSettings *) const;
    void saveQueries(QSettings *) const;
    void fromSettings(const QSettings *);
    void setPortFlagBySshType();

    QString host;
    unsigned short port;
    QString user;
    QString ssh;
    QStringList savedQueries;
    bool https;
    QString portFlag;
};

inline bool operator==(const GerritParameters &p1, const GerritParameters &p2)
{ return p1.equals(p2); }
inline bool operator!=(const GerritParameters &p1, const GerritParameters &p2)
{ return !p1.equals(p2); }

} // namespace Internal
} // namespace Gerrit

#endif // GERRITPARAMETERS_H
