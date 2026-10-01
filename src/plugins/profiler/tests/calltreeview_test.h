// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QTreeView>
#include <QWidget>

namespace Profiler::Internal {

class CallTreePanelProbeWidget final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool panelwidget READ isPanelWidget)

public:
    bool isPanelWidget() const
    {
        ++panelReads;
        return false;
    }

    mutable int panelReads = 0;
};

class CallTreePanelProbeView final : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(bool panelwidget READ isPanelWidget)

public:
    bool isPanelWidget() const
    {
        ++panelReads;
        return false;
    }

    mutable int panelReads = 0;
};

class CallTreeViewTest final : public QObject
{
    Q_OBJECT

private slots:
    void testHorizontalScrolling_data();
    void testHorizontalScrolling();
    void testNavigationKeepsHorizontalPosition_data();
    void testNavigationKeepsHorizontalPosition();
    void testBranchPainting_data();
    void testBranchPainting();
    void testCellPainting_data();
    void testCellPainting();
    void testWideColumnCache_data();
    void testWideColumnCache();
    void testBranchPrimitives_data();
    void testBranchPrimitives();
    void testRowBackgrounds_data();
    void testRowBackgrounds();
};

} // namespace Profiler::Internal
