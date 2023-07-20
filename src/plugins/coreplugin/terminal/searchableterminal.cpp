// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchableterminal.h"

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <chrono>

Q_LOGGING_CATEGORY(terminalSearchLog, "qtc.terminal.search", QtWarningMsg)

using namespace Utils;

using namespace std::chrono_literals;

namespace Core {

constexpr std::chrono::milliseconds debounceInterval = 100ms;

TerminalSearch::TerminalSearch(TerminalSolution::TerminalSurface *surface)
    : m_surface(surface)
{
    m_debounceTimer.setInterval(debounceInterval);
    m_debounceTimer.setSingleShot(true);

    connect(surface,
            &TerminalSolution::TerminalSurface::invalidated,
            this,
            &TerminalSearch::updateHits);
    connect(&m_debounceTimer, &QTimer::timeout, this, &TerminalSearch::debouncedUpdateHits);
}

void TerminalSearch::setCurrentSelection(std::optional<SearchHitWithText> selection)
{
    m_currentSelection = selection;
}

void TerminalSearch::setSearchString(const QString &searchString, FindFlags findFlags)
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

QList<TerminalSolution::SearchHit> TerminalSearch::search()
{
    QList<TerminalSolution::SearchHit> hits;

    std::function<bool(char32_t, char32_t)> compare;

    if (m_findFlags.testFlag(FindFlag::FindCaseSensitively)) {
        compare = [](char32_t a, char32_t b) { return a == b || isSpace(a, b); };
    } else {
        compare = [](char32_t a, char32_t b) {
            return std::tolower(a) == std::tolower(b) || isSpace(a, b);
        };
    }

    if (!m_currentSearchString.isEmpty()) {
        const QList<uint> asUcs4 = m_currentSearchString.toUcs4();
        std::u32string searchString(asUcs4.begin(), asUcs4.end());

        if (m_findFlags.testFlag(FindFlag::FindWholeWords)) {
            searchString.push_back(std::numeric_limits<char32_t>::max());
            searchString.insert(searchString.begin(), std::numeric_limits<char32_t>::max());
        }

        TerminalSolution::CellIterator it = m_surface->begin();
        while (it != m_surface->end()) {
            it = std::search(it, m_surface->end(), searchString.begin(), searchString.end(), compare);

            if (it != m_surface->end()) {
                auto hit = TerminalSolution::SearchHit{it.position(),
                                                       static_cast<int>(it.position()
                                                                        + searchString.size())};
                if (m_findFlags.testFlag(FindFlag::FindWholeWords)) {
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

QList<TerminalSolution::SearchHit> TerminalSearch::searchRegex()
{
    QList<TerminalSolution::SearchHit> hits;

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
                          m_findFlags.testFlag(FindFlag::FindCaseSensitively)
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
        hits << TerminalSolution::SearchHit{s, e};
    }

    return hits;
}

void TerminalSearch::debouncedUpdateHits()
{
    QElapsedTimer t;
    t.start();

    m_currentHit = -1;

    const bool regex = m_findFlags.testFlag(FindFlag::FindRegularExpression);

    QList<TerminalSolution::SearchHit> hits = regex ? searchRegex() : search();

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

FindFlags TerminalSearch::supportedFindFlags() const
{
    return FindFlag::FindCaseSensitively | FindFlag::FindBackward | FindFlag::FindRegularExpression
           | FindFlag::FindWholeWords;
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

Core::IFindSupport::Result TerminalSearch::findIncremental(const QString &txt, FindFlags findFlags)
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

Core::IFindSupport::Result TerminalSearch::findStep(const QString &txt, FindFlags findFlags)
{
    if (txt == m_currentSearchString) {
        if (m_debounceTimer.isActive())
            return Result::NotYetFound;
        else if (m_hits.isEmpty())
            return Result::NotFound;

        if (findFlags.testFlag(FindFlag::FindBackward))
            previousHit();
        else
            nextHit();

        return Result::Found;
    }

    return findIncremental(txt, findFlags);
}

void TerminalSearch::highlightAll(const QString &txt, FindFlags findFlags)
{
    setSearchString(txt, findFlags);
}

SearchableTerminal::SearchableTerminal(QWidget *parent)
    : TerminalSolution::TerminalView(parent)
{
    m_aggregate = new Aggregation::Aggregate(this);
    m_aggregate->add(this);

    surfaceChanged();
}

SearchableTerminal::~SearchableTerminal() = default;

void SearchableTerminal::surfaceChanged()
{
    TerminalView::surfaceChanged();

    m_search = TerminalSearchPtr(new TerminalSearch(surface()), [this](TerminalSearch *p) {
        m_aggregate->remove(p);
        delete p;
    });

    m_aggregate->add(m_search.get());

    connect(m_search.get(), &TerminalSearch::hitsChanged, this, &SearchableTerminal::updateViewport);
    connect(m_search.get(), &TerminalSearch::currentHitChanged, this, [this] {
        TerminalSolution::SearchHit hit = m_search->currentHit();
        if (hit.start >= 0) {
            setSelection(Selection{hit.start, hit.end, true}, hit != m_lastSelectedHit);
            m_lastSelectedHit = hit;
        }
    });
}

void SearchableTerminal::selectionChanged(const std::optional<Selection> &newSelection)
{
    TerminalView::selectionChanged(newSelection);

    if (selection() && selection()->final) {
        QString text = textFromSelection();

        if (m_search) {
            m_search->setCurrentSelection(
                SearchHitWithText{{newSelection->start, newSelection->end}, text});
        }
    }
}

const QList<TerminalSolution::SearchHit> &SearchableTerminal::searchHits() const
{
    if (!m_search)
        return TerminalSolution::TerminalView::searchHits();
    return m_search->hits();
}

} // namespace Core
