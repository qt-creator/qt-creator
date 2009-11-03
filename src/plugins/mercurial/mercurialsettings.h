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

namespace Mercurial {
namespace Internal {

class MercurialSettings
{
public:
    MercurialSettings();

    QString binary() const;
    QString application() const;
    QStringList standardArguments() const;
    QString userName() const;
    QString email() const;
    int logCount() const;
    int timeout() const;
    int timeoutSeconds() const;
    bool prompt() const;
    void writeSettings(const QString &application, const QString &userName,
                       const QString &email, int logCount, int timeout, bool prompt);
private:

    void readSettings();
    void setBinAndArgs();

    QString bin; // used because windows requires cmd.exe to run the mercurial binary
                 // in this case the actual mercurial binary will be part of the standard args
    QString app; // this is teh actual mercurial executable
    QStringList standardArgs;
    QString user;
    QString mail;
    int m_logCount;
    int m_timeout;
    bool m_prompt;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALSETTINGS_H
