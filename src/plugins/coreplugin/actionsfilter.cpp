// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "actionsfilter.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "coreplugintr.h"
#include "dialogs/ioptionspage.h"
#include "icore.h"
#include "locator/locatormanager.h"

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

using namespace Tasking;
using namespace Utils;

static const char lastTriggeredC[] = "LastTriggeredActions";

namespace Core::Internal {

static const QList<QAction *> menuBarActions()
{
    QMenuBar *menuBar = Core::ActionManager::actionContainer(Constants::MENU_BAR)->menuBar();
    QTC_ASSERT(menuBar, return {});
    return menuBar->actions();
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

static QList<QPointer<QAction>> collectEnabledActions()
{
    QSet<QAction *> enabledActions;
    QList<QAction *> queue = menuBarActions();
    for (QAction *action : std::as_const(queue))
        requestMenuUpdate(action);
    while (!queue.isEmpty()) {
        QAction *action = queue.takeFirst();
        if (action->isEnabled() && !action->isSeparator() && action->isVisible()) {
            enabledActions.insert(action);
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
            enabledActions.insert(command->action());
        }
    }
    return Utils::transform<QList>(enabledActions, [](QAction *action) {
        return QPointer<QAction>(action);
    });
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
        if (LocatorManager::locatorHasFocus()) {
            m_enabledActions = collectEnabledActions();
        }
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

class ActionEntryCache
{
public:
    ActionEntryCache(const QSet<QAction *> &enabledActions)
        : m_enabledActions(enabledActions)
    {}

    void update(QAction *action, const LocatorFilterEntry &entry)
    {
        const int index = m_actionIndexCache.value(action, -1);
        if (index < 0) {
            m_actionIndexCache[action] = m_entries.size();
            m_entries << entry;
        } else {
            m_entries[index] = entry;
        }
    }

    bool isEnabled(QAction *action) const { return m_enabledActions.contains(action); }

    LocatorFilterEntries entries() const { return m_entries; }

private:
    LocatorFilterEntries m_entries;
    QHash<QAction *, int> m_actionIndexCache;
    QSet<QAction *> m_enabledActions;
};

LocatorMatcherTasks ActionsFilter::matchers()
{
    const auto onSetup = [this](Async<void> &async) {
        m_enabledActions = Utils::filtered(m_enabledActions, [](const QPointer<QAction> &action) {
            return action.get();
        });
        ActionEntryCache cache(Utils::transform<QSet>(m_enabledActions, [](const QPointer<QAction> &action) {
            return action.get();
        }));
        QList<const QMenu *> processedMenus;
        collectEntriesForLastTriggered(&cache);
        for (QAction* action : menuBarActions())
            collectEntriesForAction(action, {}, processedMenus, &cache);
        collectEntriesForCommands(&cache);
        const LocatorFilterEntries entries = cache.entries() + collectEntriesForPreferences();
        const LocatorStorage &storage = *LocatorStorage::storage();
        if (storage.input().simplified().isEmpty()) {
            storage.reportOutput(entries);
            return SetupResult::StopWithSuccess;
        }
        async.setConcurrentCallData(matches, storage, entries);
        return SetupResult::Continue;
    };

    return {AsyncTask<void>(onSetup)};
}

LocatorFilterEntry::Acceptor ActionsFilter::acceptor(const ActionFilterEntryData &data) const
{
    static const int maxHistorySize = 30;
    return [this, data] {
        if (!data.action && !data.optionsPageId.isValid())
            return AcceptResult();
        m_lastTriggered.removeAll(data);
        m_lastTriggered.prepend(data);
        if (data.action) {
            QMetaObject::invokeMethod(
                data.action,
                [action = data.action] {
                    if (action && action->isEnabled())
                        action->trigger();
                },
                Qt::QueuedConnection);
        } else if (data.optionsPageId.isValid()) {
            QMetaObject::invokeMethod(
                Core::ICore::instance(),
                [id = data.optionsPageId] { Core::ICore::showOptionsDialog(id); },
                Qt::QueuedConnection);
        }
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
                                            QList<const QMenu *> &processedMenus,
                                            ActionEntryCache *cache) const
{
    if (!cache->isEnabled(action))
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
                collectEntriesForAction(menuAction, menuPath, processedMenus, cache);
        }
    } else if (!text.isEmpty()) {
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = text;
        filterEntry.acceptor = acceptor(ActionFilterEntryData{action, {}});
        filterEntry.displayIcon = action->icon();
        filterEntry.extraInfo = path.join(" > ");
        cache->update(action, filterEntry);
    }
}

void ActionsFilter::collectEntriesForCommands(ActionEntryCache *cache) const
{
    const QList<Command *> commands = Core::ActionManager::commands();
    for (const Command *command : commands) {
        QAction *action = command->action();
        if (!cache->isEnabled(action))
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
        filterEntry.acceptor = acceptor(ActionFilterEntryData{action, command->id()});
        filterEntry.displayIcon = action->icon();
        filterEntry.displayExtra = command->keySequence().toString(QKeySequence::NativeText);
        if (path.size() >= 2)
            filterEntry.extraInfo = path.mid(0, path.size() - 1).join(" > ");
        cache->update(action, filterEntry);
    }
}

void ActionsFilter::collectEntriesForLastTriggered(ActionEntryCache *cache) const
{
    for (ActionFilterEntryData &data : m_lastTriggered) {
        if (!data.action) {
            if (Command *command = Core::ActionManager::command(data.commandId))
                data.action = command->action();
        }
        if (!data.action || !cache->isEnabled(data.action))
            continue;
        LocatorFilterEntry filterEntry;
        filterEntry.displayName = actionText(data.action);
        filterEntry.acceptor = acceptor(data);
        filterEntry.displayIcon = data.action->icon();
        cache->update(data.action, filterEntry);
    }
}

LocatorFilterEntries ActionsFilter::collectEntriesForPreferences() const
{
    static QHash<IOptionsPage *, LocatorFilterEntries> entriesForPages;
    static QMap<Utils::Id, QString> categoryDisplay;
    for (IOptionsPage *page : IOptionsPage::allOptionsPages()) {
        if (!categoryDisplay.contains(page->category()) && !page->displayCategory().isEmpty())
            categoryDisplay[page->category()] = page->displayCategory();
    }
    QList<IOptionsPage *> oldPages = entriesForPages.keys();
    for (IOptionsPage *page : IOptionsPage::allOptionsPages()) {
        oldPages.removeAll(page);
        if (entriesForPages.contains(page))
            continue;
        if (const std::optional<AspectContainer *> aspects = page->aspects()) {
            const Id optionsPageId = page->id();
            const QStringList extraInfo = {Tr::tr("Preferences"), page->displayName()};
            std::function<void(AspectContainer *, const QStringList &)> collectContainerEntries;
            collectContainerEntries =
                [this,
                 &collectContainerEntries,
                 &optionsPageId,
                 &page](AspectContainer *container, const QStringList &extraInfo) {
                    for (auto aspect : container->aspects()) {
                        if (auto container = qobject_cast<AspectContainer *>(aspect)) {
                            collectContainerEntries(
                                container, QStringList(extraInfo) << container->displayName());
                        } else if (!aspect->displayName().isEmpty()) {
                            LocatorFilterEntry filterEntry;
                            filterEntry.displayName = aspect->displayName();
                            filterEntry.acceptor = acceptor(ActionFilterEntryData(optionsPageId));
                            filterEntry.extraInfo = extraInfo.join(" > ");
                            filterEntry.displayIcon = aspect->icon();
                            entriesForPages[page].append(filterEntry);
                        }
                    }
                };
            collectContainerEntries(*aspects, extraInfo);
            if (!entriesForPages.contains(page))
                entriesForPages.insert(page, {});
        }
    }
    for (auto oldPage : std::as_const(oldPages))
        entriesForPages.remove(oldPage);
    LocatorFilterEntries result;
    for (const LocatorFilterEntries &entries : std::as_const(entriesForPages))
        result.append(entries);
    return result;
}

void ActionsFilter::saveState(QJsonObject &object) const
{
    QJsonArray commands;
    for (const ActionFilterEntryData &data : std::as_const(m_lastTriggered)) {
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
            m_lastTriggered.append(ActionFilterEntryData{nullptr, Id::fromString(command.toString())});
    }
}

ActionFilterEntryData::ActionFilterEntryData(
    const QPointer<QAction> &action, const Utils::Id &commandId)
    : action(action)
    , commandId(commandId)
{}

ActionFilterEntryData::ActionFilterEntryData(const Utils::Id &optionsPageId)
    : optionsPageId(optionsPageId)
{}

bool operator==(const ActionFilterEntryData &a, const ActionFilterEntryData &b)
{
    return a.action == b.action && a.commandId == b.commandId
           && a.optionsPageId == b.optionsPageId;
}

} // Core::Internal
