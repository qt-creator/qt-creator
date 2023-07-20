// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"
#include "../find/ifindsupport.h"

#include <aggregation/aggregate.h>

#include <solutions/terminal/terminalview.h>

namespace Core {

struct SearchHitWithText : TerminalSolution::SearchHit
{
    QString text;
};

class TerminalSearch : public IFindSupport
{
    Q_OBJECT
public:
    TerminalSearch(TerminalSolution::TerminalSurface *surface);

    void setCurrentSelection(std::optional<SearchHitWithText> selection);
    void setSearchString(const QString &searchString, Utils::FindFlags findFlags);
    void nextHit();
    void previousHit();

    const QList<TerminalSolution::SearchHit> &hits() const { return m_hits; }
    TerminalSolution::SearchHit currentHit() const
    {
        return m_currentHit >= 0 ? m_hits.at(m_currentHit) : TerminalSolution::SearchHit{};
    }

public:
    bool supportsReplace() const override { return false; }
    Utils::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override;
    void clearHighlights() override;
    QString currentFindString() const override;
    QString completedFindString() const override;
    Result findIncremental(const QString &txt, Utils::FindFlags findFlags) override;
    Result findStep(const QString &txt, Utils::FindFlags findFlags) override;

    void highlightAll(const QString &, Utils::FindFlags) override;

signals:
    void hitsChanged();
    void currentHitChanged();

protected:
    void updateHits();
    void debouncedUpdateHits();
    QList<TerminalSolution::SearchHit> search();
    QList<TerminalSolution::SearchHit> searchRegex();

private:
    std::optional<SearchHitWithText> m_currentSelection;
    QString m_currentSearchString;
    Utils::FindFlags m_findFlags;
    TerminalSolution::TerminalSurface *m_surface;

    int m_currentHit{-1};
    QList<TerminalSolution::SearchHit> m_hits;
    QTimer m_debounceTimer;
};

class CORE_EXPORT SearchableTerminal : public TerminalSolution::TerminalView
{
public:
    SearchableTerminal(QWidget *parent = nullptr);
    ~SearchableTerminal() override;

protected:
    void surfaceChanged() override;
    const QList<TerminalSolution::SearchHit> &searchHits() const override;
    void selectionChanged(const std::optional<Selection> &newSelection) override;

private:
    using TerminalSearchPtr = std::unique_ptr<TerminalSearch, std::function<void(TerminalSearch *)>>;
    TerminalSearchPtr m_search;
    TerminalSolution::SearchHit m_lastSelectedHit{};

    Aggregation::Aggregate *m_aggregate{nullptr};
};

} // namespace Core
