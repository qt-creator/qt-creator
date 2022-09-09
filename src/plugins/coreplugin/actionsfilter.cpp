// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "actionsfilter.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "icore.h"
#include "locator/locatormanager.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QMenuBar>
#include <QPointer>
#include <QRegularExpression>
#include <QTextDocument>

QT_BEGIN_NAMESPACE
size_t qHash(const QPointer<QAction> &p, size_t seed)
{
    return qHash(p.data(), seed);
}
QT_END_NAMESPACE

using namespace Core::Internal;
using namespace Core;

class ActionFilterEntryData
{
public:
    QPointer<QAction> action;
    QStringList path;
};

ActionsFilter::ActionsFilter()
{
    setId("Actions from the menu");
    setDisplayName(tr("Global Actions & Actions from the Menu"));
    setDescription(
        tr("Triggers an action. If it is from the menu it matches any part of a menu hierarchy, "
           "separated by \">\". For example \"sess def\" matches \"File > Sessions > Default\"."));
    setDefaultShortcutString("t");
    setDefaultSearchText({});
    setDefaultKeySequence(QKeySequence("Ctrl+Shift+K"));
    connect(ICore::instance(), &ICore::contextAboutToChange, this, [this] {
        if (LocatorManager::locatorHasFocus())
            updateEnabledActionCache();
    });
}

static const QList<QAction *> menuBarActions()
{
    QMenuBar *menuBar = Core::ActionManager::actionContainer(Constants::MENU_BAR)->menuBar();
    QTC_ASSERT(menuBar, return {});
    return menuBar->actions();
}

QList<LocatorFilterEntry> ActionsFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                    const QString &entry)
{
    Q_UNUSED(future)
    static const QString separators = ". >/";
    static const QRegularExpression seperatorRegExp(QString("[%1]").arg(separators));
    QString normalized = entry;
    normalized.replace(seperatorRegExp, separators.at(0));
    const QStringList entryPath = normalized.split(separators.at(0), Qt::SkipEmptyParts);

    QList<LocatorFilterEntry> filtered;

    for (LocatorFilterEntry filterEntry : qAsConst(m_entries)) {
        int entryIndex = 0;
        int entryLength = 0;
        int pathIndex = 0;
        const ActionFilterEntryData data = filterEntry.internalData.value<ActionFilterEntryData>();
        const QString pathText = data.path.join(" > ");
        QStringList actionPath(data.path);
        if (!entryPath.isEmpty()) {
            actionPath << filterEntry.displayName;
            for (const QString &entry : entryPath) {
                const QRegularExpression re(".*" + entry + ".*",
                                            QRegularExpression::CaseInsensitiveOption);
                pathIndex = actionPath.indexOf(re, pathIndex);
                if (pathIndex < 0)
                    continue;
            }
            const QString &lastEntry(entryPath.last());
            entryLength = lastEntry.length();
            entryIndex = filterEntry.displayName.indexOf(lastEntry, 0, Qt::CaseInsensitive);
            LocatorFilterEntry::HighlightInfo::DataType highlightType =
                    LocatorFilterEntry::HighlightInfo::DisplayName;
            if (entryIndex >= 0) {
                highlightType = LocatorFilterEntry::HighlightInfo::DisplayName;
            } else {
                entryIndex = pathText.indexOf(lastEntry, 0, Qt::CaseInsensitive);
                if (entryIndex < 0)
                    continue;
                highlightType = LocatorFilterEntry::HighlightInfo::ExtraInfo;
            }
            filterEntry.highlightInfo = {entryIndex, entryLength, highlightType};
        }
        filtered << filterEntry;
    }

    return filtered;
}

void ActionsFilter::accept(const LocatorFilterEntry &selection, QString *newText,
                           int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    if (QPointer<QAction> action = selection.internalData.value<ActionFilterEntryData>().action) {
        QMetaObject::invokeMethod(action, [action] {
            if (action && action->isEnabled())
                action->trigger();
        }, Qt::QueuedConnection);
    }
}

QList<LocatorFilterEntry> ActionsFilter::entriesForAction(QAction *action,
                                                         const QStringList &path,
                                                         QList<const QMenu *> &processedMenus)
{
    QList<LocatorFilterEntry> entries;
    if (!m_enabledActions.contains(action))
        return entries;
    const QString whatsThis = action->whatsThis();
    const QString text = Utils::stripAccelerator(action->text())
                         + (whatsThis.isEmpty() ? QString() : QString(" (" + whatsThis + ")"));
    if (QMenu *menu = action->menu()) {
        if (processedMenus.contains(menu))
            return entries;
        processedMenus.append(menu);
        if (menu->isEnabled()) {
            const QList<QAction *> &actions = menu->actions();
            QStringList menuPath(path);
            menuPath << text;
            for (QAction *menuAction : actions)
                entries << entriesForAction(menuAction, menuPath, processedMenus);
        }
    } else if (!text.isEmpty()) {
        const ActionFilterEntryData data{action, path};
        LocatorFilterEntry filterEntry(this, text, QVariant::fromValue(data), action->icon());
        filterEntry.extraInfo = path.join(" > ");
        entries << filterEntry;
    }
    return entries;
}

QList<LocatorFilterEntry> ActionsFilter::entriesForCommands()
{
    QList<LocatorFilterEntry> entries;
    const QList<Command *> commands = Core::ActionManager::commands();
    for (const Command *command : commands) {
        QAction *action = command->action();
        if (!m_enabledActions.contains(action))
            continue;

        QString text = command->description();
        if (text.isEmpty())
            continue;
        if (text.contains('<')) {
            // removing HTML tags
            QTextDocument html;
            html.setHtml(text);
            text = html.toPlainText();
        }

        const QString identifier = command->id().toString();
        const QStringList path = identifier.split(QLatin1Char('.'));
        const ActionFilterEntryData data{action, path};
        LocatorFilterEntry filterEntry(this, text, QVariant::fromValue(data), action->icon());
        if (path.size() >= 2)
            filterEntry.extraInfo = path.mid(0, path.size() - 1).join(" > ");
        entries << filterEntry;
    }
    return entries;
}

static void requestMenuUpdate(const QAction* action)
{
    if (QMenu *menu = action->menu()) {
        emit menu->aboutToShow();
        const QList<QAction *> &actions = menu->actions();
        for (const QAction *menuActions : actions)
            requestMenuUpdate(menuActions);
    }
}

void ActionsFilter::updateEnabledActionCache()
{
    m_enabledActions.clear();
    QList<QAction *> queue = menuBarActions();
    for (QAction *action : qAsConst(queue))
        requestMenuUpdate(action);
    while (!queue.isEmpty()) {
        QAction *action = queue.takeFirst();
        if (action->isEnabled() && !action->isSeparator() && action->isVisible()) {
            m_enabledActions.insert(action);
            if (QMenu *menu = action->menu()) {
                if (menu->isEnabled())
                    queue.append(menu->actions());
            }
        }
    }
    const QList<Command *> commands = Core::ActionManager::commands();
    for (const Command *command : commands) {
        if (command && command->action() && command->action()->isEnabled()
                && !command->action()->isSeparator()) {
            m_enabledActions.insert(command->action());
        }
    }
}

void Core::Internal::ActionsFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_entries.clear();
    QList<const QMenu *> processedMenus;
    for (QAction* action : menuBarActions())
        m_entries << entriesForAction(action, QStringList(), processedMenus);
    m_entries << entriesForCommands();
}
