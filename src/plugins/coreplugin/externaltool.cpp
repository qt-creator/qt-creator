// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaltool.h"

#include "coreplugintr.h"
#include "externaltoolmanager.h"
#include "icore.h"
#include "idocument.h"
#include "messagemanager.h"
#include "documentmanager.h"
#include "editormanager/editormanager.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace Utils;
using namespace Core::Internal;

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

ExternalTool::ExternalTool() :
    m_displayCategory("") // difference between isNull and isEmpty
{
}

ExternalTool::ExternalTool(const ExternalTool *other)
    : m_id(other->m_id),
      m_description(other->m_description),
      m_displayName(other->m_displayName),
      m_displayCategory(other->m_displayCategory),
      m_order(other->m_order),
      m_executables(other->m_executables),
      m_arguments(other->m_arguments),
      m_input(other->m_input),
      m_workingDirectory(other->m_workingDirectory),
      m_baseEnvironmentProviderId(other->m_baseEnvironmentProviderId),
      m_environment(other->m_environment),
      m_outputHandling(other->m_outputHandling),
      m_errorHandling(other->m_errorHandling),
      m_modifiesCurrentDocument(other->m_modifiesCurrentDocument),
      m_filePath(other->m_filePath),
      m_presetTool(other->m_presetTool)
{
}

ExternalTool &ExternalTool::operator=(const ExternalTool &other)
{
    m_id = other.m_id;
    m_description = other.m_description;
    m_displayName = other.m_displayName;
    m_displayCategory = other.m_displayCategory;
    m_order = other.m_order;
    m_executables = other.m_executables;
    m_arguments = other.m_arguments;
    m_input = other.m_input;
    m_workingDirectory = other.m_workingDirectory;
    m_environment = other.m_environment;
    m_outputHandling = other.m_outputHandling;
    m_errorHandling = other.m_errorHandling;
    m_modifiesCurrentDocument = other.m_modifiesCurrentDocument;
    m_filePath = other.m_filePath;
    m_presetFileName = other.m_presetFileName;
    m_presetTool = other.m_presetTool;
    return *this;
}

ExternalTool::~ExternalTool() = default;

QString ExternalTool::id() const
{
    return m_id;
}

QString ExternalTool::description() const
{
    return m_description;
}

QString ExternalTool::displayName() const
{
    return m_displayName;
}

QString ExternalTool::displayCategory() const
{
    return m_displayCategory;
}

int ExternalTool::order() const
{
    return m_order;
}

FilePaths ExternalTool::executables() const
{
    return m_executables;
}

QString ExternalTool::arguments() const
{
    return m_arguments;
}

QString ExternalTool::input() const
{
    return m_input;
}

FilePath ExternalTool::workingDirectory() const
{
    return m_workingDirectory;
}

Id ExternalTool::baseEnvironmentProviderId() const
{
    return m_baseEnvironmentProviderId;
}

Environment ExternalTool::baseEnvironment() const
{
    if (m_baseEnvironmentProviderId.isValid()) {
        const std::optional<EnvironmentProvider> provider = EnvironmentProvider::provider(
            m_baseEnvironmentProviderId.name());
        if (provider && provider->environment)
            return provider->environment();
    }
    return Environment::systemEnvironment();
}

EnvironmentItems ExternalTool::environmentUserChanges() const
{
    return m_environment;
}

ExternalTool::OutputHandling ExternalTool::outputHandling() const
{
    return m_outputHandling;
}

ExternalTool::OutputHandling ExternalTool::errorHandling() const
{
    return m_errorHandling;
}

bool ExternalTool::modifiesCurrentDocument() const
{
    return m_modifiesCurrentDocument;
}

void ExternalTool::setFilePath(const FilePath &filePath)
{
    m_filePath = filePath;
}

void ExternalTool::setPreset(QSharedPointer<ExternalTool> preset)
{
    m_presetTool = preset;
}

FilePath ExternalTool::filePath() const
{
    return m_filePath;
}

QSharedPointer<ExternalTool> ExternalTool::preset() const
{
    return m_presetTool;
}

void ExternalTool::setId(const QString &id)
{
    m_id = id;
}

void ExternalTool::setDisplayCategory(const QString &category)
{
    m_displayCategory = category;
}

void ExternalTool::setDisplayName(const QString &name)
{
    m_displayName = name;
}

void ExternalTool::setDescription(const QString &description)
{
    m_description = description;
}

void ExternalTool::setOutputHandling(OutputHandling handling)
{
    m_outputHandling = handling;
}

void ExternalTool::setErrorHandling(OutputHandling handling)
{
    m_errorHandling = handling;
}

void ExternalTool::setModifiesCurrentDocument(bool modifies)
{
    m_modifiesCurrentDocument = modifies;
}

void ExternalTool::setExecutables(const FilePaths &executables)
{
    m_executables = executables;
}

void ExternalTool::setArguments(const QString &arguments)
{
    m_arguments = arguments;
}

void ExternalTool::setInput(const QString &input)
{
    m_input = input;
}

void ExternalTool::setWorkingDirectory(const FilePath &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

void ExternalTool::setBaseEnvironmentProviderId(Id id)
{
    m_baseEnvironmentProviderId = id;
}

void ExternalTool::setEnvironmentUserChanges(const EnvironmentItems &items)
{
    m_environment = items;
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

ExternalTool * ExternalTool::createFromXml(const QByteArray &xml, QString *errorMessage, const QString &locale)
{
    int descriptionLocale = -1;
    int nameLocale = -1;
    int categoryLocale = -1;
    const QStringList &locales = splitLocale(locale);
    auto tool = new ExternalTool;
    QXmlStreamReader reader(xml);

    if (!reader.readNextStartElement() || reader.name() != QLatin1String(kExternalTool))
        reader.raiseError("Missing start element <externaltool>");
    tool->m_id = reader.attributes().value(kId).toString();
    if (tool->m_id.isEmpty())
        reader.raiseError("Missing or empty id attribute for <externaltool>");
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String(kDescription)) {
            localizedText(locales, &reader, &descriptionLocale, &tool->m_description);
        } else if (reader.name() == QLatin1String(kDisplayName)) {
            localizedText(locales, &reader, &nameLocale, &tool->m_displayName);
        } else if (reader.name() == QLatin1String(kCategory)) {
            localizedText(locales, &reader, &categoryLocale, &tool->m_displayCategory);
        } else if (reader.name() == QLatin1String(kOrder)) {
            if (tool->m_order >= 0) {
                reader.raiseError("only one <order> element allowed");
                break;
            }
            bool ok;
            tool->m_order = reader.readElementText().toInt(&ok);
            if (!ok || tool->m_order < 0)
                reader.raiseError("<order> element requires non-negative integer value");
        } else if (reader.name() == QLatin1String(kExecutable)) {
            if (reader.attributes().hasAttribute(kOutput)) {
                if (!parseOutputAttribute(kOutput, &reader, &tool->m_outputHandling))
                    break;
            }
            if (reader.attributes().hasAttribute(kError)) {
                if (!parseOutputAttribute(kError, &reader, &tool->m_errorHandling))
                    break;
            }
            if (reader.attributes().hasAttribute(kModifiesDocument)) {
                const auto value = reader.attributes().value(kModifiesDocument);
                if (value == QLatin1String(kYes) || value == QLatin1String(kTrue)) {
                    tool->m_modifiesCurrentDocument = true;
                } else if (value == QLatin1String(kNo) || value == QLatin1String(kFalse)) {
                    tool->m_modifiesCurrentDocument = false;
                } else {
                    reader.raiseError("Allowed values for modifiesdocument attribute are 'yes','true','no','false'");
                    break;
                }
            }
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String(kPath)) {
                    tool->m_executables.append(FilePath::fromString(reader.readElementText()));
                } else if (reader.name() == QLatin1String(kArguments)) {
                    if (!tool->m_arguments.isEmpty()) {
                        reader.raiseError("only one <arguments> element allowed");
                        break;
                    }
                    tool->m_arguments = reader.readElementText();
                } else if (reader.name() == QLatin1String(kInput)) {
                    if (!tool->m_input.isEmpty()) {
                        reader.raiseError("only one <input> element allowed");
                        break;
                    }
                    tool->m_input = reader.readElementText();
                } else if (reader.name() == QLatin1String(kWorkingDirectory)) {
                    if (!tool->m_workingDirectory.isEmpty()) {
                        reader.raiseError("only one <workingdirectory> element allowed");
                        break;
                    }
                    tool->m_workingDirectory = FilePath::fromString(reader.readElementText());
                } else if (reader.name() == QLatin1String(kBaseEnvironmentId)) {
                    if (tool->m_baseEnvironmentProviderId.isValid()) {
                        reader.raiseError("only one <baseEnvironmentId> element allowed");
                        break;
                    }
                    tool->m_baseEnvironmentProviderId = Id::fromString(reader.readElementText());
                } else if (reader.name() == QLatin1String(kEnvironment)) {
                    if (!tool->m_environment.isEmpty()) {
                        reader.raiseError("only one <environment> element allowed");
                        break;
                    }
                    QStringList lines = reader.readElementText().split(QLatin1Char(';'));
                    for (auto iter = lines.begin(); iter != lines.end(); ++iter)
                        *iter = QString::fromUtf8(QByteArray::fromPercentEncoding(iter->toUtf8()));
                    tool->m_environment = EnvironmentItem::fromStringList(lines);
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
        if (errorMessage)
            *errorMessage = reader.errorString();
        delete tool;
        return nullptr;
    }
    return tool;
}

ExternalTool * ExternalTool::createFromFile(const FilePath &fileName, QString *errorMessage,
                                            const QString &locale)
{
    FilePath absFileName = fileName.absoluteFilePath();
    FileReader reader;
    if (!reader.fetch(absFileName, errorMessage))
        return nullptr;
    ExternalTool *tool = ExternalTool::createFromXml(reader.data(), errorMessage, locale);
    if (!tool)
        return nullptr;
    tool->m_filePath = absFileName;
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

bool ExternalTool::save(QString *errorMessage) const
{
    if (m_filePath.isEmpty())
        return false;
    FileSaver saver(m_filePath);
    if (!saver.hasError()) {
        QXmlStreamWriter out(saver.file());
        out.setAutoFormatting(true);
        out.writeStartDocument("1.0");
        out.writeComment(QString::fromLatin1("Written on %1 by %2")
                         .arg(QDateTime::currentDateTime().toString(), ICore::versionString()));
        out.writeStartElement(kExternalTool);
        out.writeAttribute(kId, m_id);
        out.writeTextElement(kDescription, m_description);
        out.writeTextElement(kDisplayName, m_displayName);
        out.writeTextElement(kCategory, m_displayCategory);
        if (m_order != -1)
            out.writeTextElement(kOrder, QString::number(m_order));

        out.writeStartElement(kExecutable);
        out.writeAttribute(kOutput, stringForOutputHandling(m_outputHandling));
        out.writeAttribute(kError, stringForOutputHandling(m_errorHandling));
        out.writeAttribute(kModifiesDocument, QLatin1String(m_modifiesCurrentDocument ? kYes : kNo));
        for (const FilePath &executable : m_executables)
            out.writeTextElement(kPath, executable.toString());
        if (!m_arguments.isEmpty())
            out.writeTextElement(kArguments, m_arguments);
        if (!m_input.isEmpty())
            out.writeTextElement(kInput, m_input);
        if (!m_workingDirectory.isEmpty())
            out.writeTextElement(kWorkingDirectory, m_workingDirectory.toString());
        if (m_baseEnvironmentProviderId.isValid())
            out.writeTextElement(kBaseEnvironmentId, m_baseEnvironmentProviderId.toString());
        if (!m_environment.isEmpty()) {
            QStringList envLines = EnvironmentItem::toStringList(m_environment);
            for (auto iter = envLines.begin(); iter != envLines.end(); ++iter)
                *iter = QString::fromUtf8(iter->toUtf8().toPercentEncoding());
            out.writeTextElement(kEnvironment, envLines.join(QLatin1Char(';')));
        }
        out.writeEndElement();

        out.writeEndDocument();

        saver.setResult(&out);
    }
    return saver.finalize(errorMessage);
}

bool ExternalTool::operator==(const ExternalTool &other) const
{
    return m_id == other.m_id
            && m_description == other.m_description
            && m_displayName == other.m_displayName
            && m_displayCategory == other.m_displayCategory
            && m_order == other.m_order
            && m_executables == other.m_executables
            && m_arguments == other.m_arguments
            && m_input == other.m_input
            && m_workingDirectory == other.m_workingDirectory
            && m_baseEnvironmentProviderId == other.m_baseEnvironmentProviderId
            && m_environment == other.m_environment
            && m_outputHandling == other.m_outputHandling
            && m_modifiesCurrentDocument == other.m_modifiesCurrentDocument
            && m_errorHandling == other.m_errorHandling
            && m_filePath == other.m_filePath;
}

// #pragma mark -- ExternalToolRunner

ExternalToolRunner::ExternalToolRunner(const ExternalTool *tool)
    : m_tool(new ExternalTool(tool)),
      m_process(nullptr),
      m_outputCodec(QTextCodec::codecForLocale()),
      m_hasError(false)
{
    run();
}

ExternalToolRunner::~ExternalToolRunner()
{
    if (m_tool)
        delete m_tool;
}

bool ExternalToolRunner::hasError() const
{
    return m_hasError;
}

QString ExternalToolRunner::errorString() const
{
    return m_errorString;
}

bool ExternalToolRunner::resolve()
{
    if (!m_tool)
        return false;
    m_resolvedExecutable.clear();
    m_resolvedArguments.clear();
    m_resolvedWorkingDirectory.clear();
    m_resolvedEnvironment = m_tool->baseEnvironment();

    MacroExpander *expander = globalMacroExpander();
    EnvironmentItems expandedEnvironment = Utils::transform(
        m_tool->environmentUserChanges(), [expander](const EnvironmentItem &item) {
            return EnvironmentItem(item.name, expander->expand(item.value), item.operation);
        });
    m_resolvedEnvironment.modify(expandedEnvironment);

    {
        // executable
        FilePaths expandedExecutables; /* for error message */
        const FilePaths executables = m_tool->executables();
        for (const FilePath &executable : executables) {
            FilePath expanded = expander->expand(executable);
            expandedExecutables.append(expanded);
            m_resolvedExecutable = m_resolvedEnvironment.searchInPath(expanded.path());
            if (!m_resolvedExecutable.isEmpty())
                break;
        }
        if (m_resolvedExecutable.isEmpty()) {
            m_hasError = true;
            for (int i = 0; i < expandedExecutables.size(); ++i) {
                m_errorString += Tr::tr("Could not find executable for \"%1\" (expanded \"%2\")")
                        .arg(m_tool->executables().at(i).toUserOutput(),
                             expandedExecutables.at(i).toUserOutput());
                m_errorString += QLatin1Char('\n');
            }
            if (!m_errorString.isEmpty())
                m_errorString.chop(1);
            return false;
        }
    }

    m_resolvedArguments = expander->expandProcessArgs(m_tool->arguments());
    m_resolvedInput = expander->expand(m_tool->input());
    m_resolvedWorkingDirectory = expander->expand(m_tool->workingDirectory());

    return true;
}

void ExternalToolRunner::run()
{
    if (!resolve()) {
        deleteLater();
        return;
    }
    if (m_tool->modifiesCurrentDocument()) {
        if (IDocument *document = EditorManager::currentDocument()) {
            m_expectedFilePath = document->filePath();
            if (!DocumentManager::saveModifiedDocument(document)) {
                deleteLater();
                return;
            }
            DocumentManager::expectFileChange(m_expectedFilePath);
        }
    }
    m_process = new Process(this);
    connect(m_process, &Process::done, this, &ExternalToolRunner::done);
    m_process->setStdOutLineCallback([this](const QString &s) { readStandardOutput(s); });
    m_process->setStdErrLineCallback([this](const QString &s) { readStandardError(s); });
    if (!m_resolvedWorkingDirectory.isEmpty())
        m_process->setWorkingDirectory(m_resolvedWorkingDirectory);
    const CommandLine cmd{m_resolvedExecutable, m_resolvedArguments, CommandLine::Raw};
    m_process->setCommand(cmd);
    Environment env = m_resolvedEnvironment;
    // force Qt to log to std streams, if it's not explicitly been set differently
    if (!env.hasKey("QT_LOGGING_TO_CONSOLE"))
        env.set("QT_LOGGING_TO_CONSOLE", "1");
    m_process->setEnvironment(env);
    const auto write = m_tool->outputHandling() == ExternalTool::ShowInPane
                           ? QOverload<const QString &>::of(MessageManager::writeDisrupting)
                           : QOverload<const QString &>::of(MessageManager::writeSilently);
    write(Tr::tr("Starting external tool \"%1\"").arg(cmd.toUserOutput()));
    if (!m_resolvedInput.isEmpty())
        m_process->setWriteData(m_resolvedInput.toLocal8Bit());
    m_process->start();
}

void ExternalToolRunner::done()
{
    if (m_process->result() == ProcessResult::FinishedWithSuccess
            && (m_tool->outputHandling() == ExternalTool::ReplaceSelection
                || m_tool->errorHandling() == ExternalTool::ReplaceSelection)) {
        ExternalToolManager::emitReplaceSelectionRequested(m_processOutput);
    }
    const QString message = (m_process->result() == ProcessResult::FinishedWithSuccess)
            ? Tr::tr("\"%1\" finished").arg(m_resolvedExecutable.toUserOutput())
            : Tr::tr("\"%1\" finished with error").arg(m_resolvedExecutable.toUserOutput());

    if (m_tool->modifiesCurrentDocument())
        DocumentManager::unexpectFileChange(m_expectedFilePath);
    const auto write = m_tool->outputHandling() == ExternalTool::ShowInPane
                           ? QOverload<const QString &>::of(MessageManager::writeFlashing)
                           : QOverload<const QString &>::of(MessageManager::writeSilently);
    write(message);
    deleteLater();
}

static QString stripNewline(const QString &output)
{
    if (output.endsWith('\n'))
        return output.chopped(1);
    return output;
}

void ExternalToolRunner::readStandardOutput(const QString &output)
{
    if (m_tool->outputHandling() == ExternalTool::Ignore)
        return;
    if (m_tool->outputHandling() == ExternalTool::ShowInPane)
        MessageManager::writeSilently(stripNewline(output));
    else if (m_tool->outputHandling() == ExternalTool::ReplaceSelection)
        m_processOutput.append(output);
}

void ExternalToolRunner::readStandardError(const QString &output)
{
    if (m_tool->errorHandling() == ExternalTool::Ignore)
        return;
    if (m_tool->errorHandling() == ExternalTool::ShowInPane)
        MessageManager::writeSilently(stripNewline(output));
    else if (m_tool->errorHandling() == ExternalTool::ReplaceSelection)
        m_processOutput.append(output);
}

} // namespace Core
