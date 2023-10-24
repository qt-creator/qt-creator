// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakefilecompletionassist.h"

#include "cmakebuildsystem.h"
#include "cmakebuildtarget.h"
#include "cmakebuildconfiguration.h"
#include "cmakeconfigitem.h"
#include "cmakeprojectconstants.h"
#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include "3rdparty/cmake/cmListFileCache.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/texteditorsettings.h>

#include <utils/async.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/utilsicons.h>

using namespace TextEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class PerformInputData;
using PerformInputDataPtr = std::shared_ptr<PerformInputData>;

class CMakeFileCompletionAssist : public AsyncProcessor
{
public:
    CMakeFileCompletionAssist();

    IAssistProposal *perform() final;
    IAssistProposal *performAsync() final { return nullptr; }

    const QIcon m_variableIcon;
    const QIcon m_projectVariableIcon;
    const QIcon m_functionIcon;
    const QIcon m_projectFunctionIcon;
    const QIcon m_propertyIcon;
    const QIcon m_argsIcon;
    const QIcon m_genexIcon;
    const QIcon m_moduleIcon;
    const QIcon m_targetsIcon;
    const QIcon m_importedTargetIcon;

    TextEditor::SnippetAssistCollector m_snippetCollector;

private:
    IAssistProposal *doPerform(const PerformInputDataPtr &data);
    PerformInputDataPtr generatePerformInputData() const;
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
    , m_targetsIcon(ProjectExplorer::Icons::BUILD_SMALL.icon())
    , m_importedTargetIcon(Icon({{":/projectexplorer/images/buildhammerhandle.png",
                                  Theme::IconsCodeModelKeywordColor},
                                 {":/projectexplorer/images/buildhammerhead.png",
                                  Theme::IconsCodeModelKeywordColor}},
                                Icon::MenuTintedStyle)
                               .icon())
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

struct MarkDownAssitProposalItem : public AssistProposalItem
{
    Qt::TextFormat detailFormat() const override { return Qt::MarkdownText; }
};

template<typename T>
static QList<AssistProposalItemInterface *> generateList(const T &words, const QIcon &icon)
{
    return transform<QList>(words, [&icon](const QString &word) -> AssistProposalItemInterface * {
        AssistProposalItem *item = new AssistProposalItem();
        item->setText(word);
        item->setIcon(icon);
        return item;
    });
}

static QList<AssistProposalItemInterface *> generateList(const QMap<QString, FilePath> &words,
                                                         const QIcon &icon)
{
    QList<AssistProposalItemInterface *> list;
    for (auto it = words.cbegin(); it != words.cend(); ++it) {
        MarkDownAssitProposalItem *item = new MarkDownAssitProposalItem();
        item->setText(it.key());
        if (!it.value().isEmpty())
            item->setDetail(CMakeToolManager::toolTipForRstHelpFile(it.value()));
        item->setIcon(icon);
        list << item;
    }
    return list;
}

static QList<AssistProposalItemInterface *> generateList(
    const CMakeConfig &cache,
    const QIcon &icon,
    const QList<AssistProposalItemInterface *> &existingList)
{
    QHash<QString, AssistProposalItemInterface *> hash;
    for (const auto &item : existingList)
        hash.insert(item->text(), item);

    auto makeDetail = [](const CMakeConfigItem &item) {
        QString detail = QString("### %1 (cache)").arg(QString::fromUtf8(item.key));

        if (!item.documentation.isEmpty())
            detail.append(QString("\n%1\n").arg(QString::fromUtf8(item.documentation)));
        else
            detail.append("\n");

        const QString value = item.toString();
        if (!value.isEmpty())
            detail.append(QString("\n```\n%1\n```\n").arg(value));

        return detail;
    };

    QList<AssistProposalItemInterface *> list;
    for (auto it = cache.cbegin(); it != cache.cend(); ++it) {
        if (it->isAdvanced || it->isUnset || it->type == CMakeConfig::Type::INTERNAL)
            continue;

        QString text = QString::fromUtf8(it->key);
        if (!hash.contains(text)) {
            MarkDownAssitProposalItem *item = new MarkDownAssitProposalItem();
            item->setText(text);
            item->setDetail(makeDetail(*it));
            item->setIcon(icon);
            list << item;
        } else {
            auto item = static_cast<AssistProposalItem *>(hash.value(text));

            QString detail = item->detail();
            detail.append("\n");
            detail.append(makeDetail(*it));

            item->setDetail(detail);
        }
    }
    return list;
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
    const qsizetype lastSlashPos = word.lastIndexOf(QLatin1Char('/'));

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

static cmListFile parseCMakeListFromBuffer(const QByteArray &content)
{
    cmListFile cmakeListFile;
    std::string errorString;
    if (!content.isEmpty()) {
        const std::string fileName = "buffer";
        if (!cmakeListFile.ParseString(content.toStdString(), fileName, errorString))
            return {};
    }
    return cmakeListFile;
}

static QPair<QStringList, QStringList> getLocalFunctionsAndVariables(const cmListFile &cmakeListFile)
{
    QStringList variables;
    QStringList functions;
    for (const auto &func : cmakeListFile.Functions) {
        if (func.Arguments().size() == 0)
            continue;

        if (func.LowerCaseName() == "macro" || func.LowerCaseName() == "function")
            functions << QString::fromUtf8(func.Arguments()[0].Value);
        if (func.LowerCaseName() == "set" || func.LowerCaseName() == "option")
            variables << QString::fromUtf8(func.Arguments()[0].Value);
    }
    return {functions, variables};
}

static void updateCMakeConfigurationWithLocalData(CMakeConfig &cmakeCache,
                                                  const cmListFile &cmakeListFile,
                                                  const FilePath &currentDir)
{
    auto isValidCMakeVariable = [](const std::string &var) {
        return var == "CMAKE_PREFIX_PATH" || var == "CMAKE_MODULE_PATH";
    };

    const FilePath projectDir = ProjectTree::currentBuildSystem()->projectDirectory();
    auto updateDirVariables = [currentDir, projectDir, cmakeCache](QByteArray &value) {
        value.replace("${CMAKE_CURRENT_SOURCE_DIR}", currentDir.path().toUtf8());
        value.replace("${CMAKE_CURRENT_LIST_DIR}", currentDir.path().toUtf8());
        value.replace("${CMAKE_SOURCE_DIR}", projectDir.path().toUtf8());
        value.replace("${CMAKE_PREFIX_PATH}", cmakeCache.valueOf("CMAKE_PREFIX_PATH"));
        value.replace("${CMAKE_MODULE_PATH}", cmakeCache.valueOf("CMAKE_MODULE_PATH"));
    };

    auto insertOrAppendListValue = [&cmakeCache](const QByteArray &key, const QByteArray &value) {
        auto it = std::find_if(cmakeCache.begin(), cmakeCache.end(), [key](const auto &item) {
            return item.key == key;
        });
        if (it == cmakeCache.end()) {
            cmakeCache << CMakeConfigItem(key, value);
        } else {
            it->value.append(";");
            it->value.append(value);
        }
    };

    for (const auto &func : cmakeListFile.Functions) {
        const bool isSet = func.LowerCaseName() == "set" && func.Arguments().size() > 1;
        const bool isList = func.LowerCaseName() == "list" && func.Arguments().size() > 2;
        if (!isSet && !isList)
            continue;

        QByteArray key;
        QByteArray value;
        if (isSet) {
            const auto firstArg = func.Arguments()[0];
            const auto secondArg = func.Arguments()[1];
            if (!isValidCMakeVariable(firstArg.Value))
                continue;
            key = QByteArray::fromStdString(firstArg.Value);
            value = QByteArray::fromStdString(secondArg.Value);
        }
        if (isList) {
            const auto firstArg = func.Arguments()[0];
            const auto secondArg = func.Arguments()[1];
            const auto thirdArg = func.Arguments()[2];
            if (firstArg.Value != "APPEND" || !isValidCMakeVariable(secondArg.Value))
                continue;
            key = QByteArray::fromStdString(secondArg.Value);
            value = QByteArray::fromStdString(thirdArg.Value);
        }
        updateDirVariables(value);
        insertOrAppendListValue(key, value);
    }
}

static QPair<QStringList, QStringList> getFindAndConfigCMakePackages(
    const CMakeConfig &cmakeCache, const Environment &environment)
{
    auto toFilePath = [](const QByteArray &str) -> FilePath {
        return FilePath::fromUserInput(QString::fromUtf8(str));
    };

    auto findPackageName = [](const QString &fileName) -> QString {
        auto findIdx = fileName.indexOf("Find");
        auto endsWithCMakeIdx = fileName.lastIndexOf(".cmake");
        if (findIdx == 0 && endsWithCMakeIdx > 0)
            return fileName.mid(4, endsWithCMakeIdx - 4);
        return QString();
    };

    auto configPackageName = [](const QString &fileName) -> QString {
        auto configCMakeIdx = fileName.lastIndexOf("Config.cmake");
        if (configCMakeIdx > 0)
            return fileName.left(configCMakeIdx);
        auto dashConfigCMakeIdx = fileName.lastIndexOf("-config.cmake");
        if (dashConfigCMakeIdx > 0)
            return fileName.left(dashConfigCMakeIdx);
        return QString();
    };

    QStringList modulePackages;
    QStringList configPackages;

    struct
    {
        const QByteArray cmakeVariable;
        const QString pathPrefix;
        std::function<QString(const QString &)> function;
        QStringList &result;
    } mapping[] = {{"CMAKE_PREFIX_PATH", "lib/cmake", configPackageName, configPackages},
                   {"CMAKE_PREFIX_PATH", "share", configPackageName, configPackages},
                   {"CMAKE_MODULE_PATH", QString(), findPackageName, modulePackages},
                   {"CMAKE_MODULE_PATH", QString(), configPackageName, configPackages}};

    for (const auto &m : mapping) {
        FilePaths paths = Utils::transform<FilePaths>(cmakeCache.valueOf(m.cmakeVariable).split(';'),
                                                      toFilePath);

        paths << Utils::transform<FilePaths>(environment.value(QString::fromUtf8(m.cmakeVariable))
                                                 .split(";"),
                                             &FilePath::fromUserInput);

        for (const auto &prefix : paths) {
            // Only search for directories if we have a prefix
            const FilePaths dirs = !m.pathPrefix.isEmpty()
                                       ? prefix.pathAppended(m.pathPrefix)
                                             .dirEntries({{"*"}, QDir::Dirs | QDir::NoDotAndDotDot})
                                       : FilePaths{prefix};
            const QStringList cmakeFiles
                = Utils::transform<QStringList>(dirs, [](const FilePath &path) {
                      return Utils::transform(path.dirEntries({{"*.cmake"}, QDir::Files},
                                                              QDir::Name),
                                              &FilePath::fileName);
                  });
            m.result << Utils::transform(cmakeFiles, m.function);
        }
        m.result = Utils::filtered(m.result, std::not_fn(&QString::isEmpty));
    }

    return {modulePackages, configPackages};
}

class PerformInputData
{
public:
    CMakeKeywords keywords;
    QMap<QString, FilePath> projectVariables;
    QMap<QString, FilePath> projectFunctions;
    QStringList buildTargets;
    QStringList importedTargets;
    QStringList findPackageVariables;
    CMakeConfig cmakeConfiguration;
    Environment environment = Environment::systemEnvironment();
};

PerformInputDataPtr CMakeFileCompletionAssist::generatePerformInputData() const
{
    PerformInputDataPtr data = PerformInputDataPtr(new PerformInputData);

    const FilePath &filePath = interface()->filePath();
    if (!filePath.isEmpty() && filePath.isFile()) {
        if (auto tool = CMakeToolManager::defaultProjectOrDefaultCMakeTool())
            data->keywords = tool->keywords();
    }

    if (auto bs = qobject_cast<CMakeBuildSystem *>(ProjectTree::currentBuildSystem())) {
        for (const auto &target : std::as_const(bs->buildTargets()))
            if (target.targetType != TargetType::UtilityType)
                data->buildTargets << target.title;
        const CMakeKeywords &projectKeywords = bs->projectKeywords();
        data->projectVariables = projectKeywords.variables;
        data->projectFunctions = projectKeywords.functions;
        data->importedTargets = bs->projectImportedTargets();
        data->findPackageVariables = bs->projectFindPackageVariables();
        data->cmakeConfiguration = bs->configurationFromCMake();
        data->environment = bs->cmakeBuildConfiguration()->configureEnvironment();
    }

    return data;
}

IAssistProposal *CMakeFileCompletionAssist::perform()
{
    IAssistProposal *result = immediateProposal();
    interface()->prepareForAsyncUse();
    m_watcher.setFuture(Utils::asyncRun([this, inputData = generatePerformInputData()] {
        interface()->recreateTextDocument();
        return doPerform(inputData);
    }));
    return result;
}

IAssistProposal *CMakeFileCompletionAssist::doPerform(const PerformInputDataPtr &data)
{
    if (isInComment(interface()))
        return nullptr;

    const int startPos = findWordStart(interface(), interface()->position());
    const int functionStart = findFunctionStart(interface());
    const int prevFunctionEnd = findFunctionEnd(interface());

    QString functionName;
    if (functionStart > prevFunctionEnd) {
        const int functionStartPos = findWordStart(interface(), functionStart);
        functionName = interface()->textAt(functionStartPos, functionStart - functionStartPos);
    }

    if (interface()->reason() == IdleEditor) {
        const QChar chr = interface()->characterAt(interface()->position());
        const int wordSize = interface()->position() - startPos;
        if (isValidIdentifierChar(chr)
            || wordSize < TextEditorSettings::completionSettings().m_characterThreshold) {
            return nullptr;
        }
    }

    cmListFile cmakeListFile = parseCMakeListFromBuffer(
        interface()->textAt(0, prevFunctionEnd + 1).toUtf8());
    auto [localFunctions, localVariables] = getLocalFunctionsAndVariables(cmakeListFile);

    CMakeConfig cmakeConfiguration = data->cmakeConfiguration;
    const FilePath currentDir = interface()->filePath().absolutePath();
    updateCMakeConfigurationWithLocalData(cmakeConfiguration, cmakeListFile, currentDir);

    auto [findModules, configModules] = getFindAndConfigCMakePackages(cmakeConfiguration,
                                                                      data->environment);

    QList<AssistProposalItemInterface *> items;

    const QString varGenexToken = interface()->textAt(startPos - 2, 2);
    const QString varEnvironmentToken = interface()->textAt(startPos - 5, 5);
    if (varGenexToken == "${" || varGenexToken == "$<" || varEnvironmentToken == "$ENV{") {
        if (varGenexToken == "${") {
            items.append(generateList(data->keywords.variables, m_variableIcon));
            items.append(generateList(data->projectVariables, m_projectVariableIcon));
            items.append(generateList(data->findPackageVariables, m_projectVariableIcon));
        }
        if (varGenexToken == "$<")
            items.append(generateList(data->keywords.generatorExpressions, m_genexIcon));

        if (varEnvironmentToken == "$ENV{")
            items.append(generateList(data->keywords.environmentVariables, m_variableIcon));

        return new GenericProposal(startPos, items);
    }

    const QString ifEnvironmentToken = interface()->textAt(startPos - 4, 4);
    if ((functionName == "if" || functionName == "elseif") && ifEnvironmentToken == "ENV{")
        items.append(generateList(data->keywords.environmentVariables, m_variableIcon));

    int fileStartPos = startPos;
    const auto onlyFileItems = [&] { return fileStartPos != startPos; };

    if (functionName == "if" || functionName == "elseif" || functionName == "while"
        || functionName == "set" || functionName == "list"
        || functionName == "cmake_print_variables") {
        items.append(generateList(data->keywords.variables, m_variableIcon));
        items.append(generateList(data->projectVariables, m_projectVariableIcon));
        items.append(generateList(data->findPackageVariables, m_projectVariableIcon));
        items.append(generateList(localVariables, m_variableIcon));
        items.append(generateList(cmakeConfiguration, m_variableIcon, items));
    }

    if (functionName == "if" || functionName == "elseif" || functionName == "cmake_policy")
        items.append(generateList(data->keywords.policies, m_variableIcon));

    if (functionName.contains("path") || functionName.contains("file")
        || functionName.contains("add_executable") || functionName.contains("add_library")
        || functionName == "include" || functionName == "add_subdirectory"
        || functionName == "install" || functionName == "target_sources"
        || functionName == "set" || functionName == "list") {
        fileStartPos = addFilePathItems(interface(), items, startPos);
    }

    if (functionName == "set_property" || functionName == "cmake_print_properties")
        items.append(generateList(data->keywords.properties, m_propertyIcon));

    if (functionName == "set_directory_properties")
        items.append(generateList(data->keywords.directoryProperties, m_propertyIcon));
    if (functionName == "set_source_files_properties")
        items.append(generateList(data->keywords.sourceProperties, m_propertyIcon));
    if (functionName == "set_target_properties")
        items.append(generateList(data->keywords.targetProperties, m_propertyIcon));
    if (functionName == "set_tests_properties")
        items.append(generateList(data->keywords.testProperties, m_propertyIcon));

    if (functionName == "include" && !onlyFileItems())
        items.append(generateList(data->keywords.includeStandardModules, m_moduleIcon));
    if (functionName == "find_package") {
        items.append(generateList(data->keywords.findModules, m_moduleIcon));
        items.append(generateList(findModules, m_moduleIcon));
        items.append(generateList(configModules, m_moduleIcon));
    }

    if ((functionName.contains("target") || functionName == "install"
         || functionName == "add_dependencies" || functionName == "set_property"
         || functionName == "export" || functionName == "cmake_print_properties"
         || functionName == "if" || functionName == "elseif")
        && !onlyFileItems()) {
        items.append(generateList(data->buildTargets, m_targetsIcon));
        items.append(generateList(data->importedTargets, m_importedTargetIcon));
    }

    if (data->keywords.functionArgs.contains(functionName) && !onlyFileItems()) {
        const QStringList functionSymbols = data->keywords.functionArgs.value(functionName);
        items.append(generateList(functionSymbols, m_argsIcon));
    } else if (functionName.isEmpty()) {
        // On a new line we just want functions
        items.append(generateList(data->keywords.functions, m_functionIcon));
        items.append(generateList(data->projectFunctions, m_projectFunctionIcon));
        items.append(generateList(localFunctions, m_functionIcon));

        // Snippets would make more sense only for the top level suggestions
        items.append(m_snippetCollector.collect());
    } else {
        // Inside an unknown function we could have variables or properties
        fileStartPos = addFilePathItems(interface(), items, startPos);
        if (!onlyFileItems()) {
            items.append(generateList(data->keywords.variables, m_variableIcon));
            items.append(generateList(data->projectVariables, m_projectVariableIcon));
            items.append(generateList(localVariables, m_variableIcon));
            items.append(generateList(cmakeConfiguration, m_variableIcon, items));
            items.append(generateList(data->findPackageVariables, m_projectVariableIcon));

            items.append(generateList(data->keywords.properties, m_propertyIcon));
            items.append(generateList(data->buildTargets, m_targetsIcon));
            items.append(generateList(data->importedTargets, m_importedTargetIcon));
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
    return 4;
}

bool CMakeFileCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return sequence.endsWith("${") || sequence.endsWith("$<") || sequence.endsWith("/")
           || sequence.endsWith("(") || sequence.endsWith("ENV{");
}

} // namespace CMakeProjectManager::Internal
