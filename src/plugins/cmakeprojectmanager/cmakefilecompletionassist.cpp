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
#include "cmakeutils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/asyncprocessor.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/snippets/snippetassistcollector.h>

#include <utils/async.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/utilsicons.h>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace TextEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class PerformInputData;
using PerformInputDataPtr = std::shared_ptr<PerformInputData>;

class CMakeFileCompletionAssist : public AsyncProcessor
{
public:
    explicit CMakeFileCompletionAssist(bool functionHintOnly = false);

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
    IAssistProposal *functionHint(const QString &functionName,
                                  const PerformInputDataPtr &data,
                                  const CMakeLang::DocumentPtr &document,
                                  const CMakeLang::SignatureTable &local);
    PerformInputDataPtr generatePerformInputData() const;

    // Whoever asks for the signature of a call is asking for that alone.
    const bool m_functionHintOnly;
};

CMakeFileCompletionAssist::CMakeFileCompletionAssist(bool functionHintOnly)
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
                         FileIconProvider::icon(FilePath::fromString(Constants::CMAKE_LISTS_TXT)))
    , m_functionHintOnly(functionHintOnly)
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

// Where the arguments of the call the cursor stands in begin, which is
// behind the parenthesis that opens it.
static int findArgumentsStart(const AssistInterface *interface)
{
    int pos = interface->position();

    QChar chr;
    do {
        chr = interface->characterAt(--pos);
    } while (pos > 0 && chr != '(');

    return chr == '(' ? pos + 1 : interface->position();
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

    // The file that documents the name of the item, where one does.  What
    // it says is read when the item is the one being looked at: reading it
    // for every item of a completion would read every file of the Help of
    // CMake for one "${".
    void setDocumentationFile(const FilePath &file) { m_documentationFile = file; }

    QString detail() const override
    {
        const QString detail = AssistProposalItem::detail();
        if (m_documentationFile.isEmpty())
            return detail;

        const QString documentation = CMakeToolManager::toolTip(text(), m_documentationFile);
        if (documentation.isEmpty())
            return detail;
        if (detail.isEmpty())
            return documentation;
        return documentation + '\n' + detail;
    }

private:
    FilePath m_documentationFile;
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

// What a documented argument is called where it is a keyword: the
// documentation spells the value it takes out behind it, as in
// "LIMIT_INPUT <max-in>".
static QString keywordOfArgument(const QString &name)
{
    const QString keyword = name.section(' ', 0, 0);
    if (keyword == name || keyword.isEmpty())
        return {};

    for (const QChar &c : keyword) {
        if (!c.isUpper() && !c.isDigit() && c != '_')
            return {};
    }
    return keyword;
}

// The keywords a command takes, each with what its documentation says about
// it: a command like file() documents a signature of its own for every mode
// it knows.
static QList<AssistProposalItemInterface *> generateList(
    const QStringList &words, const QIcon &icon, const QList<CMakeLang::ArgumentDoc> &arguments)
{
    QHash<QString, QString> documentation;
    for (const CMakeLang::ArgumentDoc &argument : arguments) {
        documentation.insert(argument.name, argument.documentation);

        // A keyword that takes a value is documented with the value behind
        // it, and the keyword alone is what the reader is writing.
        const QString keyword = keywordOfArgument(argument.name);
        if (!keyword.isEmpty() && !documentation.contains(keyword))
            documentation.insert(keyword, argument.documentation);
    }

    QList<AssistProposalItemInterface *> list;
    for (const QString &word : words) {
        MarkDownAssitProposalItem *item = new MarkDownAssitProposalItem();
        item->setText(word);
        item->setDetail(documentation.value(word));
        item->setIcon(icon);
        list << item;
    }
    return list;
}

static QList<AssistProposalItemInterface *> generateList(const QMap<QString, FilePath> &words,
                                                         const QIcon &icon)
{
    QList<AssistProposalItemInterface *> list;
    for (auto it = words.cbegin(); it != words.cend(); ++it) {
        MarkDownAssitProposalItem *item = new MarkDownAssitProposalItem();
        item->setText(it.key());
        item->setDocumentationFile(it.value());
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
        if (it->isAdvanced || it->isUnset || it->type == CMakeConfigItem::Type::INTERNAL)
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

            // What the documentation of the name says comes first, and an
            // item that a file documents has not read it yet.
            item->appendDetail(makeDetail(*it));
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
            && interface->position() - startPos < globalCompletionSettings().characterThreshold())
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
        FileFilter({QString("%1*").arg(prefix)}, DirFilterFlag::AllEntries | DirFilterFlag::NoDotAndDotDot));
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

// What the documentation of a command says while it is being written: the
// calls it spells out, with what each argument of them means.
class CMakeFunctionHintModel final : public IFunctionHintProposalModel
{
public:
    CMakeFunctionHintModel(const QStringList &signatures,
                           const QList<CMakeLang::ArgumentDoc> &arguments)
        : m_signatures(signatures)
        , m_arguments(arguments)
    {}

    void reset() final {}
    int size() const final { return m_signatures.size(); }
    QString text(int index) const final;
    int activeArgument(const QString &prefix) const final;

private:
    const CMakeLang::ArgumentDoc *documentationOf(const QString &argument) const;

    QStringList m_signatures;
    QList<CMakeLang::ArgumentDoc> m_arguments;

    // What the cursor stands in, which the widget asks for before it asks
    // for the text to show.
    mutable int m_argument = 0;
    mutable QString m_keyword;
};

const CMakeLang::ArgumentDoc *CMakeFunctionHintModel::documentationOf(
    const QString &argument) const
{
    if (argument.isEmpty())
        return nullptr;

    for (const CMakeLang::ArgumentDoc &candidate : m_arguments) {
        if (candidate.name == argument || candidate.name == '<' + argument + '>'
            || keywordOfArgument(candidate.name) == argument) {
            return &candidate;
        }
    }
    return nullptr;
}

int CMakeFunctionHintModel::activeArgument(const QString &prefix) const
{
    // The call is over once its parenthesis is closed, and so is the hint.
    int depth = 0;
    for (const QChar &c : prefix) {
        if (c == '(')
            ++depth;
        else if (c == ')' && depth-- == 0)
            return -1;
    }

    const QStringList words = prefix.simplified().split(' ', Qt::SkipEmptyParts);

    // A keyword says what the arguments behind it are for, so it is the one
    // to explain for as long as it governs them.
    m_keyword.clear();
    for (const QString &word : words) {
        if (documentationOf(word))
            m_keyword = word;
    }

    m_argument = int(words.size());
    if (!prefix.isEmpty() && prefix.back().isSpace())
        ++m_argument;
    return m_argument;
}

// The keyword a call opens with, which is what tells the signatures of a
// command like file() apart.
static QString modeOf(const QString &signature)
{
    const qsizetype open = signature.indexOf('(');
    if (open < 0)
        return {};

    qsizetype end = open + 1;
    while (end < signature.size()
           && (signature.at(end).isUpper() || signature.at(end).isDigit()
               || signature.at(end) == '_')) {
        ++end;
    }
    if (end - open < 3)
        return {};
    if (end < signature.size() && !signature.at(end).isSpace() && signature.at(end) != ')')
        return {};
    return signature.mid(open + 1, end - open - 1);
}

// The markup of Markdown, as the rich text the hint is shown as.  What is
// read back here is what the renderer of the documentation puts out.
static QString inlineRichText(QStringView text)
{
    QString result;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);

        // A backslash takes the markup off the character behind it.
        if (c == '\\' && i + 1 < text.size()) {
            result += QString(text.at(++i)).toHtmlEscaped();
            continue;
        }

        if (c == '`') {
            const qsizetype end = text.indexOf('`', i + 1);
            if (end > i) {
                result += "<code>" + text.mid(i + 1, end - i - 1).toString().toHtmlEscaped()
                          + "</code>";
                i = end;
                continue;
            }
        }

        if (c == '*') {
            const bool strong = i + 1 < text.size() && text.at(i + 1) == '*';
            const QLatin1String marker(strong ? "**" : "*");
            const qsizetype from = i + marker.size();
            const qsizetype end = text.indexOf(marker, from);
            if (end > from) {
                const QString inner = inlineRichText(text.mid(from, end - from));
                result += strong ? "<b>" + inner + "</b>" : "<i>" + inner + "</i>";
                i = end + marker.size() - 1;
                continue;
            }
        }

        result += QString(c).toHtmlEscaped();
    }
    return result;
}

// What the documentation says in a line: the hint has room for the first
// paragraph of it, and the block of code that opens it is the signature the
// hint shows already.
static QString summaryOf(const QString &markdown)
{
    QStringList paragraph;
    bool code = false;

    const QStringList lines = markdown.split('\n');
    for (const QString &line : lines) {
        QString text = line.trimmed();
        if (text.startsWith("```")) {
            code = !code;
            continue;
        }
        if (code)
            continue;

        if (text.isEmpty()) {
            if (!paragraph.isEmpty())
                break;
            continue;
        }

        // The markers of a quote and of a list say how the text is laid
        // out, not what it says.
        while (text.startsWith("> "))
            text = text.mid(2).trimmed();
        paragraph.append(text);
    }

    return inlineRichText(paragraph.join(' '));
}

// The name of an argument, the brackets that say it is optional removed.
static QString argumentName(const QString &argument)
{
    QString name = argument;
    while (name.startsWith('[') || name.startsWith('{'))
        name = name.mid(1);
    while (name.endsWith(']') || name.endsWith('}') || name.endsWith("..."))
        name.chop(name.endsWith("...") ? 3 : 1);
    return name;
}

QString CMakeFunctionHintModel::text(int index) const
{
    // Once the call names the mode it is in, that is the one the reader is
    // writing, whichever of the signatures the hint stands on.
    QString signature = m_signatures.at(index);
    if (!m_keyword.isEmpty()) {
        for (const QString &candidate : m_signatures) {
            if (modeOf(candidate) == m_keyword) {
                signature = candidate;
                break;
            }
        }
    }

    const qsizetype open = signature.indexOf('(');
    if (open < 0)
        return signature.toHtmlEscaped();

    // The arguments of a call are separated by whitespace, and the one the
    // cursor stands in is the one the reader is writing.
    const QStringList arguments = signature.mid(open + 1, signature.lastIndexOf(')') - open - 1)
                                      .simplified()
                                      .split(' ', Qt::SkipEmptyParts);

    QStringList shown;
    for (int i = 0; i < arguments.size(); ++i) {
        const QString argument = arguments.at(i).toHtmlEscaped();
        shown << (i == m_argument - 1 ? "<b>" + argument + "</b>" : argument);
    }
    QString result = signature.first(open).toHtmlEscaped() + '(' + shown.join(' ') + ')';

    // What the hint explains is the argument the cursor stands in, and what
    // the call does while none is being written.
    QString name = m_keyword;
    if (name.isEmpty() && m_argument > 0 && m_argument <= arguments.size())
        name = argumentName(arguments.at(m_argument - 1));
    if (name.isEmpty())
        name = modeOf(signature);

    if (const CMakeLang::ArgumentDoc *documentation = documentationOf(name)) {
        const QString summary = summaryOf(documentation->documentation);
        if (!summary.isEmpty())
            result += "<p>" + summary + "</p>";
    }
    return result;
}

// The definition of a function or macro the file being edited spells out.
static CMakeLang::CommandAST *definitionOf(const CMakeLang::DocumentPtr &document,
                                           const QString &name)
{
    for (CMakeLang::CommandAST *command : document->commands()) {
        if (!command->isNamed("function") && !command->isNamed("macro"))
            continue;
        CMakeLang::ArgumentAST *argument = command->arguments().first();
        if (argument && CMakeLang::isSameCommand(argument->value(), name))
            return command;
    }
    return nullptr;
}

static QPair<QStringList, QStringList> getLocalFunctionsAndVariables(
    const CMakeLang::DocumentPtr &document)
{
    QStringList variables;
    QStringList functions;
    for (CMakeLang::CommandAST *command : document->commands()) {
        CMakeLang::ArgumentAST *argument = command->arguments().first();
        if (!argument)
            continue;

        if (command->isNamed("macro") || command->isNamed("function"))
            functions << argument->value();
        if (command->isNamed("set") || command->isNamed("option"))
            variables << argument->value();
    }
    return {functions, variables};
}

static void updateCMakeConfigurationWithLocalData(CMakeConfig &cmakeCache,
                                                  const CMakeLang::DocumentPtr &document,
                                                  const FilePath &currentDir)
{
    auto isValidCMakeVariable = [](const QString &var) {
        return var == "CMAKE_PREFIX_PATH" || var == "CMAKE_MODULE_PATH";
    };

    const FilePath projectDir = activeBuildSystemForCurrentProject()
                                    ? activeBuildSystemForCurrentProject()->projectDirectory()
                                    : currentDir;
    auto updateDirVariables = [currentDir, projectDir, cmakeCache](QByteArray &value) {
        value.replace("${CMAKE_CURRENT_SOURCE_DIR}", currentDir.path().toUtf8());
        value.replace("${CMAKE_CURRENT_LIST_DIR}", currentDir.path().toUtf8());
        value.replace("${CMAKE_SOURCE_DIR}", projectDir.path().toUtf8());
        value.replace("${CMAKE_PREFIX_PATH}", cmakeCache.valueOf("CMAKE_PREFIX_PATH"));
        value.replace("${CMAKE_MODULE_PATH}", cmakeCache.valueOf("CMAKE_MODULE_PATH"));
    };

    auto insertOrAppendListValue = [&cmakeCache](const QByteArray &key, const QByteArray &value) {
        if (cmakeCache.contains(key)) {
            CMakeConfigItem &item = cmakeCache[key];
            item.value.append(";");
            item.value.append(value);
        } else {
            cmakeCache.insert(CMakeConfigItem(key, value));
        }
    };

    for (CMakeLang::CommandAST *command : document->commands()) {
        const CMakeLang::ListView<CMakeLang::ArgumentAST *> arguments = command->arguments();
        const bool isSet = command->isNamed("set") && arguments.size() > 1;
        const bool isList = command->isNamed("list") && arguments.size() > 2;
        if (!isSet && !isList)
            continue;

        QByteArray key;
        QByteArray value;
        if (isSet) {
            if (!isValidCMakeVariable(arguments.at(0)->value()))
                continue;
            key = arguments.at(0)->value().toUtf8();
            value = arguments.at(1)->value().toUtf8();
        }
        if (isList) {
            if (arguments.at(0)->value() != "APPEND"
                || !isValidCMakeVariable(arguments.at(1)->value())) {
                continue;
            }
            key = arguments.at(1)->value().toUtf8();
            value = arguments.at(2)->value().toUtf8();
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

        for (const auto &prefix : std::as_const(paths)) {
            // Only search for directories if we have a prefix
            const FilePaths dirs = !m.pathPrefix.isEmpty()
                                       ? prefix.pathAppended(m.pathPrefix)
                                             .dirEntries({{"*"}, DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot})
                                       : FilePaths{prefix};
            const QStringList cmakeFiles
                = Utils::transform<QStringList>(dirs, [](const FilePath &path) {
                      return Utils::transform(path.dirEntries({{"*.cmake"}, DirFilterFlag::Files},
                                                              DirSortFlag::Name),
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
    CMakeLang::SignatureTable signatures;
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

    if (interface()->filePath().isFile())
        data->keywords = CMakeToolManager::defaultProjectOrDefaultCMakeKeyWords();

    if (auto bs = qobject_cast<CMakeBuildSystem *>(activeBuildSystemForCurrentProject())) {
        for (const auto &target : std::as_const(bs->buildTargets()))
            if (target.targetType != TargetType::UtilityType)
                data->buildTargets << target.title;
        const CMakeKeywords &projectKeywords = bs->projectKeywords();
        data->projectVariables = projectKeywords.variables;
        data->projectFunctions = projectKeywords.functions;
        data->signatures = bs->commandSignatures();
        data->importedTargets = bs->projectImportedTargets();
        data->findPackageVariables = bs->projectFindPackageVariables();
        data->cmakeConfiguration = bs->configurationFromCMake();
        data->environment = bs->cmakeBuildConfiguration()->configureEnvironment();
    }

    return data;
}

// The documentation of the command, wherever it is written: in the Help of
// CMake, in the module that provides the command, or in the file that is
// being edited.
static CMakeLang::Documentation documentationFor(const QString &name,
                                                 const PerformInputDataPtr &data,
                                                 const CMakeLang::DocumentPtr &document)
{
    // CMake reads the name of a command without regard to its case, and the
    // name is offered the way it is meant to be written.
    const QMap<QString, FilePath> *maps[]
        = {&data->keywords.functions, &data->projectFunctions};
    for (const QMap<QString, FilePath> *map : maps) {
        auto file = map->constFind(name);
        for (auto it = map->cbegin(); file == map->cend() && it != map->cend(); ++it) {
            if (CMakeLang::isSameCommand(it.key(), name))
                file = it;
        }
        if (file != map->cend() && !file.value().isEmpty())
            return CMakeToolManager::documentation(name, file.value());
    }

    for (const CMakeLang::Documentation &documentation : CMakeLang::documentation(document)) {
        if (documentation.isNamed(name))
            return documentation;
    }
    return {};
}

// What the arguments of a command mean.  A command that hands its arguments
// on to another says nothing about them itself: what that one says about
// them is what they mean, which is how extend_qtc_plugin() takes the
// arguments of extend_qtc_target().  It may hand them on to more than one,
// and may document some of them itself, so each of them has its say.
static QList<CMakeLang::ArgumentDoc> argumentsOf(const CMakeLang::Documentation &documentation,
                                                 const PerformInputDataPtr &data,
                                                 const CMakeLang::DocumentPtr &document,
                                                 const CMakeLang::SignatureTable &local)
{
    QList<CMakeLang::ArgumentDoc> arguments = documentation.arguments();
    if (documentation.name.isEmpty())
        return arguments;

    QStringList forwarded = data->signatures.forwardsTo(documentation.name);
    forwarded += local.forwardsTo(documentation.name);
    forwarded.removeDuplicates();

    for (const QString &command : forwarded) {
        arguments = CMakeLang::mergedArguments(
            arguments, documentationFor(command, data, document).arguments());
    }
    return arguments;
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

// How the command is called, with what the argument that is being written
// means.
IAssistProposal *CMakeFileCompletionAssist::functionHint(
    const QString &functionName,
    const PerformInputDataPtr &data,
    const CMakeLang::DocumentPtr &document,
    const CMakeLang::SignatureTable &local)
{
    if (functionName.isEmpty())
        return nullptr;

    const CMakeLang::Documentation documentation = documentationFor(functionName, data, document);
    QStringList signatures = documentation.signatures();
    if (signatures.isEmpty()) {
        // A function the project defines and does not document still spells
        // its parameters out.
        const QString signature = CMakeLang::definitionSignature(
            definitionOf(document, functionName));
        if (!signature.isEmpty())
            signatures.append(signature);
    }
    if (signatures.isEmpty())
        return nullptr;

    FunctionHintProposalModelPtr model(
        new CMakeFunctionHintModel(signatures, argumentsOf(documentation, data, document, local)));
    return new FunctionHintProposal(findArgumentsStart(interface()), model);
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
        if (isValidIdentifierChar(chr) || wordSize < globalCompletionSettings().characterThreshold())
            return nullptr;
    }

    const CMakeLang::DocumentPtr document = CMakeLang::Document::fromSource(
        interface()->textAt(0, prevFunctionEnd + 1));
    auto [localFunctions, localVariables] = getLocalFunctionsAndVariables(document);

    const CMakeLang::Documentation documentation = functionName.isEmpty()
                                                       ? CMakeLang::Documentation()
                                                       : documentationFor(functionName, data,
                                                                          document);

    CMakeLang::SignatureTable localSignatures;
    localSignatures.addDocument(document);

    if (m_functionHintOnly)
        return functionHint(functionName, data, document, localSignatures);

    CMakeLang::Signature signature = data->signatures.signature(functionName);
    signature.add(localSignatures.signature(functionName));

    CMakeConfig cmakeConfiguration = data->cmakeConfiguration;
    const FilePath currentDir = interface()->filePath().absolutePath();
    updateCMakeConfigurationWithLocalData(cmakeConfiguration, document, currentDir);

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

    const bool knowsArguments = data->keywords.functionArgs.contains(functionName)
                                || !signature.isEmpty();

    // Right behind the parenthesis that opens a call of a command that takes
    // no keywords, what the reader is after is how the command is called.
    // Where it takes keywords, those are what is being written, and the
    // proposal of one says what it is for.
    if (!knowsArguments && interface()->characterAt(interface()->position() - 1) == '(') {
        if (IAssistProposal *hint = functionHint(functionName, data, document, localSignatures))
            return hint;
    }

    if (knowsArguments && !onlyFileItems()) {
        QStringList functionSymbols = data->keywords.functionArgs.value(functionName);
        functionSymbols += signature.keywords();
        functionSymbols.removeDuplicates();
        items.append(generateList(functionSymbols,
                                  m_argsIcon,
                                  argumentsOf(documentation, data, document, localSignatures)));
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

IAssistProcessor *CMakeFunctionHintAssistProvider::createProcessor(const AssistInterface *) const
{
    return new CMakeFileCompletionAssist(/*functionHintOnly=*/true);
}

int CMakeFunctionHintAssistProvider::activationCharSequenceLength() const
{
    return 1;
}

bool CMakeFunctionHintAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return sequence.endsWith("(");
}

CompletionAssistProvider &cmakeFunctionHintAssistProvider()
{
    static CMakeFunctionHintAssistProvider theProvider;
    return theProvider;
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

#ifdef WITH_TESTS

// The documentation is written in Markdown, and the hint is shown as rich
// text: nothing of the one may reach the reader through the other.
class CMakeFunctionHintTest final : public QObject
{
    Q_OBJECT

    // The way CMake documents a command of many modes.
    static CMakeFunctionHintModel fileModel()
    {
        const QStringList signatures
            = {"file(READ <filename> <variable> [OFFSET <offset>] [HEX])",
               "file(WRITE <filename> <content>...)"};
        const QList<CMakeLang::ArgumentDoc> arguments
            = {{"READ",
                "```cmake\nfile(READ <filename> <variable>)\n```\n\nRead content from a file "
                "called `<filename>` and store it in a `<variable>`."},
               {"WRITE",
                "```cmake\nfile(WRITE <filename> <content>...)\n```\n\nWrite `<content>` into "
                "a file called `<filename>`.\n\nMore that the hint has no room for."},
               {"OFFSET <offset>", "Start from the given `<offset>`."}};
        return CMakeFunctionHintModel(signatures, arguments);
    }

private slots:
    // What the hint shows before an argument is written is the call it
    // stands on, and what that call does.
    void testHintOfTheCall()
    {
        const CMakeFunctionHintModel model = fileModel();
        QCOMPARE(model.activeArgument(""), 0);
        QCOMPARE(model.text(0),
                 "file(READ &lt;filename&gt; &lt;variable&gt; [OFFSET &lt;offset&gt;] [HEX])"
                 "<p>Read content from a file called <code>&lt;filename&gt;</code> and store "
                 "it in a <code>&lt;variable&gt;</code>.</p>");
    }

    // Once the call names the mode it is in, that is the signature the
    // reader is writing.
    void testHintOfTheMode()
    {
        const CMakeFunctionHintModel model = fileModel();
        QCOMPARE(model.activeArgument("WRITE "), 2);
        QCOMPARE(model.text(0),
                 "file(WRITE <b>&lt;filename&gt;</b> &lt;content&gt;...)"
                 "<p>Write <code>&lt;content&gt;</code> into a file called "
                 "<code>&lt;filename&gt;</code>.</p>");
    }

    // A keyword that takes a value is documented with the value behind it.
    void testHintOfAKeyword()
    {
        const CMakeFunctionHintModel model = fileModel();
        QCOMPARE(model.activeArgument("READ a b OFFSET"), 4);
        QVERIFY(model.text(0).endsWith("<p>Start from the given <code>&lt;offset&gt;</code>.</p>"));
    }

    // The hint is over once the call is closed.
    void testHintEnds() { QCOMPARE(fileModel().activeArgument("READ a b)"), -1); }

    // A command that documents no argument of that name says nothing.
    void testHintWithoutDocumentation()
    {
        const CMakeFunctionHintModel model({"my_helper(<target> <source>)"}, {});
        QCOMPARE(model.activeArgument(""), 0);
        QCOMPARE(model.text(0), "my_helper(&lt;target&gt; &lt;source&gt;)");
    }
};

QObject *createCMakeFunctionHintTest()
{
    return new CMakeFunctionHintTest;
}

#endif // WITH_TESTS

} // namespace CMakeProjectManager::Internal

#ifdef WITH_TESTS
#include "cmakefilecompletionassist.moc"
#endif
