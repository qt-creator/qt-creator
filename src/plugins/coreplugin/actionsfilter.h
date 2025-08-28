// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "locator/ilocatorfilter.h"

#include <QAction>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Core::Internal {

class ActionFilterEntryData
{
public:
    ActionFilterEntryData() = default;
    ActionFilterEntryData(const QPointer<QAction> &action, const Utils::Id &commandId);
    explicit ActionFilterEntryData(const Utils::Id &optionsPageId);

    QPointer<QAction> action;
    Utils::Id commandId;
    Utils::Id optionsPageId;

    friend bool operator==(const ActionFilterEntryData &a, const ActionFilterEntryData &b);
};

class ActionEntryCache;

class ActionsFilter final : public ILocatorFilter
{
public:
    ActionsFilter();

private:
    LocatorMatcherTasks matchers() final;
    void saveState(QJsonObject &object) const override;
    void restoreState(const QJsonObject &object) override;
    LocatorFilterEntry::Acceptor acceptor(const ActionFilterEntryData &data) const;
    void collectEntriesForAction(QAction *action, const QStringList &path,
                                 QList<const QMenu *> &processedMenus, ActionEntryCache *cache) const;
    void collectEntriesForCommands(ActionEntryCache *cache) const;
    void collectEntriesForLastTriggered(ActionEntryCache *cache) const;
    LocatorFilterEntries collectEntriesForPreferences() const;

    QList<QPointer<QAction>> m_enabledActions;
    mutable QList<ActionFilterEntryData> m_lastTriggered;
};

} // namespace Core::Internal
