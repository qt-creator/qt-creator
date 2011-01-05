/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "externaltool.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/actioncontainer.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "variablemanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/environment.h>

#include <QtCore/QXmlStreamReader>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QMenu>
#include <QtGui/QMenuItem>
#include <QtGui/QAction>

#include <QtDebug>

using namespace Core;
using namespace Core::Internal;

namespace {
    const char * const kExternalTool = "externaltool";
    const char * const kDescription = "description";
    const char * const kDisplayName = "displayname";
    const char * const kCategory = "category";
    const char * const kOrder = "order";
    const char * const kExecutable = "executable";
    const char * const kPath = "path";
    const char * const kArguments = "arguments";
    const char * const kInput = "input";
    const char * const kWorkingDirectory = "workingdirectory";

    const char * const kXmlLang = "xml:lang";
    const char * const kOutput = "output";
    const char * const kError = "error";
    const char * const kOutputShowInPane = "showinpane";
    const char * const kOutputReplaceSelection = "replaceselection";
    const char * const kOutputReloadDocument = "reloaddocument";
    const char * const kOutputIgnore = "ignore";
}

// #pragma mark -- ExternalTool

ExternalTool::ExternalTool() :
    m_order(-1),
    m_outputHandling(ShowInPane),
    m_errorHandling(ShowInPane)
{
}

ExternalTool::~ExternalTool()
{
}

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

QStringList ExternalTool::executables() const
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

QString ExternalTool::workingDirectory() const
{
    return m_workingDirectory;
}

ExternalTool::OutputHandling ExternalTool::outputHandling() const
{
    return m_outputHandling;
}

ExternalTool::OutputHandling ExternalTool::errorHandling() const
{
    return m_errorHandling;
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
    if (reader->attributes().hasAttribute(QLatin1String(kXmlLang))) {
        int index = locales.indexOf(reader->attributes().value(QLatin1String(kXmlLang)).toString());
        if (index >= 0 && (index < *currentLocale || *currentLocale < 0)) {
            *currentText = reader->readElementText();
            *currentLocale = index;
        } else {
            reader->skipCurrentElement();
        }
    } else {
        if (*currentLocale < 0 && currentText->isEmpty()) {
            *currentText = reader->readElementText();
        } else {
            reader->skipCurrentElement();
        }
    }
}

static bool parseOutputAttribute(const QString &attribute, QXmlStreamReader *reader, ExternalTool::OutputHandling *value)
{
    const QString output = reader->attributes().value(attribute).toString();
    if (output == QLatin1String(kOutputShowInPane)) {
        *value = ExternalTool::ShowInPane;
    } else if (output == QLatin1String(kOutputReplaceSelection)) {
        *value = ExternalTool::ReplaceSelection;
    } else if (output == QLatin1String(kOutputReloadDocument)) {
        *value = ExternalTool::ReloadDocument;
    } else if (output == QLatin1String(kOutputIgnore)) {
        *value = ExternalTool::Ignore;
    } else {
        reader->raiseError(QLatin1String("Allowed values for output attribute are 'showinpane','replaceselection','reloaddocument'"));
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
    ExternalTool *tool = new ExternalTool;
    QXmlStreamReader reader(xml);

    if (!reader.readNextStartElement() || reader.name() != QLatin1String(kExternalTool))
        reader.raiseError(QLatin1String("Missing start element <externaltool>"));
    tool->m_id = reader.attributes().value(QLatin1String("id")).toString();
    if (tool->m_id.isEmpty())
        reader.raiseError(QLatin1String("Missing or empty id attribute for <externaltool>"));
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String(kDescription)) {
            localizedText(locales, &reader, &descriptionLocale, &tool->m_description);
        } else if (reader.name() == QLatin1String(kDisplayName)) {
            localizedText(locales, &reader, &nameLocale, &tool->m_displayName);
        } else if (reader.name() == QLatin1String(kCategory)) {
            localizedText(locales, &reader, &categoryLocale, &tool->m_displayCategory);
        } else if (reader.name() == QLatin1String(kOrder)) {
            if (tool->m_order >= 0) {
                reader.raiseError(QLatin1String("only one <order> element allowed"));
                break;
            }
            bool ok;
            tool->m_order = reader.readElementText().toInt(&ok);
            if (!ok || tool->m_order < 0)
                reader.raiseError(QLatin1String("<order> element requires non-negative integer value"));
        } else if (reader.name() == QLatin1String(kExecutable)) {
            if (reader.attributes().hasAttribute(QLatin1String(kOutput))) {
                if (!parseOutputAttribute(QLatin1String(kOutput), &reader, &tool->m_outputHandling))
                    break;
            }
            if (reader.attributes().hasAttribute(QLatin1String(kError))) {
                if (!parseOutputAttribute(QLatin1String(kError), &reader, &tool->m_errorHandling))
                    break;
            }
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String(kPath)) {
                    tool->m_executables.append(reader.readElementText());
                } else if (reader.name() == QLatin1String(kArguments)) {
                    if (!tool->m_arguments.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <arguments> element allowed"));
                        break;
                    }
                    tool->m_arguments = reader.readElementText();
                } else if (reader.name() == QLatin1String(kInput)) {
                    if (!tool->m_input.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <input> element allowed"));
                        break;
                    }
                    tool->m_input = reader.readElementText();
                } else if (reader.name() == QLatin1String(kWorkingDirectory)) {
                    if (!tool->m_workingDirectory.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <workingdirectory> element allowed"));
                        break;
                    }
                    tool->m_workingDirectory = reader.readElementText();
                } else {
                    reader.raiseError(QString::fromLatin1("Unknown element <%1> as subelement of <%2>").arg(
                                          reader.qualifiedName().toString(), QLatin1String(kExecutable)));
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
        return 0;
    }
    return tool;
}

// #pragma mark -- ExternalToolRunner

ExternalToolRunner::ExternalToolRunner(const ExternalTool *tool)
    : m_tool(tool),
      m_process(0),
      m_outputCodec(QTextCodec::codecForLocale())
{
    run();
}

bool ExternalToolRunner::resolve()
{
    if (!m_tool)
        return false;
    m_resolvedExecutable = QString::null;
    m_resolvedArguments = QString::null;
    m_resolvedWorkingDirectory = QString::null;
    { // executable
        foreach (const QString &executable, m_tool->executables()) {
            QString resolved = Utils::expandMacros(executable,
                                                   Core::VariableManager::instance()->macroExpander());
            m_resolvedExecutable =
                    Utils::Environment::systemEnvironment().searchInPath(resolved);
        }
        if (m_resolvedExecutable.isEmpty())
            return false;
    }
    { // arguments
        m_resolvedArguments = Utils::expandMacros(m_tool->arguments(),
                                               Core::VariableManager::instance()->macroExpander());
    }
    { // input
        m_resolvedInput = Utils::expandMacros(m_tool->input(),
                                              Core::VariableManager::instance()->macroExpander());
    }
    { // working directory
        m_resolvedWorkingDirectory = Utils::expandMacros(m_tool->workingDirectory(),
                                               Core::VariableManager::instance()->macroExpander());
    }
    return true;
}

void ExternalToolRunner::run()
{
    if (!resolve()) {
        deleteLater();
        return;
    }
    if (m_tool->outputHandling() == ExternalTool::ReloadDocument
               || m_tool->errorHandling() == ExternalTool::ReloadDocument) {
        if (IEditor *editor = EditorManager::instance()->currentEditor()) {
            m_expectedFileName = editor->file()->fileName();
            bool cancelled = false;
            FileManager::instance()->saveModifiedFiles(QList<IFile *>() << editor->file(), &cancelled);
            if (cancelled) {
                deleteLater();
                return;
            }
            FileManager::instance()->expectFileChange(m_expectedFileName);
        }
    }
    m_process = new Utils::QtcProcess(this);
    connect(m_process, SIGNAL(started()), this, SLOT(started()));
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
    if (!m_resolvedWorkingDirectory.isEmpty())
        m_process->setWorkingDirectory(m_resolvedWorkingDirectory);
    m_process->setCommand(m_resolvedExecutable, m_resolvedArguments);
    ICore::instance()->messageManager()->printToOutputPane(
                tr("Starting external tool '%1'").arg(m_resolvedExecutable), false);
    if (!m_resolvedArguments.isEmpty()) {
        ICore::instance()->messageManager()->printToOutputPane(
                tr("with arguments '%1'").arg(m_resolvedArguments), false);
    }
    m_process->start();
}

void ExternalToolRunner::started()
{
    if (!m_resolvedInput.isEmpty()) {
        m_process->write(m_resolvedInput.toLocal8Bit());
    }
    m_process->closeWriteChannel();
}

void ExternalToolRunner::finished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exitCode == 0) {
        if (m_tool->outputHandling() == ExternalTool::ReplaceSelection
                || m_tool->errorHandling() == ExternalTool::ReplaceSelection) {
            emit ExternalToolManager::instance()->replaceSelectionRequested(m_processOutput);
        } else if (m_tool->outputHandling() == ExternalTool::ReloadDocument
                || m_tool->errorHandling() == ExternalTool::ReloadDocument) {
            FileManager::instance()->unexpectFileChange(m_expectedFileName);
        }
    }
    ICore::instance()->messageManager()->printToOutputPane(
                tr("'%1' finished").arg(m_resolvedExecutable), false);
    deleteLater();
}

void ExternalToolRunner::error(QProcess::ProcessError error)
{
    if (m_tool->outputHandling() == ExternalTool::ReloadDocument
            || m_tool->errorHandling() == ExternalTool::ReloadDocument) {
        FileManager::instance()->unexpectFileChange(m_expectedFileName);
    }
    // TODO inform about errors
    deleteLater();
}

void ExternalToolRunner::readStandardOutput()
{
    if (m_tool->outputHandling() == ExternalTool::Ignore)
        return;
    QByteArray data = m_process->readAllStandardOutput();
    QString output = m_outputCodec->toUnicode(data.constData(), data.length(), &m_outputCodecState);
    if (m_tool->outputHandling() == ExternalTool::ShowInPane) {
        ICore::instance()->messageManager()->printToOutputPane(output, true);
    } else if (m_tool->outputHandling() == ExternalTool::ReplaceSelection) {
        m_processOutput.append(output);
    }
}

void ExternalToolRunner::readStandardError()
{
    if (m_tool->errorHandling() == ExternalTool::Ignore)
        return;
    QByteArray data = m_process->readAllStandardError();
    QString output = m_outputCodec->toUnicode(data.constData(), data.length(), &m_errorCodecState);
    if (m_tool->errorHandling() == ExternalTool::ShowInPane) {
        ICore::instance()->messageManager()->printToOutputPane(output, true);
    } else if (m_tool->errorHandling() == ExternalTool::ReplaceSelection) {
        m_processOutput.append(output);
    }
}

// #pragma mark -- ExternalToolManager

ExternalToolManager *ExternalToolManager::m_instance = 0;

ExternalToolManager::ExternalToolManager(Core::ICore *core)
    : QObject(core), m_core(core)
{
    m_instance = this;
    initialize();
}

ExternalToolManager::~ExternalToolManager()
{
    // TODO kill running tools
    qDeleteAll(m_tools);
}

void ExternalToolManager::initialize()
{
    ActionManager *am = m_core->actionManager();
    ActionContainer *mexternaltools = am->createMenu(Id(Constants::M_TOOLS_EXTERNAL));
    mexternaltools->menu()->setTitle(tr("External"));
    ActionContainer *mtools = am->actionContainer(Constants::M_TOOLS);
    Command *cmd;

    mtools->addMenu(mexternaltools, Constants::G_DEFAULT_THREE);

    QMap<QString, QMultiMap<int, Command*> > categoryMenus;
    QDir dir(m_core->resourcePath() + QLatin1String("/externaltools"),
             QLatin1String("*.xml"), QDir::Unsorted, QDir::Files | QDir::Readable);
    foreach (const QFileInfo &info, dir.entryInfoList()) {
        QFile file(info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray &bytes = file.readAll();
            file.close();
            QString error;
            ExternalTool *tool = ExternalTool::createFromXml(bytes, &error, m_core->userInterfaceLanguage());
            if (!tool) {
                // TODO error handling
                qDebug() << tr("Error while parsing external tool %1: %2").arg(file.fileName(), error);
                continue;
            }
            if (m_tools.contains(tool->id())) {
                // TODO error handling
                qDebug() << tr("Error: External tool in %1 has duplicate id").arg(file.fileName());
                delete tool;
                continue;
            }
            m_tools.insert(tool->id(), tool);

            // tool action and command
            QAction *action = new QAction(tool->displayName(), this);
            action->setToolTip(tool->description());
            action->setWhatsThis(tool->description());
            action->setData(tool->id());
            cmd = am->registerAction(action, Id("Tools.External." + tool->id()), Context(Constants::C_GLOBAL));
            connect(action, SIGNAL(triggered()), this, SLOT(menuActivated()));
            categoryMenus[tool->displayCategory()].insert(tool->order(), cmd);
        }
    }

    // add all the category menus, QMap is nicely sorted
    QMapIterator<QString, QMultiMap<int, Command*> > it(categoryMenus);
    while (it.hasNext()) {
        it.next();
        ActionContainer *container = 0;
        if (it.key() == QString()) { // no displayCategory, so put into external tools menu directly
            container = mexternaltools;
        } else {
            container = am->createMenu(Id("Tools.External.Category." + it.key()));
            mexternaltools->addMenu(container, Constants::G_DEFAULT_ONE);
            container->menu()->setTitle(it.key());
        }
        foreach (Command *cmd, it.value().values()) {
            container->addAction(cmd, Constants::G_DEFAULT_TWO);
        }
    }
}

void ExternalToolManager::menuActivated()
{
    QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    ExternalTool *tool = m_tools.value(action->data().toString());
    QTC_ASSERT(tool, return);
    new ExternalToolRunner(tool);
}
