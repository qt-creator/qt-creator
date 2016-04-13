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

#include "journaldwatcher.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QSocketNotifier>

#include <systemd/sd-journal.h>

namespace ProjectExplorer {

JournaldWatcher *JournaldWatcher::m_instance = nullptr;

namespace Internal {

class JournaldWatcherPrivate
{
public:
    JournaldWatcherPrivate() = default;
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

    sd_journal *m_journalContext = nullptr;
    QSocketNotifier *m_notifier = nullptr;
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
    m_notifier = nullptr;

    if (m_journalContext) {
        sd_journal_close(m_journalContext);
        m_journalContext = nullptr;
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

static JournaldWatcherPrivate *d = nullptr;

JournaldWatcher::~JournaldWatcher()
{
    d->teardown();

    m_instance = nullptr;

    delete d;
    d = nullptr;
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
        if (sd_id128_get_machine(&sdId) == 0) {
            id.resize(32);
            sd_id128_to_string(sdId, id.data());
        }
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
        connect(d->m_notifier, &QSocketNotifier::activated,
                m_instance, &JournaldWatcher::handleEntry);
    m_instance->handleEntry(); // advance to the end of file...
}

void JournaldWatcher::handleEntry()
{
    if (!d->m_notifier)
        return;

    if (sd_journal_process(d->m_journalContext) != SD_JOURNAL_APPEND)
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
        if (pid.isEmpty())
            continue;

        quint64 pidNum = QString::fromLatin1(pid).toInt();

        QString message = QString::fromUtf8(logEntry.value(QByteArrayLiteral("MESSAGE")));
        message.append(QLatin1Char('\n')); // Add newline.

        emit journaldOutput(pidNum, message);
    }
}

} // namespace ProjectExplorer
