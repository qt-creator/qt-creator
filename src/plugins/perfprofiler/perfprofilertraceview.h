// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perftimelinemodelmanager.h"

#include <QQuickWidget>
#include <QWidget>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerTool;

class PerfProfilerTraceView : public QQuickWidget
{
    Q_OBJECT

public:
    PerfProfilerTraceView(QWidget *parent, PerfProfilerTool *tool);
    bool isUsable() const;
    void selectByTypeId(int typeId);
    void clear();

signals:
    void gotoSourceLocation(QString file, int line, int column);
    void typeSelected(int typeId);

private:
    void updateCursorPosition();
};

} // namespace Internal
} // namespace PerfProfiler
