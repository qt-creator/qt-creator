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
#include "actionmanager.h"
#include "actioncontainer.h"
#include "command.h"
#include "coreconstants.h"
#include "variablemanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

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
    const char * const kWorkingDirectory = "workingdirectory";

    const char * const kXmlLang = "xml:lang";
    const char * const kOutput = "output";
    const char * const kOutputShowInPane = "showinpane";
    const char * const kOutputReplaceSelection = "replaceselection";
    const char * const kOutputReloadDocument = "reloaddocument";
}

// #pragma mark -- ExternalTool

ExternalTool::ExternalTool() :
    m_order(-1),
    m_outputHandling(ShowInPane)
{
}

ExternalTool::~ExternalTool()
{
    // TODO kill running process
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

QString ExternalTool::workingDirectory() const
{
    return m_workingDirectory;
}

ExternalTool::OutputHandling ExternalTool::outputHandling() const
{
    return m_outputHandling;
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
                const QString output = reader.attributes().value(QLatin1String(kOutput)).toString();
                if (output == QLatin1String(kOutputShowInPane)) {
                    tool->m_outputHandling = ExternalTool::ShowInPane;
                } else if (output == QLatin1String(kOutputReplaceSelection)) {
                    tool->m_outputHandling = ExternalTool::ReplaceSelection;
                } else if (output == QLatin1String(kOutputReloadDocument)) {
                    tool->m_outputHandling = ExternalTool::ReloadDocument;
                } else {
                    reader.raiseError(QLatin1String("Allowed values for output attribute are 'showinpane','replaceselection','reloaddocument'"));
                    break;
                }
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
    m_resolvedArguments.clear();
    m_resolvedWorkingDirectory = QString::null;
    { // executable
        foreach (const QString &executable, m_tool->executables()) {
            QString resolved = Utils::expandMacros(executable,
                                                   Core::VariableManager::instance()->macroExpander());
            QFileInfo info(resolved);
            // TODO search in path
            if (info.exists() && info.isExecutable()) {
                m_resolvedExecutable = resolved;
                break;
            }
        }
        if (m_resolvedExecutable.isNull())
            return false;
    }
    { // arguments
        QString resolved = Utils::expandMacros(m_tool->arguments(),
                                               Core::VariableManager::instance()->macroExpander());
        // TODO stupid, do it right
        m_resolvedArguments = resolved.split(QLatin1Char(' '), QString::SkipEmptyParts);
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
    m_process = new QProcess;
    // TODO error handling, finish reporting, reading output, etc
    connect(m_process, SIGNAL(finished(int)), this, SLOT(finished()));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
    if (!m_resolvedWorkingDirectory.isEmpty())
        m_process->setWorkingDirectory(m_resolvedWorkingDirectory);
    m_process->start(m_resolvedExecutable, m_resolvedArguments, QIODevice::ReadOnly);
}

void ExternalToolRunner::finished()
{
    // TODO handle the ReplaceSelection and ReloadDocument flags
    delete m_process;
    m_process = 0;
    deleteLater();
}

void ExternalToolRunner::error(QProcess::ProcessError error)
{
    // TODO inform about errors
    delete m_process;
    m_process = 0;
    deleteLater();
}

void ExternalToolRunner::readStandardOutput()
{
    QByteArray data = m_process->readAllStandardOutput();
    QString output = m_outputCodec->toUnicode(data.constData(), data.length(), &m_outputCodecState);
    // TODO handle the ReplaceSelection flag
    if (m_tool->outputHandling() == ExternalTool::ShowInPane) {
        ICore::instance()->messageManager()->printToOutputPane(output, true);
    }
}

void ExternalToolRunner::readStandardError()
{
    QByteArray data = m_process->readAllStandardError();
    QString output = m_outputCodec->toUnicode(data.constData(), data.length(), &m_errorCodecState);
    // TODO handle the ReplaceSelection flag
    if (m_tool->outputHandling() == ExternalTool::ShowInPane) {
        ICore::instance()->messageManager()->printToOutputPane(output, true);
    }
}

// #pragma mark -- ExternalToolManager

ExternalToolManager::ExternalToolManager(Core::ICore *core)
    : QObject(core), m_core(core)
{
    initialize();
}

ExternalToolManager::~ExternalToolManager()
{
    qDeleteAll(m_tools);
}

void ExternalToolManager::initialize()
{
    ActionManager *am = m_core->actionManager();
    ActionContainer *mexternaltools = am->createMenu(Id(Constants::M_TOOLS_EXTERNAL));
    mexternaltools->menu()->setTitle(tr("External"));
    ActionContainer *mtools = am->actionContainer(Constants::M_TOOLS);
    Command *cmd;

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, Id("Tools.Separator"), Context(Constants::C_GLOBAL));
    mtools->addAction(cmd, Constants::G_DEFAULT_THREE);
    mtools->addMenu(mexternaltools, Constants::G_DEFAULT_THREE);

    QMap<QString, ActionContainer *> categoryMenus;
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

            // category menus
            // TODO sort alphabetically
            ActionContainer *container;
            if (tool->displayCategory().isEmpty())
                container = mexternaltools;
            else
                container = categoryMenus.value(tool->displayCategory());
            if (!container) {
                container = am->createMenu(Id("Tools.External.Category." + tool->displayCategory()));
                container->menu()->setTitle(tool->displayCategory());
                mexternaltools->addMenu(container, Constants::G_DEFAULT_ONE);
            }

            // tool action
            QAction *action = new QAction(tool->displayName(), this);
            action->setToolTip(tool->description());
            action->setWhatsThis(tool->description());
            action->setData(tool->id());
            cmd = am->registerAction(action, Id("Tools.External." + tool->id()), Context(Constants::C_GLOBAL));
            container->addAction(cmd, Constants::G_DEFAULT_TWO);
            connect(action, SIGNAL(triggered()), this, SLOT(menuActivated()));
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
