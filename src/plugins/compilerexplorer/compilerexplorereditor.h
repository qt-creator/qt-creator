// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "api/compile.h"
#include "compilerexplorersettings.h"

#include <coreplugin/terminal/searchableterminal.h>

#include <solutions/spinner/spinner.h>

#include <texteditor/texteditor.h>

#include <utils/fancymainwindow.h>

#include <QFutureWatcher>
#include <QMainWindow>
#include <QSplitter>
#include <QUndoStack>

#include <memory>

namespace CompilerExplorer {

class JsonSettingsDocument;
class SourceEditorWidget;
class AsmDocument;

class CodeEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT
public:
    CodeEditorWidget(const std::shared_ptr<SourceSettings> &settings, QUndoStack *undoStack);

    void updateHighlighter();

    void undo() override { m_undoStack->undo(); }
    void redo() override { m_undoStack->redo(); }

    void focusInEvent(QFocusEvent *event) override
    {
        TextEditorWidget::focusInEvent(event);
        emit gotFocus();
    }

signals:
    void gotFocus();

private:
    std::shared_ptr<SourceSettings> m_settings;
    QUndoStack *m_undoStack;
};

class AsmDocument : public TextEditor::TextDocument
{
public:
    using TextEditor::TextDocument::TextDocument;

    QList<QTextEdit::ExtraSelection> setCompileResult(const Api::CompileResult &compileResult);
    QList<Api::CompileResult::AssemblyLine> &asmLines() { return m_assemblyLines; }

private:
    QList<Api::CompileResult::AssemblyLine> m_assemblyLines;
    QList<TextEditor::TextMark *> m_marks;
};

class AsmEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT

public:
    AsmEditorWidget(QUndoStack *undoStack);

    void focusInEvent(QFocusEvent *event) override
    {
        TextEditorWidget::focusInEvent(event);
        emit gotFocus();
    }

    void findLinkAt(const QTextCursor &,
                    const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;

    void undo() override { m_undoStack->undo(); }
    void redo() override { m_undoStack->redo(); }

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void gotFocus();
    void hoveredLineChanged(const std::optional<Api::CompileResult::AssemblyLine> &assemblyLine);

private:
    QUndoStack *m_undoStack;
    std::optional<Api::CompileResult::AssemblyLine> m_currentlyHoveredLine;
};

class JsonSettingsDocument : public Core::IDocument
{
    Q_OBJECT
public:
    JsonSettingsDocument(QUndoStack *undoStack);

    OpenResult open(QString *errorString,
                    const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;

    bool saveImpl(QString *errorString,
                  const Utils::FilePath &filePath = Utils::FilePath(),
                  bool autoSave = false) override;

    bool setContents(const QByteArray &contents) override;

    QString fallbackSaveAsFileName() const override;

    bool shouldAutoSave() const override { return !filePath().isEmpty(); }
    bool isModified() const override;
    bool isSaveAsAllowed() const override { return true; }

    CompilerExplorerSettings *settings() { return &m_ceSettings; }

    void setWindowStateCallback(std::function<Utils::Store()> callback)
    {
        m_windowStateCallback = callback;
    }

signals:
    void settingsChanged();

private:
    mutable CompilerExplorerSettings m_ceSettings;
    std::function<Utils::Store()> m_windowStateCallback;
    QUndoStack *m_undoStack;
};

class SourceEditorWidget : public QWidget
{
    Q_OBJECT
public:
    SourceEditorWidget(const std::shared_ptr<SourceSettings> &settings, QUndoStack *undoStack);

    QString sourceCode();
    SourceSettings *sourceSettings() { return m_sourceSettings.get(); }

    void focusInEvent(QFocusEvent *) override { emit gotFocus(); }

    TextEditor::TextEditorWidget *textEditor() { return m_codeEditor; }

public slots:
    void markSourceLocation(const std::optional<Api::CompileResult::AssemblyLine> &assemblyLine);

signals:
    void sourceCodeChanged();
    void remove();
    void gotFocus();

private:
    CodeEditorWidget *m_codeEditor{nullptr};
    std::shared_ptr<SourceSettings> m_sourceSettings;
};

class CompilerWidget : public QWidget
{
    Q_OBJECT
public:
    CompilerWidget(const std::shared_ptr<SourceSettings> &sourceSettings,
                   const std::shared_ptr<CompilerSettings> &compilerSettings,
                   QUndoStack *undoStack);

    Core::SearchableTerminal *createTerminal();

    void compile(const QString &source);

    std::shared_ptr<SourceSettings> m_sourceSettings;
    std::shared_ptr<CompilerSettings> m_compilerSettings;

    void focusInEvent(QFocusEvent *) override { emit gotFocus(); }

    TextEditor::TextEditorWidget *textEditor() { return m_asmEditor; }

private:
    void doCompile();

signals:
    void remove();
    void gotFocus();
    void hoveredLineChanged(const std::optional<Api::CompileResult::AssemblyLine> &assemblyLine);

private:
    AsmEditorWidget *m_asmEditor{nullptr};
    Core::SearchableTerminal *m_resultTerminal{nullptr};

    SpinnerSolution::Spinner *m_spinner{nullptr};
    QSharedPointer<AsmDocument> m_asmDocument;

    std::unique_ptr<QFutureWatcher<Api::CompileResult>> m_compileWatcher;

    QString m_source;
    QTimer *m_delayTimer{nullptr};
};

class HelperWidget : public QWidget
{
    Q_OBJECT
public:
    HelperWidget();

protected:
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void addSource();
};

class EditorWidget : public Utils::FancyMainWindow
{
    Q_OBJECT
public:
    EditorWidget(const std::shared_ptr<JsonSettingsDocument> &document,
                 QUndoStack *undoStack,
                 QWidget *parent = nullptr);
    ~EditorWidget() override;

    TextEditor::TextEditorWidget *focusedEditorWidget() const;

signals:
    void sourceCodeChanged();
    void gotFocus();

protected:
    void focusInEvent(QFocusEvent *event) override;

    void setupHelpWidget();
    QWidget *createHelpWidget() const;

    CompilerWidget *addCompiler(const std::shared_ptr<SourceSettings> &sourceSettings,
                                const std::shared_ptr<CompilerSettings> &compilerSettings,
                                int idx);

    void addSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings);
    void removeSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings);

    void recreateEditors();

    QVariantMap windowStateCallback();

private:
    std::shared_ptr<JsonSettingsDocument> m_document;
    QUndoStack *m_undoStack;

    QList<QDockWidget *> m_compilerWidgets;
    QList<QDockWidget *> m_sourceWidgets;
};

class Editor : public Core::IEditor
{
public:
    Editor();
    ~Editor();

    Core::IDocument *document() const override { return m_document.get(); }
    QWidget *toolBar() override;

    std::shared_ptr<JsonSettingsDocument> m_document;
    QUndoStack m_undoStack;
    std::unique_ptr<QToolBar> m_toolBar;
};

class EditorFactory : public Core::IEditorFactory
{
public:
    EditorFactory();

private:
    QAction m_undoAction;
    QAction m_redoAction;
};

} // namespace CompilerExplorer
