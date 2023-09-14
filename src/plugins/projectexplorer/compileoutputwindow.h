// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildstep.h"

#include "projectexplorersettings.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/ioutputpane.h>

#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Core { class OutputWindow; }
namespace Utils { class OutputFormatter; }

namespace ProjectExplorer {
class Task;

namespace Internal {
class ShowOutputTaskHandler;
class CompileOutputTextEdit;

class CompileOutputSettings final : public Utils::AspectContainer
{
public:
    CompileOutputSettings();

    Utils::BoolAspect popUp{this};
    Utils::BoolAspect wrapOutput{this};
    Utils::IntegerAspect maxCharCount{this};
};

CompileOutputSettings &compileOutputSettings();

class CompileOutputWindow final : public Core::IOutputPane
{
    Q_OBJECT

public:
    explicit CompileOutputWindow(QAction *cancelBuildAction);
    ~CompileOutputWindow() override;

    QWidget *outputWidget(QWidget *) override;
    QList<QWidget *> toolBarWidgets() const override;
    void clearContents() override;
    bool canFocus() const override;
    bool hasFocus() const override;
    void setFocus() override;

    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;
    bool canNavigate() const override;

    void appendText(const QString &text, BuildStep::OutputFormat format);

    void registerPositionOf(const Task &task, int linkedOutputLines, int skipLines, int offset = 0);

    void flush();
    void reset();

    Utils::OutputFormatter *outputFormatter() const;

private:
    void updateFilter() override;
    const QList<Core::OutputWindow *> outputWindows() const override { return {m_outputWindow}; }

    void updateFromSettings();
    Core::OutputWindow *m_outputWindow;
    ShowOutputTaskHandler *m_handler;
    QToolButton *m_cancelBuildButton;
    QToolButton * const m_settingsButton;
};

} // namespace Internal
} // namespace ProjectExplorer
