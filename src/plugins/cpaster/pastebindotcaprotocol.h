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
class PasteBinDotCaProtocol : public NetworkProtocol
{
    Q_OBJECT
public:
    static QString protocolName();
    QString name() const override { return protocolName(); }

    bool hasSettings() const override { return false; }
    unsigned capabilities() const override;

    void fetch(const QString &id) override;
    void paste(const QString &text,
               ContentType ct = Text,
               int expiryDays = 1,
               const QString &username = QString(),
               const QString &comment = QString(),
               const QString &description = QString()) override;
    void list() override;

protected:
    bool checkConfiguration(QString *errorMessage) override;

private:
    void fetchFinished();
    void listFinished();
    void pasteFinished();

    QNetworkReply *m_fetchReply = nullptr;
    QNetworkReply *m_listReply = nullptr;
    QNetworkReply *m_pasteReply = nullptr;
    QString m_fetchId;
    bool m_hostChecked = false;
};

} // namespace CodePaster
