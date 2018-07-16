/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

class MenuBarFilter : public ILocatorFilter
{
    Q_OBJECT
public:
    MenuBarFilter();

    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override;
    void accept(LocatorFilterEntry selection, QString *newText,
                int *selectionStart, int *selectionLength) const override;
    void refresh(QFutureInterface<void> &future) override;
    void prepareSearch(const QString &entry) override;
private:
    QList<LocatorFilterEntry> matchesForAction(QAction *action,
                                               const QStringList &entryPath,
                                               const QStringList &path,
                                               QVector<const QMenu *> &processedMenus);
    void updateEnabledActionCache();

    QList<LocatorFilterEntry> m_entries;
    QSet<QPointer<QAction>> m_enabledActions;
};

} // namespace Internal
} // namespace Core
