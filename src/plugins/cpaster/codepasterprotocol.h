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

#ifndef CODEPASTERPROTOCOL_H
#define CODEPASTERPROTOCOL_H

#include "protocol.h"

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

namespace CodePaster {

class CodePasterSettingsPage;

class CodePasterProtocol : public NetworkProtocol
{
    Q_OBJECT
public:
    explicit CodePasterProtocol(const NetworkAccessManagerProxyPtr &nw);
    ~CodePasterProtocol();

    QString name() const;

    virtual unsigned capabilities() const;
    bool hasSettings() const;
    Core::IOptionsPage *settingsPage() const;

    virtual bool checkConfiguration(QString *errorMessage = 0);
    void fetch(const QString &id);
    void list();
    void paste(const QString &text,
               ContentType ct = Text,
               const QString &username = QString(),
               const QString &comment = QString(),
               const QString &description = QString());
public slots:
    void fetchFinished();
    void listFinished();
    void pasteFinished();

private:
    CodePasterSettingsPage *m_page;
    QNetworkReply *m_pasteReply;
    QNetworkReply *m_fetchReply;
    QNetworkReply *m_listReply;
    QString m_fetchId;
    QString m_hostChecked;
};

} // namespace CodePaster

#endif // CODEPASTERPROTOCOL_H
