/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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

protected:
    bool checkConfiguration(QString *errorMessage = 0) override;

private:
    void fetchFinished();
    void pasteFinished();
    void listFinished();

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

    QString name() const override { return protocolName(); }
    static QString protocolName();
};

} // namespace CodePaster
