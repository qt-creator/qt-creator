// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perftimelinemodel.h"

#include <utils/basetreeview.h>

#include <QWidget>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerStatisticsMainModel;
class PerfProfilerTool;
class StatisticsView;

class PerfProfilerStatisticsView : public QWidget
{
    Q_OBJECT
public:
    PerfProfilerStatisticsView(QWidget *parent, PerfProfilerTool *tool);
    bool focusedTableHasValidSelection() const;

    void selectByTypeId(int symbol);
    void copyFocusedTableToClipboard() const;
    void copyFocusedSelectionToClipboard() const;

signals:
    void gotoSourceLocation(QString file, int line, int column);
    void typeSelected(int symbol);

private:
    StatisticsView *m_mainView;
    StatisticsView *m_parentsView;
    StatisticsView *m_childrenView;
};

} // namespace Internal
} // namespace PerfProfiler
