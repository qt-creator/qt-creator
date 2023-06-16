// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "actionsfilter.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "coreplugintr.h"
#include "icore.h"
#include "locator/locatormanager.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/fuzzymatcher.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QMenuBar>
#include <QPointer>
#include <QRegularExpression>
#include <QtConcurrent>
#include <QTextDocument>

using namespace Utils;

static const char lastTriggeredC[] = "LastTriggeredActions";

QT_BEGIN_NAMESPACE
size_t qHash(const QPointer<QAction> &p, size_t seed)
{
    return qHash(p.data(), seed);
}
QT_END_NAMESPACE

namespace Core::Internal {

static const QList<QAction *> menuBarActions()
{
    QMenuBar *menuBar = Core::ActionManager::actionContainer(Constants::MENU_BAR)->menuBar();
    QTC_ASSERT(menuBar, return {});
    return menuBar->actions();
}

ActionsFilter::ActionsFilter()
{
    setId("Actions from the menu");
    setDisplayName(Tr::tr("Global Actions & Actions from the Menu"));
    setDescription(Tr::tr("Triggers an action. If it is from the menu it matches any part "
                          "of a menu hierarchy, separated by \">\". For example \"sess def\" "
                          "matches \"File > Sessions > Default\"."));
    setDefaultShortcutString("t");
    setDefaultSearchText({});
    setDefaultKeySequence(QKeySequence("Ctrl+Shift+K"));
    connect(ICore::instance(), &ICore::contextAboutToChange, this, [this] {
        if (LocatorManager::locatorHasFocus())
            updateEnabledActionCache();
    });
}

static void matches(QPromise<void> &promise, const LocatorStorage &storage,
                    const LocatorFilterEntries &entries)
{
    using Highlight = LocatorFilterEntry::HighlightInfo;
    using MatchLevel = ILocatorFilter::MatchLevel;
    const QString input = storage.input();
    const QRegularExpression regExp = ILocatorFilter::createRegExp(input, Qt::CaseInsensitive,
                                                                   true);
    using FilterResult = std::pair<MatchLevel, LocatorFilterEntry>;
    const auto filter = [&](const LocatorFilterEntry &filterEntry) -> std::optional<FilterResult> {
        if (promise.isCanceled())
            return {};
        Highlight highlight;

        const auto withHighlight = [&highlight](LocatorFilterEntry result) {
            result.highlightInfo = highlight;
            return result;
        };

        Highlight::DataType first = Highlight::DisplayName;
        QString allText = filterEntry.displayName + ' ' + filterEntry.extraInfo;
        QRegularExpressionMatch allTextMatch = regExp.match(allText);
        if (!allTextMatch.hasMatch()) {
            first = Highlight::ExtraInfo;
            allText = filterEntry.extraInfo + ' ' + filterEntry.displayName;
            allTextMatch = regExp.match(allText);
        }
        if (allTextMatch.hasMatch()) {
            if (first == Highlight::DisplayName) {
                const QRegularExpressionMatch displayMatch = regExp.match(filterEntry.displayName);
                if (displayMatch.hasMatch())
                    highlight = ILocatorFilter::highlightInfo(displayMatch);
                const QRegularExpressionMatch extraMatch = regExp.match(filterEntry.extraInfo);
                if (extraMatch.hasMatch()) {
                    Highlight extraHighlight = ILocatorFilter::highlightInfo(extraMatch,
                                                                             Highlight::ExtraInfo);
                    highlight.startsExtraInfo = extraHighlight.startsExtraInfo;
                    highlight.lengthsExtraInfo = extraHighlight.lengthsExtraInfo;
                }

                if (filterEntry.displayName.startsWith(input, Qt::CaseInsensitive))
                    return FilterResult{MatchLevel::Best, withHighlight(filterEntry)};
                if (filterEntry.displayName.contains(input, Qt::CaseInsensitive))
                    return FilterResult{MatchLevel::Better, withHighlight(filterEntry)};
                if (displayMatch.hasMatch())
                    return FilterResult{MatchLevel::Good, withHighlight(filterEntry)};
                if (extraMatch.hasMatch())
                    return FilterResult{MatchLevel::Normal, withHighlight(filterEntry)};
            }

            const FuzzyMatcher::HighlightingPositions positions
                = FuzzyMatcher::highlightingPositions(allTextMatch);
            const int positionsCount = positions.starts.count();
            QTC_ASSERT(positionsCount == positions.lengths.count(), return {});
            const int border = first == Highlight::DisplayName ? filterEntry.displayName.length()
                                                               : filterEntry.extraInfo.length();
            for (int i = 0; i < positionsCount; ++i) {
                int start = positions.starts.at(i);
                const int length = positions.lengths.at(i);
                Highlight::DataType type = first;
                if (start > border) {
                    // this highlight is behind the border so switch type and reset start index
                    start -= border + 1;
                    type = first == Highlight::DisplayName ? Highlight::ExtraInfo
                                                           : Highlight::DisplayName;
                } else if (start + length > border) {
                    const int firstStart = start;
                    const int firstLength = border - start;
                    const int secondStart = 0;
                    const int secondLength = length - firstLength - 1;
                    if (first == Highlight::DisplayName) {
                        highlight.startsDisplay.append(firstStart);
                        highlight.lengthsDisplay.append(firstLength);
                        highlight.startsExtraInfo.append(secondStart);
                        highlight.lengthsExtraInfo.append(secondLength);
                    } else {
                        highlight.startsExtraInfo.append(firstStart);
                        highlight.lengthsExtraInfo.append(firstLength);
                        highlight.startsDisplay.append(secondStart);
                        highlight.lengthsDisplay.append(secondLength);
                    }
                    continue;
                }
                if (type == Highlight::DisplayName) {
                    highlight.startsDisplay.append(start);
                    highlight.lengthsDisplay.append(length);
                } else {
                    highlight.startsExtraInfo.append(start);
                    highlight.lengthsExtraInfo.append(length);
                }
            }

            return FilterResult{MatchLevel::Normal, withHighlight(filterEntry)};
        }
        return {};
    };

    QMap<MatchLevel, LocatorFilterEntries> filtered;
    const QList<std::optional<FilterResult>> filterResults
        = QtConcurrent::blockingMapped(entries, filter);
    if (promise.isCanceled())
        return;
    for (const std::optional<FilterResult> &filterResult : filterResults) {
        if (promise.isCanceled())
            return;
        if (filterResult)
            filtered[filterResult->first] << filterResult->second;
    }
    storage.reportOutput(std::accumulate(std::begin(filtered), std::end(filtered),
                                         LocatorFilterEntries()));
}

LocatorMatcherTasks ActionsFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [this, storage](Async<void> &async) {
        m_entries.clear();
        m_indexes.clear();
        QList<const QMenu *> processedMenus;
        collectEntriesForLastTriggered();
        for (QAction* action : menuBarActions())
            collectEntriesForAction(action, {}, processedMenus);
        collectEntriesForCommands();
        if (storage->input().simplified().isEmpty()) {
            storage->reportOutput(m_entries);
            return SetupResult::StopWithDone;
        }
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(matches, *storage, m_entries);
        return SetupResult::Continue;
    };

    return {{AsyncTask<void>(onSetup), storage}};
}

LocatorFilterEntry::Acceptor ActionsFilter::acceptor(const ActionFilterEntryData &data) const
{
    static const int maxHistorySize = 30;
    return [this, data] {
        if (!data.action)
            return AcceptResult();
        m_lastTriggered.removeAll(data);
        m_lastTriggered.prepend(data);
        QMetaObject::invokeMethod(data.action, [action = data.action] {
            if (action && action->isEnabled())
                action->trigger();
        }, Qt::QueuedConnection);
        if (m_lastTriggered.size() > maxHistorySize)
            m_lastTriggered.resize(maxHistorySize);
        return AcceptResult();
    };
}

static QString actionText(QAction *action)
{
    const QString whatsThis = action->whatsThis();
    return Utils::stripAccelerator(action->text())
           + (whatsThis.isEmpty() ? QString() : QString(" (" + whatsThis + ")"));
}

void ActionsFilter::collectEntriesForAction(QAction *action,
                                            const QStringList &path,
                                            QList<const QMenu *> &processedMenus)
{
    if (!m_enabledActions.contains(action))
        return;
    const QString text = actionText(action);
    if (QMenu *menu = action->menu()) {
        if (processedMenus.contains(menu))
            return;
        processedMenus.append(menu);
        if (menu->isEnabled()) {
            const QList<QAction *> &actions = menu->actions();
            QStringList menuPath(path);
            menuPath << text;
            for (QAction *menuAction : actions)
                collectEntriesForAction(menuAction, menuPath, processedMenus);
        }
    } else if (!text.isEmpty()) {
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = text;
        filterEntry.acceptor = acceptor({action, {}});
        filterEntry.displayIcon = action->icon();
        filterEntry.extraInfo = path.join(" > ");
        updateEntry(action, filterEntry);
    }
}

void ActionsFilter::collectEntriesForCommands()
{
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
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = text;
        filterEntry.acceptor = acceptor({action, command->id()});
        filterEntry.displayIcon = action->icon();
        filterEntry.displayExtra = command->keySequence().toString(QKeySequence::NativeText);
        if (path.size() >= 2)
            filterEntry.extraInfo = path.mid(0, path.size() - 1).join(" > ");
        updateEntry(action, filterEntry);
    }
}

void ActionsFilter::collectEntriesForLastTriggered()
{
    for (ActionFilterEntryData &data : m_lastTriggered) {
        if (!data.action) {
            if (Command *command = Core::ActionManager::command(data.commandId))
                data.action = command->action();
        }
        if (!data.action || !m_enabledActions.contains(data.action))
            continue;
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = actionText(data.action);
        filterEntry.acceptor = acceptor(data);
        filterEntry.displayIcon = data.action->icon();
        updateEntry(data.action, filterEntry);
    }
}

void ActionsFilter::updateEntry(const QPointer<QAction> action, const LocatorFilterEntry &entry)
{
    const int index = m_indexes.value(action, -1);
    if (index < 0) {
        m_indexes[action] = m_entries.size();
        m_entries << entry;
    } else {
        m_entries[index] = entry;
    }
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
    for (QAction *action : std::as_const(queue))
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

void ActionsFilter::saveState(QJsonObject &object) const
{
    QJsonArray commands;
    for (const ActionFilterEntryData &data : m_lastTriggered) {
        if (data.commandId.isValid())
            commands.append(data.commandId.toString());
    }
    object.insert(lastTriggeredC, commands);
}

void ActionsFilter::restoreState(const QJsonObject &object)
{
    m_lastTriggered.clear();
    const QJsonArray commands = object.value(lastTriggeredC).toArray();
    for (const QJsonValue &command : commands) {
        if (command.isString())
            m_lastTriggered.append({nullptr, Id::fromString(command.toString())});
    }
}

} // Core::Internal
