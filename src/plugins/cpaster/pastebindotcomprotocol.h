/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PASTEBINDOTCOMPROTOCOL_H
#define PASTEBINDOTCOMPROTOCOL_H

#include "protocol.h"

namespace CodePaster {
class PasteBinDotComSettings;

class PasteBinDotComProtocol : public NetworkProtocol
{
    Q_OBJECT
public:
    explicit PasteBinDotComProtocol(const NetworkAccessManagerProxyPtr &nw);

    QString name() const { return QLatin1String("Pastebin.Com"); }

    virtual unsigned capabilities() const;
    bool hasSettings() const { return true; }
    Core::IOptionsPage *settingsPage();

    virtual void fetch(const QString &id);
    virtual void paste(const QString &text,
                       ContentType ct = Text,
                       const QString &username = QString(),
                       const QString &comment = QString(),
                       const QString &description = QString());
    virtual void list();

public slots:
    void fetchFinished();
    void pasteFinished();
    void listFinished();

private:
    QString hostName(bool withSubDomain) const;

    PasteBinDotComSettings *m_settings;
    QNetworkReply *m_fetchReply;
    QNetworkReply *m_pasteReply;
    QNetworkReply *m_listReply;

    QString m_fetchId;
    int m_postId;
};
} // namespace CodePaster
#endif // PASTEBINDOTCOMPROTOCOL_H
