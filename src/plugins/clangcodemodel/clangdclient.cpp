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

#include "clangdclient.h"

#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>
#include <languageclient/languageclientinterface.h>

using namespace LanguageClient;

namespace ClangCodeModel {
namespace Internal {

static Q_LOGGING_CATEGORY(clangdLog, "qtc.clangcodemodel.clangd", QtWarningMsg);

static BaseClientInterface *clientInterface(const Utils::FilePath &jsonDbDir)
{
    QString clangdArgs = "--index --background-index --limit-results=0";
    if (!jsonDbDir.isEmpty())
        clangdArgs += " --compile-commands-dir=" + jsonDbDir.toString();
    if (clangdLog().isDebugEnabled())
        clangdArgs += " --log=verbose --pretty";
    const auto interface = new StdIOClientInterface;
    interface->setExecutable(CppTools::codeModelSettings()->clangdFilePath().toString());
    interface->setArguments(clangdArgs);
    return interface;
}

ClangdClient::ClangdClient(ProjectExplorer::Project *project, const Utils::FilePath &jsonDbDir)
    : Client(clientInterface(jsonDbDir))
{
    setName(tr("clangd"));
    LanguageFilter langFilter;
    langFilter.mimeTypes = QStringList{"text/x-chdr", "text/x-c++hdr", "text/x-c++src",
            "text/x-objc++src", "text/x-objcsrc"};
    setSupportedLanguage(langFilter);
    LanguageServerProtocol::ClientCapabilities caps = Client::defaultClientCapabilities();
    caps.clearExperimental();
    caps.clearTextDocument();
    setClientCapabilities(caps);
    setLocatorsEnabled(false);
    setDocumentActionsEnabled(false);
    setProgressTitleForToken("backgroundIndexProgress", tr("Parsing C/C++ Files (clangd)"));
    setCurrentProject(project);
    start();
}

} // namespace Internal
} // namespace ClangCodeModel
