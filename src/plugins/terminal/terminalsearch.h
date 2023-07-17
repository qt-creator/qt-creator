// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <terminal/terminalsurface.h>

#include <solutions/terminal/terminalview.h>

#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/find/textfindconstants.h>

#include <QTimer>

namespace Terminal {

struct SearchHitWithText : TerminalSolution::SearchHit
{
    QString text;
};

class TerminalSearch : public Core::IFindSupport
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

} // namespace Terminal
