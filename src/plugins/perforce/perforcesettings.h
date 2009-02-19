/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef PERFOCESETTINGS_H
#define PERFOCESETTINGS_H

#include <QtCore/QString>
#include <QtCore/QFuture>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Perforce {
namespace Internal {

class PerforceSettings {
public:
    PerforceSettings();
    ~PerforceSettings();
    void fromSettings(QSettings *settings);
    void toSettings(QSettings *) const;
    void setSettings(const QString &p4Command, const QString &p4Port, const QString &p4Client, const QString p4User, bool defaultEnv);
    bool isValid() const;

    QString p4Command() const;
    QString p4Port() const;
    QString p4Client() const;
    QString p4User() const;
    bool defaultEnv() const;
    QStringList basicP4Args() const;
private:
    void run(QFutureInterface<void> &fi);
    mutable QFuture<void> m_future;
    mutable QMutex m_mutex;

    QString m_p4Command;
    QString m_p4Port;
    QString m_p4Client;
    QString m_p4User;
    bool m_defaultEnv;
    bool m_valid;
    Q_DISABLE_COPY(PerforceSettings);
};

} // Internal
} // Perforce

#endif // PERFOCESETTINGS_H
