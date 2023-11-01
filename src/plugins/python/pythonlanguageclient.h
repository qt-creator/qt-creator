// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <languageclient/client.h>
#include <languageclient/languageclientsettings.h>

namespace Core { class IDocument; }
namespace ProjectExplorer { class ExtraCompiler; }
namespace TextEditor { class TextDocument; }

namespace Python::Internal {

class PySideUicExtraCompiler;
class PythonLanguageServerState;
class PyLSInterface;

class PyLSClient : public LanguageClient::Client
{
    Q_OBJECT
public:
    explicit PyLSClient(PyLSInterface *interface);
    ~PyLSClient();

    void openDocument(TextEditor::TextDocument *document) override;
    void projectClosed(ProjectExplorer::Project *project) override;

    void updateExtraCompilers(ProjectExplorer::Project *project,
                              const QList<PySideUicExtraCompiler *> &extraCompilers);

    static PyLSClient *clientForPython(const Utils::FilePath &python);
    void updateConfiguration();

private:
    void updateExtraCompilerContents(ProjectExplorer::ExtraCompiler *compiler,
                                     const Utils::FilePath &file);
    void closeExtraDoc(const Utils::FilePath &file);
    void closeExtraCompiler(ProjectExplorer::ExtraCompiler *compiler);

    Utils::FilePaths m_extraWorkspaceDirs;
    Utils::FilePath m_extraCompilerOutputDir;

    QHash<ProjectExplorer::Project *, QList<ProjectExplorer::ExtraCompiler *>> m_extraCompilers;
};

class PyLSConfigureAssistant : public QObject
{
    Q_OBJECT
public:
    static PyLSConfigureAssistant *instance();

    static void openDocumentWithPython(const Utils::FilePath &python,
                                       TextEditor::TextDocument *document);

private:
    explicit PyLSConfigureAssistant(QObject *parent);

    void handlePyLSState(const Utils::FilePath &python,
                         const PythonLanguageServerState &state,
                         TextEditor::TextDocument *document);
    void resetEditorInfoBar(TextEditor::TextDocument *document);
    void installPythonLanguageServer(const Utils::FilePath &python,
                                     QPointer<TextEditor::TextDocument> document);

    QHash<Utils::FilePath, QList<TextEditor::TextDocument *>> m_infoBarEntries;
};

} // Python::Internal
