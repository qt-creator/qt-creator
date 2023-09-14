// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "locator/ilocatorfilter.h"

#include <QAction>
#include <QPointer>
#include <QSet>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Core::Internal {

class ActionFilterEntryData
{
public:
    QPointer<QAction> action;
    Utils::Id commandId;
    friend bool operator==(const ActionFilterEntryData &a, const ActionFilterEntryData &b)
    {
        return a.action == b.action && a.commandId == b.commandId;
    }
};

class ActionsFilter : public ILocatorFilter
{
public:
    ActionsFilter();

private:
    LocatorMatcherTasks matchers() final;
    void saveState(QJsonObject &object) const override;
    void restoreState(const QJsonObject &object) override;
    LocatorFilterEntry::Acceptor acceptor(const ActionFilterEntryData &data) const;
    void collectEntriesForAction(QAction *action,
                                 const QStringList &path,
                                 QList<const QMenu *> &processedMenus);
    void collectEntriesForCommands();
    void collectEntriesForLastTriggered();
    void updateEntry(const QPointer<QAction> action, const LocatorFilterEntry &entry);
    void updateEnabledActionCache();

    LocatorFilterEntries m_entries;
    QMap<QPointer<QAction>, int> m_indexes;
    QSet<QPointer<QAction>> m_enabledActions;
    mutable QList<ActionFilterEntryData> m_lastTriggered;
};

} // namespace Core::Internal
