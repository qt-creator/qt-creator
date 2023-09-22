// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeeditor.h"
#include "cmaketoolmanager.h"

#include "cmakeautocompleter.h"
#include "cmakebuildsystem.h"
#include "cmakefilecompletionassist.h"
#include "cmakeindenter.h"
#include "cmakekitaspect.h"
#include "cmakeprojectconstants.h"

#include "3rdparty/cmake/cmListFileCache.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreplugintr.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>
#include <utils/textutils.h>

#include <QTextDocument>

#include <functional>

using namespace Core;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace CMakeProjectManager::Internal {

//
// CMakeEditor
//

class CMakeEditor : public TextEditor::BaseTextEditor
{
    CMakeKeywords m_keywords;
public:
    CMakeEditor();
    void contextHelp(const HelpCallback &callback) const final;
};

CMakeEditor::CMakeEditor()
{
    CMakeTool *tool = nullptr;
    if (auto bs = ProjectTree::currentBuildSystem())
        tool = CMakeKitAspect::cmakeTool(bs->target()->kit());
    if (!tool)
        tool = CMakeToolManager::defaultCMakeTool();

    if (tool)
        m_keywords = tool->keywords();
}

void CMakeEditor::contextHelp(const HelpCallback &callback) const
{
    auto helpPrefix = [this](const QString &word) {
        if (m_keywords.includeStandardModules.contains(word))
            return "module/";
        if (m_keywords.functions.contains(word))
            return "command/";
        if (m_keywords.variables.contains(word))
            return "variable/";
        if (m_keywords.directoryProperties.contains(word))
            return "prop_dir/";
        if (m_keywords.targetProperties.contains(word))
            return "prop_tgt/";
        if (m_keywords.sourceProperties.contains(word))
            return "prop_sf/";
        if (m_keywords.testProperties.contains(word))
            return "prop_test/";
        if (m_keywords.properties.contains(word))
            return "prop_gbl/";
        if (m_keywords.policies.contains(word))
            return "policy/";

        return "unknown/";
    };

    const QString word = Utils::Text::wordUnderCursor(editorWidget()->textCursor());
    const QString id = helpPrefix(word) + word;
    if (id.startsWith("unknown/")) {
        BaseTextEditor::contextHelp(callback);
        return;
    }

    callback({{id, word}, {}, {}, HelpItem::Unknown});
}

//
// CMakeEditorWidget
//

class CMakeEditorWidget final : public TextEditorWidget
{
public:
    ~CMakeEditorWidget() final = default;

private:
    void findLinkAt(const QTextCursor &cursor,
                    const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
};

void CMakeEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

static bool mustBeQuotedInFileName(const QChar &c)
{
    return c.isSpace() || c == '"' || c == '(' || c == ')';
}

static bool isValidFileNameChar(const QString &block, int pos)
{
    const QChar c = block.at(pos);
    return !mustBeQuotedInFileName(c) || (pos > 0 && block.at(pos - 1) == '\\');
}

static QString unescape(const QString &s)
{
    QString result;
    int i = 0;
    const qsizetype size = s.size();
    while (i < size) {
        const QChar c = s.at(i);
        if (c == '\\' && i < size - 1) {
            const QChar nc = s.at(i + 1);
            if (mustBeQuotedInFileName(nc)) {
                result += nc;
                i += 2;
                continue;
            }
        }
        result += c;
        ++i;
    }
    return result;
}

QHash<QString, Utils::Link> getLocalSymbolsHash(const QByteArray &content, const Utils::FilePath &filePath)
{
    cmListFile cmakeListFile;
    if (!content.isEmpty()) {
        std::string errorString;
        const std::string fileName = "buffer";
        if (!cmakeListFile.ParseString(content.toStdString(), fileName, errorString))
            return {};
    }

    QHash<QString, Utils::Link> hash;
    for (const auto &func : cmakeListFile.Functions) {
        if (func.LowerCaseName() != "function" && func.LowerCaseName() != "macro"
            && func.LowerCaseName() != "set" && func.LowerCaseName() != "option")
            continue;

        if (func.Arguments().size() == 0)
            continue;
        auto arg = func.Arguments()[0];

        Utils::Link link;
        link.targetFilePath = filePath;
        link.targetLine = arg.Line;
        link.targetColumn = arg.Column - 1;
        hash.insert(QString::fromUtf8(arg.Value), link);
    }
    return hash;
}

void CMakeEditorWidget::findLinkAt(const QTextCursor &cursor,
                                   const Utils::LinkHandler &processLinkCallback,
                                   bool/* resolveTarget*/,
                                   bool /*inNextSplit*/)
{
    Utils::Link link;

    int line = 0;
    int column = 0;
    convertPosition(cursor.position(), &line, &column);

    const QString block = cursor.block().text();

    // check if the current position is commented out
    const qsizetype hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < column)
        return processLinkCallback(link);

    // find the beginning of a filename
    QString buffer;
    int beginPos = column - 1;
    while (beginPos >= 0) {
        if (isValidFileNameChar(block, beginPos)) {
            buffer.prepend(block.at(beginPos));
            beginPos--;
        } else {
            break;
        }
    }

    // find the end of a filename
    int endPos = column;
    while (endPos < block.size()) {
        if (isValidFileNameChar(block, endPos)) {
            buffer.append(block.at(endPos));
            endPos++;
        } else {
            break;
        }
    }

    if (buffer.isEmpty())
        return processLinkCallback(link);

    const Utils::FilePath dir = textDocument()->filePath().absolutePath();
    buffer.replace("${CMAKE_CURRENT_SOURCE_DIR}", dir.path());
    buffer.replace("${CMAKE_CURRENT_LIST_DIR}", dir.path());

    auto addTextStartEndToLink = [&](Utils::Link &link) {
        link.linkTextStart = cursor.position() - column + beginPos + 1;
        link.linkTextEnd = cursor.position() - column + endPos;
        return link;
    };

    if (auto project = ProjectTree::currentProject()) {
        buffer.replace("${CMAKE_SOURCE_DIR}", project->projectDirectory().path());
        if (auto bs = ProjectTree::currentBuildSystem()) {
            buffer.replace("${CMAKE_BINARY_DIR}", bs->buildConfiguration()->buildDirectory().path());

            // Get the path suffix from current source dir to project source dir and apply it
            // for the binary dir
            const QString relativePathSuffix = textDocument()
                                                   ->filePath()
                                                   .parentDir()
                                                   .relativePathFrom(project->projectDirectory())
                                                   .path();
            buffer.replace("${CMAKE_CURRENT_BINARY_DIR}",
                           bs->buildConfiguration()
                               ->buildDirectory()
                               .pathAppended(relativePathSuffix)
                               .path());

            // Check if the symbols is a user defined function or macro
            const CMakeBuildSystem *cbs = static_cast<const CMakeBuildSystem *>(bs);
            if (cbs->cmakeSymbolsHash().contains(buffer)) {
                link = cbs->cmakeSymbolsHash().value(buffer);
                addTextStartEndToLink(link);
                return processLinkCallback(link);
            }
        }
    }
    // TODO: Resolve more variables

    // Resolve local variables and functions
    auto findFunctionEnd = [cursor, this]() -> int {
        int pos = cursor.position();
        QChar chr;
        do {
            chr = textDocument()->characterAt(--pos);
        } while (pos > 0 && chr != ')');
        return pos;
    };
    auto hash = getLocalSymbolsHash(textDocument()->textAt(0, findFunctionEnd() + 1).toUtf8(),
                                    textDocument()->filePath());

    // Strip variable coating
    if (buffer.startsWith("${") && buffer.endsWith("}"))
        buffer = buffer.mid(2, buffer.size() - 3);

    if (hash.contains(buffer)) {
        link = hash.value(buffer);
        addTextStartEndToLink(link);
        return processLinkCallback(link);
    }

    Utils::FilePath fileName = dir.withNewPath(unescape(buffer));
    if (fileName.isRelativePath())
        fileName = dir.pathAppended(fileName.path());
    if (fileName.exists()) {
        if (fileName.isDir()) {
            Utils::FilePath subProject = fileName.pathAppended("CMakeLists.txt");
            if (subProject.exists())
                fileName = subProject;
            else
                return processLinkCallback(link);
        }
        link.targetFilePath = fileName;
        addTextStartEndToLink(link);
    }

    processLinkCallback(link);
}

static TextDocument *createCMakeDocument()
{
    auto doc = new TextDocument;
    doc->setId(Constants::CMAKE_EDITOR_ID);
    doc->setMimeType(QLatin1String(Constants::CMAKE_MIMETYPE));
    return doc;
}

//
// CMakeEditorFactory
//

CMakeEditorFactory::CMakeEditorFactory()
{
    setId(Constants::CMAKE_EDITOR_ID);
    setDisplayName(::Core::Tr::tr("CMake Editor"));
    addMimeType(Constants::CMAKE_MIMETYPE);
    addMimeType(Constants::CMAKE_PROJECT_MIMETYPE);

    setEditorCreator([] { return new CMakeEditor; });
    setEditorWidgetCreator([] { return new CMakeEditorWidget; });
    setDocumentCreator(createCMakeDocument);
    setIndenterCreator([](QTextDocument *doc) { return new CMakeIndenter(doc); });
    setUseGenericHighlighter(true);
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setCodeFoldingSupported(true);

    setCompletionAssistProvider(new CMakeFileCompletionAssistProvider);
    setAutoCompleterCreator([] { return new CMakeAutoCompleter; });

    setEditorActionHandlers(TextEditorActionHandler::UnCommentSelection
                            | TextEditorActionHandler::FollowSymbolUnderCursor
                            | TextEditorActionHandler::Format);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    contextMenu->addAction(ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR));
    contextMenu->addSeparator(Context(Constants::CMAKE_EDITOR_ID));
    contextMenu->addAction(ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION));
}

} // CMakeProjectManager::Internal
