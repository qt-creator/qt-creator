// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminalsearch.h"
#include "terminalcommands.h"

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <chrono>

Q_LOGGING_CATEGORY(terminalSearchLog, "qtc.terminal.search", QtWarningMsg)

using namespace std::chrono_literals;

namespace Terminal {

using namespace Terminal::Internal;

constexpr std::chrono::milliseconds debounceInterval = 100ms;

TerminalSearch::TerminalSearch(TerminalSurface *surface)
    : m_surface(surface)
{
    m_debounceTimer.setInterval(debounceInterval);
    m_debounceTimer.setSingleShot(true);

    connect(surface, &TerminalSurface::invalidated, this, &TerminalSearch::updateHits);
    connect(&m_debounceTimer, &QTimer::timeout, this, &TerminalSearch::debouncedUpdateHits);

    connect(&TerminalCommands::widgetActions().findNext,
            &QAction::triggered,
            this,
            &TerminalSearch::nextHit);
    connect(&TerminalCommands::widgetActions().findPrevious,
            &QAction::triggered,
            this,
            &TerminalSearch::previousHit);
}

void TerminalSearch::setCurrentSelection(std::optional<SearchHitWithText> selection)
{
    m_currentSelection = selection;
}

void TerminalSearch::setSearchString(const QString &searchString, Core::FindFlags findFlags)
{
    if (m_currentSearchString != searchString || m_findFlags != findFlags) {
        m_currentSearchString = searchString;
        m_findFlags = findFlags;
        updateHits();
    }
}

void TerminalSearch::nextHit()
{
    if (m_hits.isEmpty())
        return;

    m_currentHit = (m_currentHit + 1) % m_hits.size();
    emit currentHitChanged();
}

void TerminalSearch::previousHit()
{
    if (m_hits.isEmpty())
        return;

    m_currentHit = (m_currentHit - 1 + m_hits.size()) % m_hits.size();
    emit currentHitChanged();
}

void TerminalSearch::updateHits()
{
    if (!m_hits.isEmpty()) {
        m_hits.clear();
        m_currentHit = -1;
        emit hitsChanged();
        emit currentHitChanged();
    }

    m_debounceTimer.start();
}

bool isSpace(char32_t a, char32_t b)
{
    if (a == std::numeric_limits<char32_t>::max())
        return std::isspace(b);
    else if (b == std::numeric_limits<char32_t>::max())
        return std::isspace(a);

    return false;
}

QList<SearchHit> TerminalSearch::search()
{
    QList<SearchHit> hits;

    std::function<bool(char32_t, char32_t)> compare;

    if (m_findFlags.testFlag(Core::FindFlag::FindCaseSensitively))
        compare = [](char32_t a, char32_t b) {
            return std::tolower(a) == std::tolower(b) || isSpace(a, b);
        };
    else
        compare = [](char32_t a, char32_t b) { return a == b || isSpace(a, b); };

    if (!m_currentSearchString.isEmpty()) {
        const QList<uint> asUcs4 = m_currentSearchString.toUcs4();
        std::u32string searchString(asUcs4.begin(), asUcs4.end());

        if (m_findFlags.testFlag(Core::FindFlag::FindWholeWords)) {
            searchString.push_back(std::numeric_limits<char32_t>::max());
            searchString.insert(searchString.begin(), std::numeric_limits<char32_t>::max());
        }

        Internal::CellIterator it = m_surface->begin();
        while (it != m_surface->end()) {
            it = std::search(it, m_surface->end(), searchString.begin(), searchString.end(), compare);

            if (it != m_surface->end()) {
                auto hit = SearchHit{it.position(),
                                     static_cast<int>(it.position() + searchString.size())};
                if (m_findFlags.testFlag(Core::FindFlag::FindWholeWords)) {
                    hit.start++;
                    hit.end--;
                }
                hits << hit;
                it += m_currentSearchString.size();
            }
        }
    }
    return hits;
}

QList<SearchHit> TerminalSearch::searchRegex()
{
    QList<SearchHit> hits;

    QString allText;
    allText.reserve(1000);

    // Contains offsets at which there are characters > 2 bytes
    QList<int> adjustTable;

    for (auto it = m_surface->begin(); it != m_surface->end(); ++it) {
        auto chs = QChar::fromUcs4(*it);
        if (chs.size() > 1)
            adjustTable << (allText.size());
        allText += chs;
    }

    QRegularExpression re(m_currentSearchString,
                          m_findFlags.testFlag(Core::FindFlag::FindCaseSensitively)
                              ? QRegularExpression::NoPatternOption
                              : QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = re.globalMatch(allText);
    int adjust = 0;
    auto itAdjust = adjustTable.begin();
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        int s = match.capturedStart();
        int e = match.capturedEnd();

        // Update 'adjust' to account for characters > 2 bytes
        if (itAdjust != adjustTable.end()) {
            while (s > *itAdjust && itAdjust != adjustTable.end()) {
                adjust++;
                itAdjust++;
            }
            s -= adjust;
            while (e > *itAdjust && itAdjust != adjustTable.end()) {
                adjust++;
                itAdjust++;
            }
            e -= adjust;
        }
        hits << SearchHit{s, e};
    }

    return hits;
}

void TerminalSearch::debouncedUpdateHits()
{
    QElapsedTimer t;
    t.start();

    m_currentHit = -1;

    const bool regex = m_findFlags.testFlag(Core::FindFlag::FindRegularExpression);

    QList<SearchHit> hits = regex ? searchRegex() : search();

    if (hits != m_hits) {
        m_currentHit = -1;
        if (m_currentSelection)
            m_currentHit = hits.indexOf(*m_currentSelection);

        if (m_currentHit == -1 && !hits.isEmpty())
            m_currentHit = 0;

        m_hits = hits;
        emit hitsChanged();
        emit currentHitChanged();
        emit changed();
    }
    if (!m_currentSearchString.isEmpty())
        qCDebug(terminalSearchLog) << "Search took" << t.elapsed() << "ms";
}

Core::FindFlags TerminalSearch::supportedFindFlags() const
{
    return Core::FindFlag::FindCaseSensitively | Core::FindFlag::FindBackward
           | Core::FindFlag::FindRegularExpression | Core::FindFlag::FindWholeWords;
}

void TerminalSearch::resetIncrementalSearch()
{
    m_currentSelection.reset();
}

void TerminalSearch::clearHighlights()
{
    setSearchString("", {});
}

QString TerminalSearch::currentFindString() const
{
    if (m_currentSelection)
        return m_currentSelection->text;
    else
        return m_currentSearchString;
}

QString TerminalSearch::completedFindString() const
{
    return {};
}

Core::IFindSupport::Result TerminalSearch::findIncremental(const QString &txt,
                                                           Core::FindFlags findFlags)
{
    if (txt == m_currentSearchString) {
        if (m_debounceTimer.isActive())
            return Result::NotYetFound;
        else if (m_hits.isEmpty())
            return Result::NotFound;
        else
            return Result::Found;
    }

    setSearchString(txt, findFlags);
    return Result::NotYetFound;
}

Core::IFindSupport::Result TerminalSearch::findStep(const QString &txt, Core::FindFlags findFlags)
{
    if (txt == m_currentSearchString) {
        if (m_debounceTimer.isActive())
            return Result::NotYetFound;
        else if (m_hits.isEmpty())
            return Result::NotFound;

        if (findFlags.testFlag(Core::FindFlag::FindBackward))
            previousHit();
        else
            nextHit();

        return Result::Found;
    }

    return findIncremental(txt, findFlags);
}

void TerminalSearch::highlightAll(const QString &txt, Core::FindFlags findFlags)
{
    setSearchString(txt, findFlags);
}

} // namespace Terminal
