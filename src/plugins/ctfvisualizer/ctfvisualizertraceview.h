// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QQuickWidget>

namespace CtfVisualizer::Internal {

class CtfVisualizerTool;

class CtfVisualizerTraceView : public QQuickWidget
{
    Q_OBJECT

public:
    CtfVisualizerTraceView(QWidget *parent, CtfVisualizerTool *tool);

   ~CtfVisualizerTraceView();

    void selectByTypeId(int typeId);
};

} // namespace CtfVisualizer::Internal

