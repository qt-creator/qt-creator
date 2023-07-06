// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "api/compile.h"
#include "compilerexplorersettings.h"

#include <coreplugin/terminal/searchableterminal.h>

#include <solutions/spinner/spinner.h>

#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>

#include <utils/fancymainwindow.h>

#include <QFutureWatcher>
#include <QMainWindow>
#include <QSplitter>

#include <memory>

namespace CppEditor {
class CppEditorWidget;
}

namespace CompilerExplorer {

class CodeEditorWidget;

class CompilerWidget : public QWidget
{
    Q_OBJECT
public:
    CompilerWidget();

    Core::SearchableTerminal *createTerminal();

    void compile(const QString &source);

private:
    void doCompile();

private:
    TextEditor::TextEditorWidget *m_asmEditor{nullptr};
    Core::SearchableTerminal *m_resultTerminal{nullptr};

    SpinnerSolution::Spinner *m_spinner{nullptr};
    QSharedPointer<TextEditor::TextDocument> m_asmDocument;

    std::unique_ptr<QFutureWatcher<Api::CompileResult>> m_compileWatcher;

    CompilerExplorer::CompilerSettings m_compilerSettings;

    QString m_source;
    QTimer *m_delayTimer{nullptr};
    QList<TextEditor::TextMark *> m_marks;
};

class EditorWidget : public Utils::FancyMainWindow
{
    Q_OBJECT
public:
    EditorWidget(QSharedPointer<TextEditor::TextDocument> document = nullptr, QWidget *parent = nullptr);

    void addCompiler();

protected:
    Core::SearchableTerminal *createTerminal();

    void onLanguageChanged();

signals:
    void sourceCodeChanged();

private:
    CodeEditorWidget *m_codeEditor;
    CompilerExplorer::Settings m_currentSettings;
    QSplitter *m_mainSplitter;
    int m_compilerCount{0};

    Core::IContext *m_context;
};

class EditorFactory : public Core::IEditorFactory
{
public:
    EditorFactory();

private:
    TextEditor::TextEditorActionHandler m_actionHandler;
};

} // namespace CompilerExplorer
