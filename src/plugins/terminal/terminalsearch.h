// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "terminalsurface.h"

#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/find/textfindconstants.h>

#include <QTimer>

namespace Terminal {

struct SearchHit
{
    int start{-1};
    int end{-1};

    bool operator!=(const SearchHit &other) const
    {
        return start != other.start || end != other.end;
    }
    bool operator==(const SearchHit &other) const { return !operator!=(other); }
};

struct SearchHitWithText : SearchHit
{
    QString text;
};

class TerminalSearch : public Core::IFindSupport
{
    Q_OBJECT
public:
    TerminalSearch(Internal::TerminalSurface *surface);

    void setCurrentSelection(std::optional<SearchHitWithText> selection);
    void setSearchString(const QString &searchString, Utils::FindFlags findFlags);
    void nextHit();
    void previousHit();

    const QList<SearchHit> &hits() const { return m_hits; }
    SearchHit currentHit() const
    {
        return m_currentHit >= 0 ? m_hits.at(m_currentHit) : SearchHit{};
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
    QList<SearchHit> search();
    QList<SearchHit> searchRegex();

private:
    std::optional<SearchHitWithText> m_currentSelection;
    QString m_currentSearchString;
    Utils::FindFlags m_findFlags;
    Internal::TerminalSurface *m_surface;

    int m_currentHit{-1};
    QList<SearchHit> m_hits;
    QTimer m_debounceTimer;
};

} // namespace Terminal
