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

#pragma once

#include "projectexplorer_export.h"

#include <QHash>
#include <QObject>

#include <functional>

namespace ProjectExplorer {

class Project;
class ProjectConfiguration;
class Target;

namespace Internal {

class PROJECTEXPLORER_EXPORT Subscription : public QObject
{
    Q_OBJECT
public:
    using Connector = std::function<QMetaObject::Connection(ProjectConfiguration *)>;

    Subscription(const Connector &s, const QObject *receiver, QObject *parent);
    ~Subscription() override;

protected:
    void subscribe(ProjectConfiguration *pc);
    void unsubscribe(ProjectConfiguration *pc);

    Connector m_subscriber;
    QHash<ProjectConfiguration *, QMetaObject::Connection> m_connections;
};

class PROJECTEXPLORER_EXPORT ProjectSubscription : public Subscription
{
public:
    ProjectSubscription(const Connector &s, const QObject *receiver, Project *p);
    ~ProjectSubscription() final;
};

class PROJECTEXPLORER_EXPORT TargetSubscription : public Subscription
{
public:
    TargetSubscription(const Connector &s, const QObject *receiver, Target *t);
    ~TargetSubscription() final;
};

} // namespace Internal
} // namespace ProjectExplorer
