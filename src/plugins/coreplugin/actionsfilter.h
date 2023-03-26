// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

#include <QAction>
#include <QPointer>
#include <QSet>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

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
    Q_OBJECT
public:
    ActionsFilter();

    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override;
    void accept(const LocatorFilterEntry &selection, QString *newText,
                int *selectionStart, int *selectionLength) const override;
    void prepareSearch(const QString &entry) override;

private:
    void saveState(QJsonObject &object) const override;
    void restoreState(const QJsonObject &object) override;
    void collectEntriesForAction(QAction *action,
                                 const QStringList &path,
                                 QList<const QMenu *> &processedMenus);
    void collectEntriesForCommands();
    void collectEntriesForLastTriggered();
    void updateEntry(const QPointer<QAction> action, const LocatorFilterEntry &entry);
    void updateEnabledActionCache();

    QList<LocatorFilterEntry> m_entries;
    QMap<QPointer<QAction>, int> m_indexes;
    QSet<QPointer<QAction>> m_enabledActions;
    mutable QList<ActionFilterEntryData> m_lastTriggered;
};

} // namespace Internal
} // namespace Core
