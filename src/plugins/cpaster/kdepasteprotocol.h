/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KDEPASTE_H
#define KDEPASTE_H

#include "protocol.h"

namespace CodePaster {

class StickyNotesPasteProtocol : public NetworkProtocol
{
    Q_OBJECT
public:
    unsigned capabilities() const override;

    void fetch(const QString &id) override;
    void paste(const QString &text,
               ContentType ct = Text,
               int expiryDays = 1,
               const QString &username = QString(),
               const QString &comment = QString(),
               const QString &description = QString()) override;
    void list() override;



    QString hostUrl() const { return m_hostUrl; }
    void setHostUrl(const QString &hostUrl);

public slots:
    void fetchFinished();
    void pasteFinished();
    void listFinished();

protected:
    bool checkConfiguration(QString *errorMessage = 0) override;

private:
    QString m_hostUrl;

    QNetworkReply *m_fetchReply = nullptr;
    QNetworkReply *m_pasteReply = nullptr;
    QNetworkReply *m_listReply = nullptr;

    QString m_fetchId;
    int m_postId = -1;
    bool m_hostChecked = false;
};

class KdePasteProtocol : public StickyNotesPasteProtocol
{
public:
    KdePasteProtocol()
    {
        setHostUrl(QLatin1String("https://pastebin.kde.org/"));
    }

    QString name() const { return protocolName(); }
    static QString protocolName();
};

} // namespace CodePaster

#endif // KDEPASTE_H
