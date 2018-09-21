/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "compilationdatabaseproject.h"

#include "compilationdatabaseconstants.h"

#include <coreplugin/icontext.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cppprojectupdater.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <texteditor/textdocument.h>
#include <utils/runextensions.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QRegularExpression>

namespace CompilationDatabaseProjectManager {
namespace Internal {

class DBProjectNode : public ProjectExplorer::ProjectNode
{
public:
    explicit DBProjectNode(const Utils::FileName &projectFilePath)
        : ProjectExplorer::ProjectNode(projectFilePath)
    {}
};

static QStringList splitCommandLine(const QString &line)
{
    QStringList result;
    bool insideQuotes = false;

    for (const QString &part : line.split(QRegularExpression("\""))) {
        if (insideQuotes) {
            const QString quotedPart = "\"" + part + "\"";
            if (result.last().endsWith("="))
                result.last().append(quotedPart);
            else
                result.append(quotedPart);
        } else { // If 's' is outside quotes ...
            result.append(part.split(QRegularExpression("\\s+"), QString::SkipEmptyParts));
        }
        insideQuotes = !insideQuotes;
    }
    return result;
}

static QString updatedPathFlag(const QString &pathStr, const QString &workingDir,
                               const QString &originalFlag)
{
    QString result = pathStr;
    if (!QDir(pathStr).exists()
            && QDir(workingDir + "/" + pathStr).exists()) {
        result = workingDir + "/" + pathStr;
    }

    if (originalFlag.startsWith("-I"))
        return "-I" + result;

    if (originalFlag.startsWith("-isystem"))
        return "-isystem" + result;

    return result;
}

static QStringList filteredFlags(const QStringList &flags, const QString &fileName,
                                 const QString &workingDir)
{
    QStringList filtered;
    // Skip compiler call if present.
    bool skipNext = !flags.first().startsWith('-');
    bool includePath = false;

    for (const QString &flag : flags) {
        if (skipNext) {
            skipNext = false;
            continue;
        }

        QString pathStr;
        if (includePath) {
            includePath = false;
            pathStr = flag;
        } else if ((flag.startsWith("-I") || flag.startsWith("-isystem"))
                   && flag != "-I" && flag != "-isystem") {
            pathStr = flag.mid(flag.startsWith("-I") ? 2 : 8);
        }

        if (!pathStr.isEmpty()) {
            filtered.push_back(updatedPathFlag(pathStr, workingDir, flag));
            continue;
        }

        if (flag == "-c" || flag == "-pedantic" || flag.startsWith("/") || flag.startsWith("-m")
                || flag.startsWith("-O") || flag.startsWith("-W") || flag.startsWith("-w")
                || flag.startsWith("--sysroot=")) {
            continue;
        }

        if (flag == "-target" || flag == "-triple" || flag == "-isysroot" || flag == "-isystem"
                || flag == "--sysroot") {
            skipNext = true;
            continue;
        }

        if (flag.endsWith(fileName))
            continue;

        if (flag == "-I" || flag == "-isystem")
            includePath = true;

        filtered.push_back(flag);
    }

    return filtered;
}

static CppTools::RawProjectPart makeRawProjectPart(const Utils::FileName &projectFile,
                                                   const QJsonObject &object)
{
    const Utils::FileName fileName = Utils::FileName::fromString(
                QDir::fromNativeSeparators(object["file"].toString()));
    QStringList flags;
    const QJsonArray arguments = object["arguments"].toArray();
    if (arguments.isEmpty()) {
        flags = splitCommandLine(object["command"].toString());
    } else {
        for (const QJsonValue &arg : arguments)
            flags.append(arg.toString());
    }

    const QString workingDir = object["directory"].toString();
    flags = filteredFlags(flags, fileName.fileName(), workingDir);

    CppTools::RawProjectPart rpp;
    rpp.setProjectFileLocation(projectFile.toString());
    rpp.setBuildSystemTarget(workingDir);
    rpp.setDisplayName(fileName.fileName());
    rpp.setFiles({fileName.toString()});

    CppTools::RawProjectPartFlags cxxProjectFlags;
    cxxProjectFlags.commandLineFlags = flags;
    rpp.setFlagsForCxx(cxxProjectFlags);

    return rpp;
}

CompilationDatabaseProject::CompilationDatabaseProject(const Utils::FileName &projectFile)
    : Project(Constants::COMPILATIONDATABASEMIMETYPE, projectFile)
    , m_cppCodeModelUpdater(std::make_unique<CppTools::CppProjectUpdater>(this))
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());

    connect(this, &Project::activeTargetChanged, [this, projectFile](ProjectExplorer::Target *target) {
        if (!target)
            return;

        ProjectExplorer::Kit *kit = target->kit();
        if (!kit)
            return;

        auto toolchains = ProjectExplorer::ToolChainKitInformation::toolChains(kit);
        if (toolchains.isEmpty())
            return;

        emitParsingStarted();

        const QFuture<void> future = ::Utils::runAsync([this, projectFile, kit,
                                                       tc = toolchains.first()](){
            QFile file(projectFilePath().toString());
            if (!file.open(QIODevice::ReadOnly)) {
                emitParsingFinished(false);
                return;
            }

            const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();

            auto root = std::make_unique<DBProjectNode>(projectDirectory());
            root->addNode(std::make_unique<ProjectExplorer::FileNode>(
                              projectFile,
                              ProjectExplorer::FileType::Project,
                              false));
            auto headers = std::make_unique<ProjectExplorer::VirtualFolderNode>(
                        Utils::FileName::fromString("Headers"), 0);
            auto sources = std::make_unique<ProjectExplorer::VirtualFolderNode>(
                        Utils::FileName::fromString("Sources"), 0);
            CppTools::RawProjectParts rpps;
            for (const QJsonValue &element : array) {
                const QJsonObject object = element.toObject();
                const QString filePath = object["file"].toString();
                const CppTools::ProjectFile::Kind kind = CppTools::ProjectFile::classify(filePath);
                ProjectExplorer::FolderNode *parent = nullptr;
                ProjectExplorer::FileType type = ProjectExplorer::FileType::Unknown;
                if (CppTools::ProjectFile::isHeader(kind)) {
                    parent = headers.get();
                    type = ProjectExplorer::FileType::Header;
                } else if (CppTools::ProjectFile::isSource(kind)) {
                    parent = sources.get();
                    type = ProjectExplorer::FileType::Source;
                } else {
                    parent = root.get();
                }
                parent->addNode(std::make_unique<ProjectExplorer::FileNode>(
                                    Utils::FileName::fromString(filePath), type, false));

                rpps.append(makeRawProjectPart(projectFile, object));
            }

            root->addNode(std::move(headers));
            root->addNode(std::move(sources));

            setRootProjectNode(std::move(root));

            CppTools::ToolChainInfo tcInfo;
            tcInfo.type = ProjectExplorer::Constants::COMPILATION_DATABASE_TOOLCHAIN_TYPEID;
            tcInfo.isMsvc2015ToolChain
                    = tc->targetAbi().osFlavor() == ProjectExplorer::Abi::WindowsMsvc2015Flavor;
            tcInfo.wordWidth = tc->targetAbi().wordWidth();
            tcInfo.targetTriple = tc->originalTargetTriple();
            tcInfo.sysRootPath = ProjectExplorer::SysRootKitInformation::sysRoot(kit).toString();
            tcInfo.headerPathsRunner = tc->createBuiltInHeaderPathsRunner();
            tcInfo.predefinedMacrosRunner = tc->createPredefinedMacrosRunner();

            m_cppCodeModelUpdater->update({this, tcInfo, tcInfo, rpps});

            emitParsingFinished(true);
        });
        m_parserWatcher.setFuture(future);
    });
}

CompilationDatabaseProject::~CompilationDatabaseProject()
{
    m_parserWatcher.cancel();
    m_parserWatcher.waitForFinished();
}

static TextEditor::TextDocument *createCompilationDatabaseDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    doc->setMimeType(Constants::COMPILATIONDATABASEMIMETYPE);
    return doc;
}

CompilationDatabaseEditorFactory::CompilationDatabaseEditorFactory()
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setDisplayName("Compilation Database");
    addMimeType(Constants::COMPILATIONDATABASEMIMETYPE);

    setEditorCreator([]() { return new TextEditor::BaseTextEditor; });
    setEditorWidgetCreator([]() { return new TextEditor::TextEditorWidget; });
    setDocumentCreator(createCompilationDatabaseDocument);
    setUseGenericHighlighter(true);
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setCodeFoldingSupported(true);
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
