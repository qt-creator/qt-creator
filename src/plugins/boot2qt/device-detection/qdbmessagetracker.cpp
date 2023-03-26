// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbmessagetracker.h"
#include "qdbwatcher.h"

#include "../qdbtr.h"
#include "../qdbutils.h"
#include "hostmessages.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Qdb::Internal {

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
                Tr::tr("Shutting down message reception due to unexpected response: %1");
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
        showMessage(Tr::tr("QDB message: %1").arg(message), true);
    }
}

} // Qdb::Internal
