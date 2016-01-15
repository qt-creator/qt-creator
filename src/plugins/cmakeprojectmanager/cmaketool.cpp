/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <utils/qtcassert.h>

#include <QProcess>
#include <QFileInfo>
#include <QTextDocument>
#include <QUuid>

using namespace CMakeProjectManager;

const char CMAKE_INFORMATION_ID[] = "Id";
const char CMAKE_INFORMATION_COMMAND[] = "Binary";
const char CMAKE_INFORMATION_DISPLAYNAME[] = "DisplayName";
const char CMAKE_INFORMATION_AUTODETECTED[] = "AutoDetected";

///////////////////////////
// CMakeTool
///////////////////////////
CMakeTool::CMakeTool(Detection d, const Core::Id &id) :
    m_isAutoDetected(d == AutoDetection),
    m_id(id)
{
    //make sure every CMakeTool has a valid ID
    if (!m_id.isValid())
        createId();
}

CMakeTool::CMakeTool(const QVariantMap &map, bool fromSdk) :
    m_isAutoDetected(fromSdk)
{
    m_id = Core::Id::fromSetting(map.value(QLatin1String(CMAKE_INFORMATION_ID)));
    m_displayName = map.value(QLatin1String(CMAKE_INFORMATION_DISPLAYNAME)).toString();

    //loading a CMakeTool from SDK is always autodetection
    if (!fromSdk)
        m_isAutoDetected = map.value(QLatin1String(CMAKE_INFORMATION_AUTODETECTED), false).toBool();

    setCMakeExecutable(Utils::FileName::fromUserInput(map.value(QLatin1String(CMAKE_INFORMATION_COMMAND)).toString()));
}

CMakeTool::~CMakeTool()
{
    cancel();
}

void CMakeTool::cancel()
{
    if (m_process) {
        disconnect(m_process, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                   this, &CMakeTool::finished);

        if (m_process->state() != QProcess::NotRunning)
            m_process->kill();

        m_process->waitForFinished();
        delete m_process;
        m_process = 0;
    }
}

void CMakeTool::setCMakeExecutable(const Utils::FileName &executable)
{
    cancel();
    m_process = new QProcess();
    connect(m_process, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this, &CMakeTool::finished);

    m_executable = executable;
    QFileInfo fi = m_executable.toFileInfo();
    if (fi.exists() && fi.isExecutable()) {
        // Run it to find out more
        m_state = CMakeTool::RunningBasic;
        if (!startProcess(QStringList(QLatin1String("--help"))))
            m_state = CMakeTool::Invalid;
    } else {
        m_state = CMakeTool::Invalid;
    }

    CMakeToolManager::notifyAboutUpdate(this);
}

void CMakeTool::finished(int exitCode)
{
    if (exitCode) {
        m_state = CMakeTool::Invalid;
        return;
    }
    if (m_state == CMakeTool::RunningBasic) {
        QByteArray response = m_process->readAll();

        m_hasCodeBlocksMsvcGenerator = response.contains("CodeBlocks - NMake Makefiles");
        m_hasCodeBlocksNinjaGenerator = response.contains("CodeBlocks - Ninja");

        if (response.isEmpty()) {
            m_state = CMakeTool::Invalid;
        } else {
            m_state = CMakeTool::RunningFunctionList;
            if (!startProcess(QStringList(QLatin1String("--help-command-list"))))
                finished(0); // should never happen, just continue
        }
    } else if (m_state == CMakeTool::RunningFunctionList) {
        parseFunctionOutput(m_process->readAll());
        m_state = CMakeTool::RunningFunctionDetails;
        if (!startProcess(QStringList(QLatin1String("--help-commands"))))
            finished(0); // should never happen, just continue
    } else if (m_state == CMakeTool::RunningFunctionDetails) {
        parseFunctionDetailsOutput(m_process->readAll());
        m_state = CMakeTool::RunningPropertyList;
        if (!startProcess(QStringList(QLatin1String("--help-property-list"))))
            finished(0); // should never happen, just continue
    } else if (m_state == CMakeTool::RunningPropertyList) {
        parseVariableOutput(m_process->readAll());
        m_state = CMakeTool::RunningVariableList;
        if (!startProcess(QStringList(QLatin1String("--help-variable-list"))))
            finished(0); // should never happen, just continue
    } else if (m_state == CMakeTool::RunningVariableList) {
        parseVariableOutput(m_process->readAll());
        parseDone();
        m_state = CMakeTool::RunningDone;
    }
}

bool CMakeTool::isValid() const
{
    if (m_state == CMakeTool::Invalid || !m_id.isValid())
        return false;
    if (m_state == CMakeTool::RunningBasic) {
        if (!m_process->waitForFinished(10000)) {
            return false;
        }
    }
    return (m_state != CMakeTool::Invalid);
}

void CMakeTool::createId()
{
    QTC_ASSERT(!m_id.isValid(), return);
    m_id = Core::Id::fromString(QUuid::createUuid().toString());
}

QVariantMap CMakeTool::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(CMAKE_INFORMATION_DISPLAYNAME), m_displayName);
    data.insert(QLatin1String(CMAKE_INFORMATION_ID), m_id.toSetting());
    data.insert(QLatin1String(CMAKE_INFORMATION_COMMAND), m_executable.toString());
    data.insert(QLatin1String(CMAKE_INFORMATION_AUTODETECTED), m_isAutoDetected);
    return data;
}

bool CMakeTool::startProcess(const QStringList &args)
{
    m_process->start(m_executable.toString(), args);
    return m_process->waitForStarted(2000);
}

Utils::FileName CMakeTool::cmakeExecutable() const
{
    return m_executable;
}

bool CMakeTool::hasCodeBlocksMsvcGenerator() const
{
    if (!isValid())
        return false;
    return m_hasCodeBlocksMsvcGenerator;
}

bool CMakeTool::hasCodeBlocksNinjaGenerator() const
{
    if (!isValid())
        return false;
    return m_hasCodeBlocksNinjaGenerator;
}

TextEditor::Keywords CMakeTool::keywords()
{
    while (m_state != RunningDone && m_state != CMakeTool::Invalid) {
        m_process->waitForFinished();
    }

    if (m_state == CMakeTool::Invalid)
        return TextEditor::Keywords(QStringList(), QStringList(), QMap<QString, QStringList>());

    return TextEditor::Keywords(m_variables, m_functions, m_functionArgs);
}

bool CMakeTool::isAutoDetected() const
{
    return m_isAutoDetected;
}

static void extractKeywords(const QByteArray &input, QStringList *destination)
{
    if (!destination)
        return;

    QString keyword;
    int ignoreZone = 0;
    for (int i = 0; i < input.count(); ++i) {
        const QChar chr = QLatin1Char(input.at(i));
        if (chr == QLatin1Char('{'))
            ++ignoreZone;
        if (chr == QLatin1Char('}'))
            --ignoreZone;
        if (ignoreZone == 0) {
            if ((chr.isLetterOrNumber() && chr.isUpper())
                || chr == QLatin1Char('_')) {
                keyword += chr;
            } else {
                if (!keyword.isEmpty()) {
                    if (keyword.size() > 1)
                        *destination << keyword;
                    keyword.clear();
                }
            }
        }
    }
    if (keyword.size() > 1)
        *destination << keyword;
}

void CMakeTool::parseFunctionOutput(const QByteArray &output)
{
    QList<QByteArray> cmakeFunctionsList = output.split('\n');
    m_functions.clear();
    if (!cmakeFunctionsList.isEmpty()) {
        cmakeFunctionsList.removeFirst(); //remove version string
        foreach (const QByteArray &function, cmakeFunctionsList)
            m_functions << QString::fromLocal8Bit(function.trimmed());
    }
}

QString CMakeTool::formatFunctionDetails(const QString &command, const QString &args)
{
    return QString::fromLatin1("<table><tr><td><b>%1</b></td><td>%2</td></tr>")
            .arg(command.toHtmlEscaped(), args.toHtmlEscaped());
}

QString CMakeTool::displayName() const
{
    return m_displayName;
}

void CMakeTool::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
    CMakeToolManager::notifyAboutUpdate(this);
}

void CMakeTool::setPathMapper(const CMakeTool::PathMapper &pathMapper)
{
    m_pathMapper = pathMapper;
}

QString CMakeTool::mapAllPaths(ProjectExplorer::Kit *kit, const QString &in) const
{
    if (m_pathMapper)
        return m_pathMapper(kit, in);
    return in;
}

void CMakeTool::parseFunctionDetailsOutput(const QByteArray &output)
{
    QStringList cmakeFunctionsList = m_functions;
    QList<QByteArray> cmakeCommandsHelp = output.split('\n');
    for (int i = 0; i < cmakeCommandsHelp.count(); ++i) {
        QByteArray lineTrimmed = cmakeCommandsHelp.at(i).trimmed();
        if (cmakeFunctionsList.isEmpty())
            break;
        if (cmakeFunctionsList.first().toLatin1() == lineTrimmed) {
            QStringList commandSyntaxes;
            QString currentCommandSyntax;
            QString currentCommand = cmakeFunctionsList.takeFirst();
            ++i;
            for (; i < cmakeCommandsHelp.count(); ++i) {
                lineTrimmed = cmakeCommandsHelp.at(i).trimmed();

                if (!cmakeFunctionsList.isEmpty() && cmakeFunctionsList.first().toLatin1() == lineTrimmed) {
                    //start of next function in output
                    if (!currentCommandSyntax.isEmpty())
                        commandSyntaxes << currentCommandSyntax.append(QLatin1String("</table>"));
                    --i;
                    break;
                }
                if (lineTrimmed.startsWith(currentCommand.toLatin1() + "(")) {
                    if (!currentCommandSyntax.isEmpty())
                        commandSyntaxes << currentCommandSyntax.append(QLatin1String("</table>"));

                    QByteArray argLine = lineTrimmed.mid(currentCommand.length());
                    extractKeywords(argLine, &m_variables);
                    currentCommandSyntax = formatFunctionDetails(currentCommand, QString::fromUtf8(argLine));
                } else {
                    if (!currentCommandSyntax.isEmpty()) {
                        if (lineTrimmed.isEmpty()) {
                            commandSyntaxes << currentCommandSyntax.append(QLatin1String("</table>"));
                            currentCommandSyntax.clear();
                        } else {
                            extractKeywords(lineTrimmed, &m_variables);
                            currentCommandSyntax += QString::fromLatin1("<tr><td>&nbsp;</td><td>%1</td></tr>")
                                    .arg(QString::fromLocal8Bit(lineTrimmed).toHtmlEscaped());
                        }
                    }
                }
            }
            m_functionArgs[currentCommand] = commandSyntaxes;
        }
    }
    m_functions = m_functionArgs.keys();
}

void CMakeTool::parseVariableOutput(const QByteArray &output)
{
    QList<QByteArray> variableList = output.split('\n');
    if (!variableList.isEmpty()) {
        variableList.removeFirst(); //remove version string
        foreach (const QByteArray &variable, variableList) {
            if (variable.contains("_<CONFIG>")) {
                m_variables << QString::fromLocal8Bit(variable).replace(QLatin1String("_<CONFIG>"), QLatin1String("_DEBUG"));
                m_variables << QString::fromLocal8Bit(variable).replace(QLatin1String("_<CONFIG>"), QLatin1String("_RELEASE"));
                m_variables << QString::fromLocal8Bit(variable).replace(QLatin1String("_<CONFIG>"), QLatin1String("_MINSIZEREL"));
                m_variables << QString::fromLocal8Bit(variable).replace(QLatin1String("_<CONFIG>"), QLatin1String("_RELWITHDEBINFO"));
            } else if (variable.contains("_<LANG>")) {
                m_variables << QString::fromLocal8Bit(variable).replace(QLatin1String("_<LANG>"), QLatin1String("_C"));
                m_variables << QString::fromLocal8Bit(variable).replace(QLatin1String("_<LANG>"), QLatin1String("_CXX"));
            } else if (!variable.contains("_<") && !variable.contains('[')) {
                m_variables << QString::fromLocal8Bit(variable);
            }
        }
    }
}

void CMakeTool::parseDone()
{
    m_variables.sort();
    m_variables.removeDuplicates();
}
