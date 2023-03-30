// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QToolBar>

#include <memory>

namespace QmlDesigner {

class ToolBar
{

public:
    static std::unique_ptr<QToolBar> create();
    static std::unique_ptr<QWidget> createStatusBar();
    static bool isVisible();
};

} // namespace QmlDesigner
