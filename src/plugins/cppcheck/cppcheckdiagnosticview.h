// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/analyzer/detailederrorview.h>

namespace Cppcheck::Internal {

class DiagnosticView : public Debugger::DetailedErrorView
{
public:
    explicit DiagnosticView(QWidget *parent = nullptr);
    ~DiagnosticView() override;

    void goNext() override;
    void goBack() override;

private:
    void openEditorForCurrentIndex();
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

} // Cppcheck::Internal
