// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaltool.h"

#include "coreplugintr.h"
#include "documentmanager.h"
#include "editormanager/editormanager.h"
#include "externaltoolmanager.h"
#include "icore.h"
#include "idocument.h"
#include "messagemanager.h"

#include <utils/fileutils.h>
#include <utils/globaltasktree.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace QtTaskTree;
using namespace Utils;

namespace Core {

const char kExternalTool[] = "externaltool";
const char kId[] = "id";
const char kDescription[] = "description";
const char kDisplayName[] = "displayname";
const char kCategory[] = "category";
const char kOrder[] = "order";
const char kExecutable[] = "executable";
const char kPath[] = "path";
const char kArguments[] = "arguments";
const char kInput[] = "input";
const char kWorkingDirectory[] = "workingdirectory";
const char kBaseEnvironmentId[] = "baseEnvironmentId";
const char kEnvironment[] = "environment";

const char kXmlLang[] = "xml:lang";
const char kOutput[] = "output";
const char kError[] = "error";
const char kOutputShowInPane[] = "showinpane";
const char kOutputReplaceSelection[] = "replaceselection";
const char kOutputIgnore[] = "ignore";
const char kModifiesDocument[] = "modifiesdocument";
const char kYes[] = "yes";
const char kNo[] = "no";
const char kTrue[] = "true";
const char kFalse[] = "false";

// #pragma mark -- ExternalTool

ExternalTool::ExternalTool() = default;

ExternalTool::ExternalTool(const ExternalTool *other)
    : m_data(other->m_data)
{}

ExternalTool &ExternalTool::operator=(const ExternalTool &other)
{
    m_data = other.m_data;
    return *this;
}

ExternalTool::~ExternalTool() = default;

QString ExternalTool::id() const
{
    return m_data.id;
}

QString ExternalTool::description() const
{
    return m_data.description;
}

QString ExternalTool::displayName() const
{
    return m_data.displayName;
}

QString ExternalTool::displayCategory() const
{
    return m_data.displayCategory;
}

int ExternalTool::order() const
{
    return m_data.order;
}

FilePaths ExternalTool::executables() const
{
    return m_data.executables;
}

QString ExternalTool::arguments() const
{
    return m_data.arguments;
}

QString ExternalTool::input() const
{
    return m_data.input;
}

FilePath ExternalTool::workingDirectory() const
{
    return m_data.workingDirectory;
}

Id ExternalTool::baseEnvironmentProviderId() const
{
    return m_data.baseEnvironmentProviderId;
}

Environment ExternalTool::baseEnvironment() const
{
    if (m_data.baseEnvironmentProviderId.isValid()) {
        const std::optional<EnvironmentProvider> provider = EnvironmentProvider::provider(
            m_data.baseEnvironmentProviderId.toByteArray());
        if (provider && provider->environment)
            return provider->environment();
    }
    return Environment::systemEnvironment();
}

EnvironmentChanges ExternalTool::environmentUserChanges() const
{
    return m_data.environment;
}

ExternalTool::OutputHandling ExternalTool::outputHandling() const
{
    return m_data.outputHandling;
}

ExternalTool::OutputHandling ExternalTool::errorHandling() const
{
    return m_data.errorHandling;
}

bool ExternalTool::modifiesCurrentDocument() const
{
    return m_data.modifiesCurrentDocument;
}

void ExternalTool::setFilePath(const FilePath &filePath)
{
    m_data.filePath = filePath;
}

void ExternalTool::setPreset(std::shared_ptr<ExternalTool> preset)
{
    m_data.presetTool = preset;
}

FilePath ExternalTool::filePath() const
{
    return m_data.filePath;
}

std::shared_ptr<ExternalTool> ExternalTool::preset() const
{
    return m_data.presetTool;
}

void ExternalTool::setId(const QString &id)
{
    m_data.id = id;
}

void ExternalTool::setDisplayCategory(const QString &category)
{
    m_data.displayCategory = category;
}

void ExternalTool::setDisplayName(const QString &name)
{
    m_data.displayName = name;
}

void ExternalTool::setDescription(const QString &description)
{
    m_data.description = description;
}

void ExternalTool::setOutputHandling(OutputHandling handling)
{
    m_data.outputHandling = handling;
}

void ExternalTool::setErrorHandling(OutputHandling handling)
{
    m_data.errorHandling = handling;
}

void ExternalTool::setModifiesCurrentDocument(bool modifies)
{
    m_data.modifiesCurrentDocument = modifies;
}

void ExternalTool::setExecutables(const FilePaths &executables)
{
    m_data.executables = executables;
}

void ExternalTool::setArguments(const QString &arguments)
{
    m_data.arguments = arguments;
}

void ExternalTool::setInput(const QString &input)
{
    m_data.input = input;
}

void ExternalTool::setWorkingDirectory(const FilePath &workingDirectory)
{
    m_data.workingDirectory = workingDirectory;
}

void ExternalTool::setBaseEnvironmentProviderId(Id id)
{
    m_data.baseEnvironmentProviderId = id;
}

void ExternalTool::setEnvironmentUserChanges(const EnvironmentChanges &env)
{
    m_data.environment = env;
}

static QStringList splitLocale(const QString &locale)
{
    QString value = locale;
    QStringList values;
    if (!value.isEmpty())
        values << value;
    int index = value.indexOf(QLatin1Char('.'));
    if (index >= 0) {
        value = value.left(index);
        if (!value.isEmpty())
            values << value;
    }
    index = value.indexOf(QLatin1Char('_'));
    if (index >= 0) {
        value = value.left(index);
        if (!value.isEmpty())
            values << value;
    }
    return values;
}

static void localizedText(const QStringList &locales, QXmlStreamReader *reader, int *currentLocale, QString *currentText)
{
    Q_ASSERT(reader);
    Q_ASSERT(currentLocale);
    Q_ASSERT(currentText);
    if (reader->attributes().hasAttribute(kXmlLang)) {
        int index = locales.indexOf(reader->attributes().value(kXmlLang).toString());
        if (index >= 0 && (index < *currentLocale || *currentLocale < 0)) {
            *currentText = reader->readElementText();
            *currentLocale = index;
        } else {
            reader->skipCurrentElement();
        }
    } else {
        if (*currentLocale < 0 && currentText->isEmpty()) {
            *currentText = Tr::tr(reader->readElementText().toUtf8().constData(), "");
        } else {
            reader->skipCurrentElement();
        }
    }
    if (currentText->isNull()) // prefer isEmpty over isNull
        *currentText = "";
}

static bool parseOutputAttribute(const QString &attribute, QXmlStreamReader *reader, ExternalTool::OutputHandling *value)
{
    const auto output = reader->attributes().value(attribute);
    if (output == QLatin1String(kOutputShowInPane)) {
        *value = ExternalTool::ShowInPane;
    } else if (output == QLatin1String(kOutputReplaceSelection)) {
        *value = ExternalTool::ReplaceSelection;
    } else if (output == QLatin1String(kOutputIgnore)) {
        *value = ExternalTool::Ignore;
    } else {
        reader->raiseError("Allowed values for output attribute are 'showinpane','replaceselection','ignore'");
        return false;
    }
    return true;
}

Result<ExternalTool *> ExternalTool::createFromXml(const QByteArray &xml, const QString &locale)
{
    int descriptionLocale = -1;
    int nameLocale = -1;
    int categoryLocale = -1;
    const QStringList &locales = splitLocale(locale);
    auto tool = new ExternalTool;
    QXmlStreamReader reader(xml);

    if (!reader.readNextStartElement() || reader.name() != QLatin1String(kExternalTool))
        reader.raiseError("Missing start element <externaltool>");
    tool->m_data.id = reader.attributes().value(kId).toString();
    if (tool->m_data.id.isEmpty())
        reader.raiseError("Missing or empty id attribute for <externaltool>");
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String(kDescription)) {
            localizedText(locales, &reader, &descriptionLocale, &tool->m_data.description);
        } else if (reader.name() == QLatin1String(kDisplayName)) {
            localizedText(locales, &reader, &nameLocale, &tool->m_data.displayName);
        } else if (reader.name() == QLatin1String(kCategory)) {
            localizedText(locales, &reader, &categoryLocale, &tool->m_data.displayCategory);
        } else if (reader.name() == QLatin1String(kOrder)) {
            if (tool->m_data.order >= 0) {
                reader.raiseError("only one <order> element allowed");
                break;
            }
            bool ok;
            tool->m_data.order = reader.readElementText().toInt(&ok);
            if (!ok || tool->m_data.order < 0)
                reader.raiseError("<order> element requires non-negative integer value");
        } else if (reader.name() == QLatin1String(kExecutable)) {
            if (reader.attributes().hasAttribute(kOutput)) {
                if (!parseOutputAttribute(kOutput, &reader, &tool->m_data.outputHandling))
                    break;
            }
            if (reader.attributes().hasAttribute(kError)) {
                if (!parseOutputAttribute(kError, &reader, &tool->m_data.errorHandling))
                    break;
            }
            if (reader.attributes().hasAttribute(kModifiesDocument)) {
                const auto value = reader.attributes().value(kModifiesDocument);
                if (value == QLatin1String(kYes) || value == QLatin1String(kTrue)) {
                    tool->m_data.modifiesCurrentDocument = true;
                } else if (value == QLatin1String(kNo) || value == QLatin1String(kFalse)) {
                    tool->m_data.modifiesCurrentDocument = false;
                } else {
                    reader.raiseError("Allowed values for modifiesdocument attribute are 'yes','true','no','false'");
                    break;
                }
            }
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String(kPath)) {
                    tool->m_data.executables.append(FilePath::fromString(reader.readElementText()));
                } else if (reader.name() == QLatin1String(kArguments)) {
                    if (!tool->m_data.arguments.isEmpty()) {
                        reader.raiseError("only one <arguments> element allowed");
                        break;
                    }
                    tool->setArguments(reader.readElementText());
                } else if (reader.name() == QLatin1String(kInput)) {
                    if (!tool->m_data.input.isEmpty()) {
                        reader.raiseError("only one <input> element allowed");
                        break;
                    }
                    tool->setInput(reader.readElementText());
                } else if (reader.name() == QLatin1String(kWorkingDirectory)) {
                    if (!tool->workingDirectory().isEmpty()) {
                        reader.raiseError("only one <workingdirectory> element allowed");
                        break;
                    }
                    tool->m_data.workingDirectory = FilePath::fromString(reader.readElementText());
                } else if (reader.name() == QLatin1String(kBaseEnvironmentId)) {
                    if (tool->m_data.baseEnvironmentProviderId.isValid()) {
                        reader.raiseError("only one <baseEnvironmentId> element allowed");
                        break;
                    }
                    tool->m_data.baseEnvironmentProviderId = Id::fromString(reader.readElementText());
                } else if (reader.name() == QLatin1String(kEnvironment)) {
                    if (tool->m_data.environment.hasData()) {
                        reader.raiseError("only one <environment> element allowed");
                        break;
                    }
                    const QString rawString = reader.readElementText();
                    const QStringList elems = rawString.split("__qtc_sep__");
                    QString itemsString;
                    if (!elems.isEmpty())
                        itemsString = elems.first();
                    else
                        itemsString = rawString;
                    QStringList items = itemsString.split(QLatin1Char(';'));
                    for (auto iter = items.begin(); iter != items.end(); ++iter)
                        *iter = QString::fromUtf8(QByteArray::fromPercentEncoding(iter->toUtf8()));
                    tool->m_data.environment.setItemsFromUser(EnvironmentItem::fromStringList(items));
                    if (elems.size() > 1) {
                        const FilePath file = FilePath::fromString(QString::fromUtf8(
                                QByteArray::fromPercentEncoding(elems.at(1).toUtf8())));
                        bool isScript = false;
                        if (elems.size() == 3)
                            isScript = elems.last() == "true";
                        tool->m_data.environment.setFile(file, isScript);
                    }
                } else {
                    reader.raiseError(QString::fromLatin1("Unknown element <%1> as subelement of <%2>").arg(
                                          reader.qualifiedName().toString(), QString(kExecutable)));
                    break;
                }
            }
        } else {
            reader.raiseError(QString::fromLatin1("Unknown element <%1>").arg(reader.qualifiedName().toString()));
        }
    }
    if (reader.hasError()) {
        delete tool;
        return ResultError(reader.errorString());
    }
    return tool;
}

Result<ExternalTool *> ExternalTool::createFromFile(const FilePath &filePath, const QString &locale)
{
    const Result<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return ResultError(contents.error());
    Result<ExternalTool *> res = ExternalTool::createFromXml(*contents, locale);
    if (!res)
        return ResultError(res.error());
    ExternalTool *tool = *res;
    tool->m_data.filePath = filePath.absoluteFilePath();
    return tool;
}

static QString stringForOutputHandling(ExternalTool::OutputHandling handling)
{
    switch (handling) {
    case ExternalTool::Ignore:
        return QLatin1String(kOutputIgnore);
    case ExternalTool::ShowInPane:
        return QLatin1String(kOutputShowInPane);
    case ExternalTool::ReplaceSelection:
        return QLatin1String(kOutputReplaceSelection);
    }
    return QString();
}

Result<> ExternalTool::save() const
{
    if (filePath().isEmpty())
        return ResultError(ResultAssert); // FIXME: Find something better
    FileSaver saver(filePath());
    if (!saver.hasError()) {
        QXmlStreamWriter out(saver.file());
        out.setAutoFormatting(true);
        out.writeStartDocument("1.0");
        out.writeComment(QString::fromLatin1("Written on %1 by %2")
                         .arg(QDateTime::currentDateTime().toString(), ICore::versionString()));
        out.writeStartElement(kExternalTool);
        out.writeAttribute(kId, id());
        out.writeTextElement(kDescription, description());
        out.writeTextElement(kDisplayName, displayName());
        out.writeTextElement(kCategory, displayCategory());
        if (order() != -1)
            out.writeTextElement(kOrder, QString::number(order()));

        out.writeStartElement(kExecutable);
        out.writeAttribute(kOutput, stringForOutputHandling(outputHandling()));
        out.writeAttribute(kError, stringForOutputHandling(errorHandling()));
        out.writeAttribute(kModifiesDocument, QLatin1String(modifiesCurrentDocument() ? kYes : kNo));
        for (const FilePath &executable : executables())
            out.writeTextElement(kPath, executable.toUrlishString());
        if (!arguments().isEmpty())
            out.writeTextElement(kArguments, arguments());
        if (!input().isEmpty())
            out.writeTextElement(kInput, input());
        if (!workingDirectory().isEmpty())
            out.writeTextElement(kWorkingDirectory, workingDirectory().toUrlishString());
        if (baseEnvironmentProviderId().isValid())
            out.writeTextElement(kBaseEnvironmentId, baseEnvironmentProviderId().toString());
        if (environmentUserChanges().hasData()) {
            QStringList envLines = EnvironmentItem::toStringList(environmentUserChanges().itemsFromUser());
            for (auto iter = envLines.begin(); iter != envLines.end(); ++iter)
                *iter = QString::fromUtf8(iter->toUtf8().toPercentEncoding());
            const QString envString = envLines.join(QLatin1Char(';'));
            const QString fileString = QString::fromUtf8(
                environmentUserChanges().file().toFSPathString().toUtf8().toPercentEncoding());
            const QString isScriptString = environmentUserChanges().isScript()
                ? QLatin1String("true") : QLatin1String("false");
            out.writeTextElement(
                kEnvironment,
                envString + "__qtc_sep__" + fileString + "__qtc_sep__" + isScriptString);
        }
        out.writeEndElement();

        out.writeEndDocument();

        saver.setResult(&out);
    }
    return saver.finalize();
}

bool ExternalTool::operator==(const ExternalTool &other) const
{
    return m_data.id == other.m_data.id
           && m_data.description == other.m_data.description
           && m_data.displayName == other.m_data.displayName
           && m_data.displayCategory == other.m_data.displayCategory
           && m_data.order == other.m_data.order
           && m_data.executables == other.m_data.executables
           && m_data.arguments == other.m_data.arguments
           && m_data.input == other.m_data.input
           && m_data.workingDirectory == other.m_data.workingDirectory
           && m_data.baseEnvironmentProviderId == other.m_data.baseEnvironmentProviderId
           && m_data.environment == other.m_data.environment
           && m_data.outputHandling == other.m_data.outputHandling
           && m_data.modifiesCurrentDocument == other.m_data.modifiesCurrentDocument
           && m_data.errorHandling == other.m_data.errorHandling
           && m_data.filePath == other.m_data.filePath;
}

struct ExecuteData
{
    ExecuteData(const ExternalTool *tool) : m_tool(tool) {}
    Result<> resolve();
    void readStandardOutput(const QString &output);
    void readStandardError(const QString &error);

    const ExternalTool m_tool;
    FilePath m_resolvedExecutable;
    QString m_resolvedArguments;
    QString m_resolvedInput;
    FilePath m_resolvedWorkingDirectory;
    Environment m_resolvedEnvironment;
    QString m_processOutput;
    FilePath m_expectedFilePath;
};

Result<> ExecuteData::resolve()
{
    m_resolvedEnvironment = m_tool.baseEnvironment();

    MacroExpander *expander = globalMacroExpander();
    m_tool.environmentUserChanges().modifyEnvironment(m_resolvedEnvironment, expander);

    FilePaths expandedExecutables; /* for error message */
    const FilePaths executables = m_tool.executables();
    for (const FilePath &executable : executables) {
        FilePath expanded = expander->expand(executable);
        expandedExecutables.append(expanded);
        m_resolvedExecutable = m_resolvedEnvironment.searchInPath(expanded.path());
        if (!m_resolvedExecutable.isEmpty())
            break;
    }
    if (m_resolvedExecutable.isEmpty()) {
        QString error;
        for (int i = 0; i < expandedExecutables.size(); ++i) {
            error += Tr::tr("Could not find executable for \"%1\" (expanded \"%2\")")
            .arg(m_tool.executables().at(i).toUserOutput(),
                 expandedExecutables.at(i).toUserOutput());
            error += QLatin1Char('\n');
        }
        if (!error.isEmpty())
            error.chop(1);
        return ResultError(error);
    }

    const Result<QString> args = expander->expandProcessArgs(m_tool.arguments());
    QTC_ASSERT_RESULT(args, return ResultError(args.error()));

    m_resolvedArguments = *args;
    m_resolvedInput = expander->expand(m_tool.input());
    m_resolvedWorkingDirectory = expander->expand(m_tool.workingDirectory());
    return ResultOk;
}

static QString stripNewline(const QString &output)
{
    if (output.endsWith('\n'))
        return output.chopped(1);
    return output;
}

void ExecuteData::readStandardOutput(const QString &output)
{
    if (m_tool.outputHandling() == ExternalTool::Ignore)
        return;
    if (m_tool.outputHandling() == ExternalTool::ShowInPane)
        MessageManager::writeSilently(stripNewline(output));
    else if (m_tool.outputHandling() == ExternalTool::ReplaceSelection)
        m_processOutput.append(output);
}

void ExecuteData::readStandardError(const QString &error)
{
    if (m_tool.errorHandling() == ExternalTool::Ignore)
        return;
    if (m_tool.errorHandling() == ExternalTool::ShowInPane)
        MessageManager::writeSilently(stripNewline(error));
    else if (m_tool.errorHandling() == ExternalTool::ReplaceSelection)
        m_processOutput.append(error);
}

void ExternalTool::execute() const
{
    const Storage<ExecuteData> storage(this);

    const auto onSetup = [storage] {
        ExecuteData *data = storage.activeStorage();
        if (const Result<> res = data->resolve(); !res) {
            MessageManager::writeFlashing(res.error());
            return SetupResult::StopWithError;
        }

        if (data->m_tool.modifiesCurrentDocument()) {
            if (IDocument *document = EditorManager::currentDocument()) {
                data->m_expectedFilePath = document->filePath();
                if (!DocumentManager::saveModifiedDocument(document))
                    return SetupResult::StopWithSuccess;

                DocumentManager::expectFileChange(data->m_expectedFilePath);
            }
        }
        return SetupResult::Continue;
    };

    const auto onProcessSetup = [storage](Process &process) {
        ExecuteData *data = storage.activeStorage();
        process.setStdOutLineCallback([data](const QString &s) { data->readStandardOutput(s); });
        process.setStdErrLineCallback([data](const QString &s) { data->readStandardError(s); });
        if (!data->m_resolvedWorkingDirectory.isEmpty())
            process.setWorkingDirectory(data->m_resolvedWorkingDirectory);
        const CommandLine cmd{data->m_resolvedExecutable, data->m_resolvedArguments, CommandLine::Raw};
        process.setCommand(cmd);
        Environment env = data->m_resolvedEnvironment;
        // force Qt to log to std streams, if it's not explicitly been set differently
        if (!env.hasKey("QT_LOGGING_TO_CONSOLE"))
            env.set("QT_LOGGING_TO_CONSOLE", "1");
        if (!env.hasKey("QT_FORCE_STDERR_LOGGING"))
            env.set("QT_FORCE_STDERR_LOGGING", "1");

        process.setEnvironment(env);
        const auto write = data->m_tool.outputHandling() == ExternalTool::ShowInPane
                               ? QOverload<const QString &>::of(MessageManager::writeDisrupting)
                               : QOverload<const QString &>::of(MessageManager::writeSilently);
        write(Tr::tr("Starting external tool \"%1\"").arg(cmd.toUserOutput()));
        if (!data->m_resolvedInput.isEmpty())
            process.setWriteData(data->m_resolvedInput.toLocal8Bit());
    };
    const auto onProcessDone = [storage](const Process &process) {
        ExecuteData *data = storage.activeStorage();
        if (process.result() == ProcessResult::FinishedWithSuccess
            && (data->m_tool.outputHandling() == ExternalTool::ReplaceSelection
                || data->m_tool.errorHandling() == ExternalTool::ReplaceSelection)) {
            ExternalToolManager::emitReplaceSelectionRequested(data->m_processOutput);
        }
        const QString message = (process.result() == ProcessResult::FinishedWithSuccess)
                                    ? Tr::tr("\"%1\" finished").arg(data->m_resolvedExecutable.toUserOutput())
                                    : Tr::tr("\"%1\" finished with error").arg(data->m_resolvedExecutable.toUserOutput());

        if (data->m_tool.modifiesCurrentDocument())
            DocumentManager::unexpectFileChange(data->m_expectedFilePath);
        const auto write = data->m_tool.outputHandling() == ExternalTool::ShowInPane
                               ? QOverload<const QString &>::of(MessageManager::writeFlashing)
                               : QOverload<const QString &>::of(MessageManager::writeSilently);
        write(message);
    };

    const Group recipe {
        storage,
        onGroupSetup(onSetup),
        ProcessTask(onProcessSetup, onProcessDone)
    };

    GlobalTaskTree::start(recipe);
}

} // namespace Core
