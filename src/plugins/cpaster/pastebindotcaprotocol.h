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

#ifndef PASTEBINDOTCAPROTOCOL_H
#define PASTEBINDOTCAPROTOCOL_H

#include "protocol.h"

#include <QtNetwork/QHttp>

namespace CodePaster {
class PasteBinDotCaProtocol : public NetworkProtocol
{
    Q_OBJECT
public:
    explicit PasteBinDotCaProtocol(const NetworkAccessManagerProxyPtr &nw);
    QString name() const { return QLatin1String("Pastebin.Ca"); }

    bool hasSettings() const { return false; }
    virtual unsigned capabilities() const { return 0; }

    void fetch(const QString &id);
    void paste(const QString &text,
               ContentType ct = Text,
               const QString &username = QString(),
               const QString &comment = QString(),
               const QString &description = QString());
public slots:
    void fetchFinished();

    void postRequestFinished(int id, bool error);

private:
    QNetworkReply *m_fetchReply;
    QString m_fetchId;

    QHttp m_http;
    int m_postId;
};

} // namespace CodePaster
#endif // PASTEBINDOTCAPROTOCOL_H
