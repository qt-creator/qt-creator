// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/analyzer/detailederrorview.h>

namespace ClangTools {
namespace Internal {

class DiagnosticViewStyle;
class DiagnosticViewDelegate;

class DiagnosticView : public Debugger::DetailedErrorView
{
    Q_OBJECT

public:
    DiagnosticView(QWidget *parent = nullptr);
    ~DiagnosticView() override;

    void scheduleAllFixits(bool schedule);

signals:
    void showHelp();

    void showFilter();
    void clearFilter();
    void filterForCurrentKind();
    void filterOutCurrentKind();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    QList<QAction *> customActions() const override;
    void goNext() override;
    void goBack() override;

    void openEditorForCurrentIndex();
    void suppressCurrentDiagnostic();
    void disableCheckForCurrentDiagnostic();
    enum Direction { Next = 1, Previous = -1 };
    QModelIndex getIndex(const QModelIndex &index, Direction direction) const;
    QModelIndex getTopLevelIndex(const QModelIndex &index, Direction direction) const;

    bool disableChecksEnabled() const;

    QAction *m_help = nullptr;

    QAction *m_showFilter = nullptr;
    QAction *m_clearFilter = nullptr;
    QAction *m_filterForCurrentKind = nullptr;
    QAction *m_filterOutCurrentKind = nullptr;

    QAction *m_suppressAction = nullptr;
    QAction *m_disableChecksAction = nullptr;

    QAction *m_separator = nullptr;
    QAction *m_separator2 = nullptr;

    DiagnosticViewStyle *m_style = nullptr;
    DiagnosticViewDelegate *m_delegate = nullptr;
};

} // namespace Internal
} // namespace ClangTools
