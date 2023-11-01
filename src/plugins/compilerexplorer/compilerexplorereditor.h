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
#include <QUndoStack>

#include <memory>

namespace CppEditor {
class CppEditorWidget;
}

namespace CompilerExplorer {

class JsonSettingsDocument;
class SourceEditorWidget;

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

    void undo() override { m_undoStack->undo(); }
    void redo() override { m_undoStack->redo(); }

signals:
    void gotFocus();

private:
    QUndoStack *m_undoStack;
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

private:
    AsmEditorWidget *m_asmEditor{nullptr};
    Core::SearchableTerminal *m_resultTerminal{nullptr};

    SpinnerSolution::Spinner *m_spinner{nullptr};
    QSharedPointer<TextEditor::TextDocument> m_asmDocument;

    std::unique_ptr<QFutureWatcher<Api::CompileResult>> m_compileWatcher;

    QString m_source;
    QTimer *m_delayTimer{nullptr};
    QList<TextEditor::TextMark *> m_marks;
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
    EditorWidget(const QSharedPointer<JsonSettingsDocument> &document,
                 QUndoStack *undoStack,
                 TextEditor::TextEditorActionHandler &actionHandler,
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

    void addCompiler(const std::shared_ptr<SourceSettings> &sourceSettings,
                     const std::shared_ptr<CompilerSettings> &compilerSettings,
                     int idx,
                     QDockWidget *parentDockWidget);

    void addSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings);
    void removeSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings);

    void recreateEditors();

    QVariantMap windowStateCallback();

private:
    QSharedPointer<JsonSettingsDocument> m_document;
    QUndoStack *m_undoStack;
    TextEditor::TextEditorActionHandler &m_actionHandler;

    QList<QDockWidget *> m_compilerWidgets;
    QList<QDockWidget *> m_sourceWidgets;
};

class Editor : public Core::IEditor
{
public:
    Editor(TextEditor::TextEditorActionHandler &actionHandler);
    ~Editor();

    Core::IDocument *document() const override { return m_document.data(); }
    QWidget *toolBar() override;

    QSharedPointer<JsonSettingsDocument> m_document;
    QUndoStack m_undoStack;
    std::unique_ptr<QToolBar> m_toolBar;
};

class EditorFactory : public Core::IEditorFactory
{
public:
    EditorFactory();

private:
    TextEditor::TextEditorActionHandler m_actionHandler;

    QAction m_undoAction;
    QAction m_redoAction;
};

} // namespace CompilerExplorer
