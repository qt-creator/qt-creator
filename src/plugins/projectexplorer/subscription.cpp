/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "subscription.h"

#include "project.h"
#include "projectconfiguration.h"
#include "target.h"

#include <utils/qtcassert.h>

namespace ProjectExplorer {
namespace Internal {

Subscription::Subscription(const Subscription::Connector &s, const QObject *receiver, QObject *parent) :
    QObject(parent), m_subscriber(s)
{
    if (receiver != parent) {
        connect(receiver, &QObject::destroyed, this, [this]() {
            unsubscribeAll();
            m_subscriber = Connector(); // Reset subscriber
            deleteLater();
        });
    }
}

Subscription::~Subscription()
{
    unsubscribeAll();
}

void Subscription::subscribe(ProjectConfiguration *pc)
{
    if (!m_subscriber)
        return;

    connectTo(pc);

    if (auto p = qobject_cast<Project *>(pc)) {
        for (Target *t : p->targets()) {
            for (ProjectConfiguration *pc : t->projectConfigurations())
                connectTo(pc);
        }
    } else if (auto t = qobject_cast<Target *>(pc)) {
        for (ProjectConfiguration *pc : t->projectConfigurations())
            connectTo(pc);
    }
}

void Subscription::unsubscribe(ProjectConfiguration *pc)
{
    disconnectFrom(pc);

    if (auto p = qobject_cast<Project *>(pc)) {
        for (Target *t : p->targets()) {
            for (ProjectConfiguration *pc : t->projectConfigurations())
                unsubscribe(pc);
        }
    } else if (auto t = qobject_cast<Target *>(pc)) {
        for (ProjectConfiguration *pc : t->projectConfigurations())
            unsubscribe(pc);
    }
}

void Subscription::unsubscribeAll()
{
    for (const auto &c : qAsConst(m_connections))
        disconnect(c);
    m_connections.clear();
}

void Subscription::connectTo(ProjectConfiguration *pc)
{
    QTC_ASSERT(!m_connections.contains(pc), return);

    QMetaObject::Connection conn = m_subscriber(pc);
    if (conn)
        m_connections.insert(pc, conn);
}

void Subscription::disconnectFrom(ProjectConfiguration *pc)
{
    auto c = m_connections.value(pc);
    if (!c)
        return;

    disconnect(c);
    m_connections.remove(pc);
}

ProjectSubscription::ProjectSubscription(const Subscription::Connector &s, const QObject *r,
                                         Project *p) :
    Subscription(s, r, p)
{
    if (m_subscriber) {
        for (const Target *t : p->targets()) {
            for (ProjectConfiguration *pc : t->projectConfigurations())
                m_subscriber(pc);
        }
        connect(p, &Project::addedProjectConfiguration, this, &ProjectSubscription::subscribe);
        connect(p, &Project::removedProjectConfiguration, this, &ProjectSubscription::unsubscribe);
    }
}

ProjectSubscription::~ProjectSubscription() = default;

TargetSubscription::TargetSubscription(const Subscription::Connector &s, const QObject *r,
                                       Target *t) :
    Subscription(s, r, t)
{
    for (ProjectConfiguration *pc : t->projectConfigurations())
        m_subscriber(pc);
    connect(t, &Target::addedProjectConfiguration, this, &TargetSubscription::subscribe);
    connect(t, &Target::removedProjectConfiguration, this, &TargetSubscription::unsubscribe);
}

TargetSubscription::~TargetSubscription() = default;

} // namespace Internal
} // namespace ProjectExplorer
