// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "actionsfilter.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "coreplugintr.h"
#include "icore.h"
#include "locator/locatormanager.h"

#include <utils/algorithm.h>
#include <utils/fuzzymatcher.h>
#include <utils/mapreduce.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QMenuBar>
#include <QPointer>
#include <QRegularExpression>
#include <QTextDocument>

static const char lastTriggeredC[] = "LastTriggeredActions";

QT_BEGIN_NAMESPACE
size_t qHash(const QPointer<QAction> &p, size_t seed)
{
    return qHash(p.data(), seed);
}
QT_END_NAMESPACE

namespace Core::Internal {

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

static const QList<QAction *> menuBarActions()
{
    QMenuBar *menuBar = Core::ActionManager::actionContainer(Constants::MENU_BAR)->menuBar();
    QTC_ASSERT(menuBar, return {});
    return menuBar->actions();
}

QList<LocatorFilterEntry> ActionsFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                    const QString &entry)
{
    using Highlight = LocatorFilterEntry::HighlightInfo;
    if (entry.simplified().isEmpty())
        return m_entries;

    const QRegularExpression regExp = createRegExp(entry, Qt::CaseInsensitive, true);

    using FilterResult = std::pair<MatchLevel, LocatorFilterEntry>;
    const auto filter = [&](const LocatorFilterEntry &filterEntry) -> std::optional<FilterResult> {
        if (future.isCanceled())
            return {};
        Highlight highlight;

        const auto withHighlight = [&](LocatorFilterEntry result) {
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
                    highlight = highlightInfo(displayMatch);
                const QRegularExpressionMatch extraMatch = regExp.match(filterEntry.extraInfo);
                if (extraMatch.hasMatch()) {
                    Highlight extraHighlight = highlightInfo(extraMatch, Highlight::ExtraInfo);
                    highlight.startsExtraInfo = extraHighlight.startsExtraInfo;
                    highlight.lengthsExtraInfo = extraHighlight.lengthsExtraInfo;
                }

                if (filterEntry.displayName.startsWith(entry, Qt::CaseInsensitive))
                    return FilterResult{MatchLevel::Best, withHighlight(filterEntry)};
                if (filterEntry.displayName.contains(entry, Qt::CaseInsensitive))
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

    QMap<MatchLevel, QList<LocatorFilterEntry>> filtered;
    const QList<std::optional<FilterResult>> filterResults = Utils::map(std::as_const(m_entries), filter)
                                                                 .results();
    for (const std::optional<FilterResult> &filterResult : filterResults) {
        if (filterResult)
            filtered[filterResult->first] << filterResult->second;
    }

    QList<LocatorFilterEntry> result;
    for (const QList<LocatorFilterEntry> &sublist : std::as_const(filtered))
        result << sublist;
    return result;
}

void ActionsFilter::accept(const LocatorFilterEntry &selection, QString *newText,
                           int *selectionStart, int *selectionLength) const
{
    static const int maxHistorySize = 30;
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    auto data = selection.internalData.value<ActionFilterEntryData>();
    if (data.action) {
        m_lastTriggered.removeAll(data);
        m_lastTriggered.prepend(data);
        QMetaObject::invokeMethod(data.action, [action = data.action] {
            if (action && action->isEnabled())
                action->trigger();
        }, Qt::QueuedConnection);
        if (m_lastTriggered.size() > maxHistorySize)
            m_lastTriggered.resize(maxHistorySize);
    }
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
        const ActionFilterEntryData data{action, {}};
        LocatorFilterEntry filterEntry(this, text, QVariant::fromValue(data), action->icon());
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
        const ActionFilterEntryData data{action, command->id()};
        LocatorFilterEntry filterEntry(this, text, QVariant::fromValue(data), action->icon());
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
        const QString text = actionText(data.action);
        LocatorFilterEntry filterEntry(this, text, QVariant::fromValue(data), data.action->icon());
        updateEntry(data.action, filterEntry);
    }
}

void ActionsFilter::updateEntry(const QPointer<QAction> action, const LocatorFilterEntry &entry)
{
    auto index = m_indexes.value(action, -1);
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

void Core::Internal::ActionsFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_entries.clear();
    m_indexes.clear();
    QList<const QMenu *> processedMenus;
    collectEntriesForLastTriggered();
    for (QAction* action : menuBarActions())
        collectEntriesForAction(action, QStringList(), processedMenus);
    collectEntriesForCommands();
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
    for (const QJsonValue &command : object.value(lastTriggeredC).toArray()) {
        if (command.isString())
            m_lastTriggered.append({nullptr, Utils::Id::fromString(command.toString())});
    }
}

} // Core::Internal
