// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
    QList<LocatorFilterEntry> matchesForAction(QAction *action,
                                               const QStringList &entryPath,
                                               const QStringList &path,
                                               QList<const QMenu *> &processedMenus);
    QList<LocatorFilterEntry> matchesForCommands(const QString &entry);
    void updateEnabledActionCache();

    QList<LocatorFilterEntry> m_entries;
    QSet<QPointer<QAction>> m_enabledActions;
};

} // namespace Internal
} // namespace Core
