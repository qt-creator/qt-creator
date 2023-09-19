// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakefilecompletionassist.h"

#include "cmakebuildsystem.h"
#include "cmakebuildtarget.h"
#include "cmakekitaspect.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmaketool.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/texteditorsettings.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/utilsicons.h>

using namespace TextEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class CMakeFileCompletionAssist : public AsyncProcessor
{
public:
    CMakeFileCompletionAssist();

    IAssistProposal *performAsync() final;

    const QIcon m_variableIcon;
    const QIcon m_projectVariableIcon;
    const QIcon m_functionIcon;
    const QIcon m_projectFunctionIcon;
    const QIcon m_propertyIcon;
    const QIcon m_argsIcon;
    const QIcon m_genexIcon;
    const QIcon m_moduleIcon;
    const QIcon m_targetsIcon;

    TextEditor::SnippetAssistCollector m_snippetCollector;
};

CMakeFileCompletionAssist::CMakeFileCompletionAssist()
    : m_variableIcon(CodeModelIcon::iconForType(CodeModelIcon::VarPublic))
    , m_projectVariableIcon(CodeModelIcon::iconForType(CodeModelIcon::VarPublicStatic))
    , m_functionIcon(CodeModelIcon::iconForType(CodeModelIcon::FuncPublic))
    , m_projectFunctionIcon(CodeModelIcon::iconForType(CodeModelIcon::FuncPublicStatic))
    , m_propertyIcon(CodeModelIcon::iconForType(CodeModelIcon::Property))
    , m_argsIcon(CodeModelIcon::iconForType(CodeModelIcon::Enum))
    , m_genexIcon(CodeModelIcon::iconForType(CodeModelIcon::Class))
    , m_moduleIcon(
          ProjectExplorer::DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_MODULES).icon())
    , m_targetsIcon(ProjectExplorer::Icons::BUILD.icon())
    , m_snippetCollector(Constants::CMAKE_SNIPPETS_GROUP_ID,
                         FileIconProvider::icon(FilePath::fromString("CMakeLists.txt")))

{}

static bool isInComment(const AssistInterface *interface)
{
    QTextCursor tc(interface->textDocument());
    tc.setPosition(interface->position());
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    return tc.selectedText().contains('#');
}

static bool isValidIdentifierChar(const QChar &chr)
{
    return chr.isLetterOrNumber() || chr == '_' || chr == '-';
}

static int findWordStart(const AssistInterface *interface, int pos)
{
    // Find start position
    QChar chr;
    do {
        chr = interface->characterAt(--pos);
    } while (pos > 0 && isValidIdentifierChar(chr));

    return ++pos;
}

static int findFunctionStart(const AssistInterface *interface)
{
    int pos = interface->position();

    QChar chr;
    do {
        chr = interface->characterAt(--pos);
    } while (pos > 0 && chr != '(');

    if (pos > 0 && chr == '(') {
        // allow space between function name and (
        do {
            chr = interface->characterAt(--pos);
        } while (pos > 0 && chr.isSpace());
        ++pos;
    }

    return pos;
}

static int findFunctionEnd(const AssistInterface *interface)
{
    int pos = interface->position();

    QChar chr;
    do {
        chr = interface->characterAt(--pos);
    } while (pos > 0 && chr != ')');

    return pos;
}

static int findPathStart(const AssistInterface *interface)
{
    // For pragmatic reasons, we don't support spaces in file names here.
    static const auto canOccurInFilePath = [](const QChar &c) {
        return c.isLetterOrNumber() || c == '.' || c == '/' || c == '_' || c == '-';
    };

    int pos = interface->position();
    QChar chr;
    // Skip to the start of a name
    do {
        chr = interface->characterAt(--pos);
    } while (canOccurInFilePath(chr));

    return ++pos;
}

QList<AssistProposalItemInterface *> generateList(const QStringList &words, const QIcon &icon)
{
    return transform(words, [&icon](const QString &word) -> AssistProposalItemInterface * {
        AssistProposalItem *item = new AssistProposalItem();
        item->setText(word);
        item->setIcon(icon);
        return item;
    });
}

static int addFilePathItems(const AssistInterface *interface,
                            QList<AssistProposalItemInterface *> &items,
                            int symbolStartPos)
{
    if (interface->filePath().isEmpty())
        return symbolStartPos;

    const int startPos = findPathStart(interface);

    if (interface->reason() == IdleEditor
        && interface->position() - startPos
               < TextEditorSettings::completionSettings().m_characterThreshold)
        return symbolStartPos;

    const QString word = interface->textAt(startPos, interface->position() - startPos);
    FilePath baseDir = interface->filePath().absoluteFilePath().parentDir();
    const int lastSlashPos = word.lastIndexOf(QLatin1Char('/'));

    QString prefix = word;
    if (lastSlashPos != -1) {
        prefix = word.mid(lastSlashPos + 1);
        baseDir = baseDir.pathAppended(word.left(lastSlashPos));
    }

    const FilePaths filesPaths = baseDir.dirEntries(
        FileFilter({QString("%1*").arg(prefix)}, QDir::AllEntries | QDir::NoDotAndDotDot));
    for (const auto &file : filesPaths) {
        AssistProposalItem *item = new AssistProposalItem;
        QString fileName = file.fileName();
        if (file.isDir())
            fileName.append("/");
        item->setText(fileName);
        item->setIcon(FileIconProvider::icon(file));

        items << item;
    }

    return startPos;
}

IAssistProposal *CMakeFileCompletionAssist::performAsync()
{
    CMakeKeywords keywords;
    CMakeKeywords projectKeywords;
    Project *project = nullptr;
    const FilePath &filePath = interface()->filePath();
    if (!filePath.isEmpty() && filePath.isFile()) {
        project = static_cast<CMakeProject *>(ProjectManager::projectForFile(filePath));
        if (project && project->activeTarget()) {
            CMakeTool *cmake = CMakeKitAspect::cmakeTool(project->activeTarget()->kit());
            if (cmake && cmake->isValid())
                keywords = cmake->keywords();
        }
    }

    QStringList buildTargets;
    if (project && project->activeTarget()) {
        const auto bs = qobject_cast<CMakeBuildSystem *>(project->activeTarget()->buildSystem());
        if (bs) {
            for (const auto &target : std::as_const(bs->buildTargets()))
                if (target.targetType != TargetType::UtilityType)
                    buildTargets << target.title;
            projectKeywords = bs->projectKeywords();
        }
    }

    if (isInComment(interface()))
        return nullptr;

    const int startPos = findWordStart(interface(), interface()->position());

    const int functionStart = findFunctionStart(interface());
    const int prevFunctionEnd = findFunctionEnd(interface());

    QString functionName;
    if (functionStart > prevFunctionEnd) {
        int functionStartPos = findWordStart(interface(), functionStart);
        functionName
            = interface()->textAt(functionStartPos, functionStart - functionStartPos);
    }

    if (interface()->reason() == IdleEditor) {
        const QChar chr = interface()->characterAt(interface()->position());
        const int wordSize = interface()->position() - startPos;
        if (isValidIdentifierChar(chr)
            || wordSize < TextEditorSettings::completionSettings().m_characterThreshold) {
            return nullptr;
        }
    }

    QList<AssistProposalItemInterface *> items;

    const QString varGenexToken = interface()->textAt(startPos - 2, 2);
    if (varGenexToken == "${" || varGenexToken == "$<") {
        if (varGenexToken == "${") {
            items.append(generateList(keywords.variables, m_variableIcon));
            items.append(generateList(projectKeywords.variables, m_projectVariableIcon));
        }
        if (varGenexToken == "$<")
            items.append(generateList(keywords.generatorExpressions, m_genexIcon));

        return new GenericProposal(startPos, items);
    }

    int fileStartPos = startPos;
    const auto onlyFileItems = [&] { return fileStartPos != startPos; };

    if (functionName == "if" || functionName == "elseif" || functionName == "while"
        || functionName == "set" || functionName == "list"
        || functionName == "cmake_print_variables") {
        items.append(generateList(keywords.variables, m_variableIcon));
        items.append(generateList(projectKeywords.variables, m_projectVariableIcon));
    }

    if (functionName.contains("path") || functionName.contains("file")
        || functionName.contains("add_executable") || functionName.contains("add_library")
        || functionName == "include" || functionName == "add_subdirectory"
        || functionName == "install" || functionName == "target_sources" || functionName == "set"
        || functionName == "list") {
        fileStartPos = addFilePathItems(interface(), items, startPos);
    }

    if (functionName == "set_property" || functionName == "cmake_print_properties")
        items.append(generateList(keywords.properties, m_propertyIcon));

    if (functionName == "set_directory_properties")
        items.append(generateList(keywords.directoryProperties, m_propertyIcon));
    if (functionName == "set_source_files_properties")
        items.append(generateList(keywords.sourceProperties, m_propertyIcon));
    if (functionName == "set_target_properties")
        items.append(generateList(keywords.targetProperties, m_propertyIcon));
    if (functionName == "set_tests_properties")
        items.append(generateList(keywords.testProperties, m_propertyIcon));

    if (functionName == "include" && !onlyFileItems())
        items.append(generateList(keywords.includeStandardModules, m_moduleIcon));
    if (functionName == "find_package")
        items.append(generateList(keywords.findModules, m_moduleIcon));

    if ((functionName.contains("target") || functionName == "install"
         || functionName == "add_dependencies" || functionName == "set_property"
         || functionName == "export" || functionName == "cmake_print_properties")
        && !onlyFileItems())
        items.append(generateList(buildTargets, m_targetsIcon));

    if (keywords.functionArgs.contains(functionName) && !onlyFileItems()) {
        QStringList functionSymbols = keywords.functionArgs.value(functionName);
        items.append(generateList(functionSymbols, m_argsIcon));
    } else if (functionName.isEmpty()) {
        // On a new line we just want functions
        items.append(generateList(keywords.functions, m_functionIcon));
        items.append(generateList(projectKeywords.functions, m_projectFunctionIcon));

        // Snippets would make more sense only for the top level suggestions
        items.append(m_snippetCollector.collect());
    } else {
        // Inside an unknown function we could have variables or properties
        fileStartPos = addFilePathItems(interface(), items, startPos);
        if (!onlyFileItems()) {
            items.append(generateList(keywords.variables, m_variableIcon));
            items.append(generateList(projectKeywords.variables, m_projectVariableIcon));
            items.append(generateList(keywords.properties, m_propertyIcon));
            items.append(generateList(buildTargets, m_targetsIcon));
        }
    }

    return new GenericProposal(startPos, items);
}

IAssistProcessor *CMakeFileCompletionAssistProvider::createProcessor(const AssistInterface *) const
{
    return new CMakeFileCompletionAssist;
}

int CMakeFileCompletionAssistProvider::activationCharSequenceLength() const
{
    return 2;
}

bool CMakeFileCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return sequence.endsWith("${") || sequence.endsWith("$<") || sequence.endsWith("/")
           || sequence.endsWith("(");
}

} // namespace CMakeProjectManager::Internal
