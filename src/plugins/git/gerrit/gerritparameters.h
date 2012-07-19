/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
