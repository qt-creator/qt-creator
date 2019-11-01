/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbmessagetracker.h"
#include "qdbwatcher.h"

#include "../qdbutils.h"
#include "hostmessages.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Qdb {
namespace Internal {

QdbMessageTracker::QdbMessageTracker(QObject *parent)
    : QObject(parent)
{
    m_qdbWatcher = new QdbWatcher(this);
    connect(m_qdbWatcher, &QdbWatcher::incomingMessage, this, &QdbMessageTracker::handleWatchMessage);
    // Errors are not handled because showing the messages is just a helpful addition for the users
    // but not critical.
}

void QdbMessageTracker::start()
{
    m_qdbWatcher->start(RequestType::WatchMessages);
}

void QdbMessageTracker::stop()
{
    m_qdbWatcher->stop();
}

void QdbMessageTracker::handleWatchMessage(const QJsonDocument &document)
{
    const auto type = responseType(document.object());
    if (type != ResponseType::Messages) {
        stop();
        const QString message =
                tr("Shutting down message reception due to unexpected response: %1");
        emit trackerError(message.arg(QString::fromUtf8(document.toJson())));
        return;
    }

    for (const auto &i : document.object().value(QLatin1String("messages")).toArray()) {
        // There is an additional interger field "type" available allowing future
        // filtering and handling messages regarding of their type.
        const auto message = i.toObject().value(QLatin1String("text")).toString();

        // In case the issues reported from qdb server are permanent the messages
        // will reappear every second annoying the user. A cache will check
        // if the message was already displayed.
        for (auto i = m_messageCache.firstIndex(); i < m_messageCache.lastIndex(); ++i) {
            if (m_messageCache.at(i) == message)
                return;
        }

        m_messageCache.append(message);
        showMessage(tr("QDB message: %1").arg(message), true);
    }
}

} // namespace Internal
} // namespace Qdb
