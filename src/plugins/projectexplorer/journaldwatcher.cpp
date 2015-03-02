/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "journaldwatcher.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QSocketNotifier>

#include <systemd/sd-journal.h>

namespace ProjectExplorer {

JournaldWatcher *JournaldWatcher::m_instance = 0;

namespace Internal {

class JournaldWatcherPrivate
{
public:
    JournaldWatcherPrivate() :
        m_journalContext(0),
        m_notifier(0)
    { }

    ~JournaldWatcherPrivate()
    {
        teardown();
    }

    bool setup();
    void teardown();

    JournaldWatcher::LogEntry retrieveEntry();

    class SubscriberInformation {
    public:
        SubscriberInformation(QObject *sr, const JournaldWatcher::Subscription &sn) :
            subscriber(sr), subscription(sn)
        { }

        QObject *subscriber;
        JournaldWatcher::Subscription subscription;
    };
    QList<SubscriberInformation> m_subscriptions;

    sd_journal *m_journalContext;
    QSocketNotifier *m_notifier;
};

bool JournaldWatcherPrivate::setup()
{
    QTC_ASSERT(!m_journalContext, return false);
    int r = sd_journal_open(&m_journalContext, 0);
    if (r != 0)
        return false;

    r = sd_journal_seek_tail(m_journalContext);
    if (r != 0)
        return false;

    // Work around https://bugs.freedesktop.org/show_bug.cgi?id=64614
    sd_journal_previous(m_journalContext);

    int fd = sd_journal_get_fd(m_journalContext);
    if (fd < 0)
        return false;

    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read);
    return true;
}

void JournaldWatcherPrivate::teardown()
{
    delete m_notifier;
    m_notifier = 0;

    if (m_journalContext) {
        sd_journal_close(m_journalContext);
        m_journalContext = 0;
    }
}

JournaldWatcher::LogEntry JournaldWatcherPrivate::retrieveEntry()
{
    JournaldWatcher::LogEntry result;

    // Advance:
    int r = sd_journal_next(m_journalContext);
    if (r == 0)
        return result;

    const void *rawData;
    size_t length;
    SD_JOURNAL_FOREACH_DATA(m_journalContext, rawData, length) {
        QByteArray tmp = QByteArray::fromRawData(static_cast<const char *>(rawData), length);
        int offset = tmp.indexOf('=');
        if (offset < 0)
            continue;
        result.insert(tmp.left(offset), tmp.mid(offset + 1));
    }

    return result;
}

} // namespace Internal

using namespace Internal;

static JournaldWatcherPrivate *d = 0;

JournaldWatcher::~JournaldWatcher()
{
    d->teardown();

    m_instance = 0;

    delete d;
    d = 0;
}

JournaldWatcher *JournaldWatcher::instance()
{
    return m_instance;
}

const QByteArray &JournaldWatcher::machineId()
{
    static QByteArray id;
    if (id.isEmpty()) {
        sd_id128 sdId;
        if (sd_id128_get_machine(&sdId) == 0)
            id = QByteArray(reinterpret_cast<char *>(sdId.bytes), 16);
    }
    return id;
}

bool JournaldWatcher::subscribe(QObject *subscriber, const Subscription &subscription)
{
    // Check that we do not have that subscriber yet:
    int pos = Utils::indexOf(d->m_subscriptions,
                             [subscriber](const JournaldWatcherPrivate::SubscriberInformation &info) {
                                 return info.subscriber == subscriber;
                             });
    QTC_ASSERT(pos >= 0, return false);

    d->m_subscriptions.append(JournaldWatcherPrivate::SubscriberInformation(subscriber, subscription));
    connect(subscriber, &QObject::destroyed, m_instance, &JournaldWatcher::unsubscribe);

    return true;
}

void JournaldWatcher::unsubscribe(QObject *subscriber)
{
    int pos = Utils::indexOf(d->m_subscriptions,
                            [subscriber](const JournaldWatcherPrivate::SubscriberInformation &info) {
                                return info.subscriber == subscriber;
                            });
    if (pos < 0)
        return;

    d->m_subscriptions.removeAt(pos);
}

JournaldWatcher::JournaldWatcher()
{
    QTC_ASSERT(!m_instance, return);

    d = new JournaldWatcherPrivate;
    m_instance = this;

    if (!d->setup())
        d->teardown();
    else
        connect(d->m_notifier, &QSocketNotifier::activated, m_instance, &JournaldWatcher::handleEntry);
    m_instance->handleEntry(); // advance to the end of file...
}

void JournaldWatcher::handleEntry()
{
    if (!d->m_notifier)
        return;

    LogEntry logEntry;
    forever {
        logEntry = d->retrieveEntry();
        if (logEntry.isEmpty())
            break;

        foreach (const JournaldWatcherPrivate::SubscriberInformation &info, d->m_subscriptions)
            info.subscription(logEntry);

        if (logEntry.value(QByteArrayLiteral("_MACHINE_ID")) != machineId())
            continue;

        const QByteArray pid = logEntry.value(QByteArrayLiteral("_PID"));
        quint64 pidNum = pid.isEmpty() ? 0 : QString::fromLatin1(pid).toInt();

        QString message = QString::fromUtf8(logEntry.value(QByteArrayLiteral("MESSAGE")));

        emit journaldOutput(pidNum, message);
    }
}

} // namespace ProjectExplorer
