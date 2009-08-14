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

#ifndef PERFOCESETTINGS_H
#define PERFOCESETTINGS_H

#include <QtCore/QString>
#include <QtCore/QFuture>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Perforce {
namespace Internal {

struct Settings {
    Settings();
    bool equals(const Settings &s) const;
    QStringList basicP4Args() const;

    bool check(QString *errorMessage) const;
    static bool doCheck(const QString &binary, const QStringList &basicArgs, QString *errorMessage);

    QString p4Command;
    QString p4Port;
    QString p4Client;
    QString p4User;
    QString errorString;
    bool defaultEnv;
    bool promptToSubmit;
};

inline bool operator==(const Settings &s1, const Settings &s2) { return s1.equals(s2); }
inline bool operator!=(const Settings &s1, const Settings &s2) { return !s1.equals(s2); }

// PerforceSettings: Aggregates settings struct and contains a sophisticated
// background check invoked on setSettings() to figure out whether the p4
// configuration is actually valid (disabling it when invalid to save time
// when updating actions. etc.)

class PerforceSettings {
public:
    PerforceSettings();
    ~PerforceSettings();
    void fromSettings(QSettings *settings);
    void toSettings(QSettings *) const;

    void setSettings(const Settings &s);
    Settings settings() const;

    bool isValid() const;

    QString p4Command() const;
    QString p4Port() const;
    QString p4Client() const;
    QString p4User() const;
    bool defaultEnv() const;
    bool promptToSubmit() const;
    void setPromptToSubmit(bool p);

    QStringList basicP4Args() const;

    // Error code of last check
    QString errorString() const;

private:
    void run(QFutureInterface<void> &fi);

    mutable QFuture<void> m_future;
    mutable QMutex m_mutex;

    Settings m_settings;
    QString m_errorString;
    bool m_valid;
    Q_DISABLE_COPY(PerforceSettings);
};

} // Internal
} // Perforce

#endif // PERFOCESETTINGS_H
