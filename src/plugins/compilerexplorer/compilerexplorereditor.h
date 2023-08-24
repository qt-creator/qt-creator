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

class JsonSettingsDocument;
class SourceEditorWidget;
class CodeEditorWidget;

class JsonSettingsDocument : public Core::IDocument
{
    Q_OBJECT
public:
    JsonSettingsDocument();
    ~JsonSettingsDocument() override;

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

    void setWindowStateCallback(std::function<QVariantMap()> callback)
    {
        m_windowStateCallback = callback;
    }

signals:
    void settingsChanged();

private:
    mutable CompilerExplorerSettings m_ceSettings;
    std::function<QVariantMap()> m_windowStateCallback;
};

class SourceEditorWidget : public QWidget
{
    Q_OBJECT
public:
    SourceEditorWidget(const std::shared_ptr<SourceSettings> &settings);

    QString sourceCode();

    std::shared_ptr<SourceSettings> m_sourceSettings;
signals:
    void sourceCodeChanged();

private:
    CodeEditorWidget *m_codeEditor{nullptr};
};

class CompilerWidget : public QWidget
{
    Q_OBJECT
public:
    CompilerWidget(const std::shared_ptr<SourceSettings> &sourceSettings,
                   const std::shared_ptr<CompilerSettings> &compilerSettings);

    ~CompilerWidget();

    Core::SearchableTerminal *createTerminal();

    void compile(const QString &source);

    std::shared_ptr<SourceSettings> m_sourceSettings;
    std::shared_ptr<CompilerSettings> m_compilerSettings;

private:
    void doCompile();

private:
    TextEditor::TextEditorWidget *m_asmEditor{nullptr};
    Core::SearchableTerminal *m_resultTerminal{nullptr};

    SpinnerSolution::Spinner *m_spinner{nullptr};
    QSharedPointer<TextEditor::TextDocument> m_asmDocument;

    std::unique_ptr<QFutureWatcher<Api::CompileResult>> m_compileWatcher;

    QString m_source;
    QTimer *m_delayTimer{nullptr};
    QList<TextEditor::TextMark *> m_marks;
};

class EditorWidget : public Utils::FancyMainWindow
{
    Q_OBJECT
public:
    EditorWidget(const QSharedPointer<JsonSettingsDocument> &document, QWidget *parent = nullptr);
    ~EditorWidget() override;

signals:
    void sourceCodeChanged();

private:
    QSplitter *m_mainSplitter;
    int m_compilerCount{0};
    QSharedPointer<JsonSettingsDocument> m_document;

    Core::IContext *m_context;

    QList<QDockWidget *> m_compilerWidgets;
    QList<QDockWidget *> m_sourceWidgets;
};

class EditorFactory : public Core::IEditorFactory
{
public:
    EditorFactory();

private:
    TextEditor::TextEditorActionHandler m_actionHandler;
};

} // namespace CompilerExplorer
