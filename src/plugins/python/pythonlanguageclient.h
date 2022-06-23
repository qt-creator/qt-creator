/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <languageclient/client.h>
#include <languageclient/languageclientsettings.h>

namespace Core { class IDocument; }
namespace ProjectExplorer { class ExtraCompiler; }
namespace TextEditor { class TextDocument; }

namespace Python {
namespace Internal {

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

    static void updateEditorInfoBars(const Utils::FilePath &python,
                                     LanguageClient::Client *client);
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

} // namespace Internal
} // namespace Python
