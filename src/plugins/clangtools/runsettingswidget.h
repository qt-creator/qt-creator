// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QSpinBox;
QT_END_NAMESPACE


namespace CppEditor { class ClangDiagnosticConfigsSelectionWidget; }

namespace ClangTools::Internal {

class RunSettings;

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
    CppEditor::ClangDiagnosticConfigsSelectionWidget *m_diagnosticWidget;
    QCheckBox *m_preferConfigFile;
    QCheckBox *m_buildBeforeAnalysis;
    QCheckBox *m_analyzeOpenFiles;
    QSpinBox *m_parallelJobsSpinBox;
};

} // ClangTools::Internal
