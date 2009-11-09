/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIALSETTINGS_H
#define MERCURIALSETTINGS_H

#include <QtCore/QString>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Mercurial {
namespace Internal {

class MercurialSettings
{
public:
    MercurialSettings();

    QString binary() const;
    void setBinary(const QString &);

    // Calculated.
    QStringList standardArguments() const;

    QString userName() const;
    void setUserName(const QString &);

    QString email() const;
    void setEmail(const QString &);

    int logCount() const;
    void setLogCount(int l);

    int timeoutMilliSeconds() const;
    int timeoutSeconds() const;
    void setTimeoutSeconds(int s);

    bool prompt() const;
    void setPrompt(bool b);

    void writeSettings(QSettings *settings) const;
    void readSettings(const QSettings *settings);

    bool equals(const MercurialSettings &rhs) const;

private:

    void readSettings();

    QString m_binary;
    QStringList m_standardArguments;
    QString m_user;
    QString m_mail;
    int m_logCount;
    int m_timeoutSeconds;
    bool m_prompt;
};

inline bool operator==(const MercurialSettings &s1, const MercurialSettings &s2)
{ return s1.equals(s2); }
inline bool operator!=(const MercurialSettings &s1, const MercurialSettings &s2)
{ return !s1.equals(s2); }

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALSETTINGS_H
