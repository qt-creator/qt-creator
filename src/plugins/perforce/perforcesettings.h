/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
