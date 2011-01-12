/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

    static QString protocolName();
    QString name() const { return protocolName(); }

    virtual unsigned capabilities() const;
    bool hasSettings() const { return true; }
    Core::IOptionsPage *settingsPage() const;

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

protected:
    virtual bool checkConfiguration(QString *errorMessage = 0);

private:
    QString hostName(bool withSubDomain) const;

    PasteBinDotComSettings *m_settings;
    QNetworkReply *m_fetchReply;
    QNetworkReply *m_pasteReply;
    QNetworkReply *m_listReply;

    QString m_fetchId;
    int m_postId;
    bool m_hostChecked;
};
} // namespace CodePaster
#endif // PASTEBINDOTCOMPROTOCOL_H
