/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    enum Direction { Next = 1, Previous = -1 };
    QModelIndex getIndex(const QModelIndex &index, Direction direction) const;
    QModelIndex getTopLevelIndex(const QModelIndex &index, Direction direction) const;

private:
    QAction *m_help = nullptr;

    QAction *m_showFilter = nullptr;
    QAction *m_clearFilter = nullptr;
    QAction *m_filterForCurrentKind = nullptr;
    QAction *m_filterOutCurrentKind = nullptr;

    QAction *m_suppressAction = nullptr;

    QAction *m_separator = nullptr;
    QAction *m_separator2 = nullptr;

    DiagnosticViewStyle *m_style = nullptr;
    DiagnosticViewDelegate *m_delegate = nullptr;
    bool m_ignoreSetSelectedFixItsCount = false;
};

} // namespace Internal
} // namespace ClangTools
