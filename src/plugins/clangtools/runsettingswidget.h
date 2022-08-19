// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>

#include <QWidget>

namespace CppEditor {
class ClangDiagnosticConfigsSelectionWidget;
}

namespace ClangTools {
namespace Internal {

class RunSettings;

namespace Ui {
class RunSettingsWidget;
}

class RunSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RunSettingsWidget(QWidget *parent = nullptr);
    ~RunSettingsWidget();

    CppEditor::ClangDiagnosticConfigsSelectionWidget *diagnosticSelectionWidget();

    void fromSettings(const RunSettings &s);
    RunSettings toSettings() const;

signals:
    void changed();

private:
    Ui::RunSettingsWidget *m_ui;
};

} // namespace Internal
} // namespace ClangTools
