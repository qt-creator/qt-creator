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

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QTextDocument>
#include <QUuid>
#include <QVariantMap>

namespace CMakeProjectManager {

const char CMAKE_INFORMATION_ID[] = "Id";
const char CMAKE_INFORMATION_COMMAND[] = "Binary";
const char CMAKE_INFORMATION_DISPLAYNAME[] = "DisplayName";
const char CMAKE_INFORMATION_AUTODETECTED[] = "AutoDetected";

///////////////////////////
// CMakeTool
///////////////////////////
CMakeTool::CMakeTool(Detection d, const Core::Id &id) :
    m_id(id), m_isAutoDetected(d == AutoDetection)
{
    QTC_ASSERT(m_id.isValid(), m_id = Core::Id::fromString(QUuid::createUuid().toString()));
}

CMakeTool::CMakeTool(const QVariantMap &map, bool fromSdk) : m_isAutoDetected(fromSdk)
{
    m_id = Core::Id::fromSetting(map.value(QLatin1String(CMAKE_INFORMATION_ID)));
    m_displayName = map.value(QLatin1String(CMAKE_INFORMATION_DISPLAYNAME)).toString();

    //loading a CMakeTool from SDK is always autodetection
    if (!fromSdk)
        m_isAutoDetected = map.value(QLatin1String(CMAKE_INFORMATION_AUTODETECTED), false).toBool();

    setCMakeExecutable(Utils::FileName::fromString(map.value(QLatin1String(CMAKE_INFORMATION_COMMAND)).toString()));
}

Core::Id CMakeTool::createId()
{
    return Core::Id::fromString(QUuid::createUuid().toString());
}

void CMakeTool::setCMakeExecutable(const Utils::FileName &executable)
{
    if (m_executable == executable)
        return;

    m_didRun = false;
    m_didAttemptToRun = false;

    m_executable = executable;
    CMakeToolManager::notifyAboutUpdate(this);
}

bool CMakeTool::isValid() const
{
    if (!m_id.isValid())
        return false;

    if (!m_didAttemptToRun)
        supportedGenerators();

    return m_didRun;
}

Utils::SynchronousProcessResponse CMakeTool::run(const QString &arg) const
{
    if (m_didAttemptToRun && !m_didRun) {
        Utils::SynchronousProcessResponse response;
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    Utils::SynchronousProcess cmake;
    cmake.setTimeoutS(1);
    cmake.setFlags(Utils::SynchronousProcess::UnixTerminalDisabled);
    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    cmake.setProcessEnvironment(env.toProcessEnvironment());
    cmake.setTimeOutMessageBoxEnabled(false);

    Utils::SynchronousProcessResponse response = cmake.run(m_executable.toString(), QStringList() << arg);
    m_didAttemptToRun = true;
    m_didRun = (response.result == Utils::SynchronousProcessResponse::Finished);
    return response;
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

Utils::FileName CMakeTool::cmakeExecutable() const
{
    return m_executable;
}

QStringList CMakeTool::supportedGenerators() const
{
    if (m_generators.isEmpty()) {
        Utils::SynchronousProcessResponse response = run(QLatin1String("--help"));
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            bool inGeneratorSection = false;
            const QStringList lines = response.stdOut.split(QLatin1Char('\n'));
            foreach (const QString &line, lines) {
                if (line.isEmpty())
                    continue;
                if (line == QLatin1String("Generators")) {
                    inGeneratorSection = true;
                    continue;
                }
                if (!inGeneratorSection)
                    continue;

                if (line.startsWith(QLatin1String("  ")) && line.at(3) != QLatin1Char(' ')) {
                    int pos = line.indexOf(QLatin1Char('='));
                    if (pos < 0)
                        pos = line.length();
                    if (pos >= 0) {
                        --pos;
                        while (pos > 2 && line.at(pos).isSpace())
                            --pos;
                    }
                    if (pos > 2)
                        m_generators.append(line.mid(2, pos - 1));
                }
            }
        }
    }
    return m_generators;
}

TextEditor::Keywords CMakeTool::keywords()
{
    if (m_functions.isEmpty()) {
        Utils::SynchronousProcessResponse response;
        response = run(QLatin1String("--help-command-list"));
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            m_functions = response.stdOut.split(QLatin1Char('\n'));

        response = run(QLatin1String("--help-commands"));
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            parseFunctionDetailsOutput(response.stdOut);

        response = run(QLatin1String("--help-property-list"));
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            m_variables = parseVariableOutput(response.stdOut);

        response = run(QLatin1String("--help-variable-list"));
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            m_variables.append(parseVariableOutput(response.stdOut));
            m_variables = Utils::filteredUnique(m_variables);
            Utils::sort(m_variables);
        }
    }

    return TextEditor::Keywords(m_variables, m_functions, m_functionArgs);
}

bool CMakeTool::isAutoDetected() const
{
    return m_isAutoDetected;
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

QString CMakeTool::mapAllPaths(const ProjectExplorer::Kit *kit, const QString &in) const
{
    if (m_pathMapper)
        return m_pathMapper(kit, in);
    return in;
}

static QStringList parseDefinition(const QString &definition)
{
    QStringList result;
    QString word;
    bool ignoreWord = false;
    QVector<QChar> braceStack;

    foreach (const QChar &c, definition) {
        if (c == QLatin1Char('[') || c == QLatin1Char('<') || c == QLatin1Char('(')) {
            braceStack.append(c);
            ignoreWord = false;
        } else if (c == QLatin1Char(']') || c == QLatin1Char('>') || c == QLatin1Char(')')) {
            if (braceStack.isEmpty() || braceStack.takeLast() == QLatin1Char('<'))
                ignoreWord = true;
        }

        if (c == QLatin1Char(' ') || c == QLatin1Char('[') || c == QLatin1Char('<') || c == QLatin1Char('(')
                || c == QLatin1Char(']') || c == QLatin1Char('>') || c == QLatin1Char(')')) {
            if (!ignoreWord && !word.isEmpty()) {
                if (result.isEmpty() || Utils::allOf(word, [](const QChar &c) { return c.isUpper() || c == QLatin1Char('_'); }))
                    result.append(word);
            }
            word.clear();
            ignoreWord = false;
        } else {
            word.append(c);
        }
    }
    return result;
}

void CMakeTool::parseFunctionDetailsOutput(const QString &output)
{
    QSet<QString> functionSet;
    functionSet.fromList(m_functions);

    bool expectDefinition = false;
    QString currentDefinition;

    const QStringList lines = output.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.count(); ++i) {
        const QString line = lines.at(i);

        if (line == QLatin1String("::")) {
            expectDefinition = true;
            continue;
        }

        if (expectDefinition) {
            if (!line.startsWith(QLatin1Char(' ')) && !line.isEmpty()) {
                expectDefinition = false;
                QStringList words = parseDefinition(currentDefinition);
                if (!words.isEmpty()) {
                    const QString command = words.takeFirst();
                    if (functionSet.contains(command)) {
                        QStringList tmp = words + m_functionArgs[command];
                        Utils::sort(tmp);
                        m_functionArgs[command] = Utils::filteredUnique(tmp);
                    }
                }
                if (!words.isEmpty() && functionSet.contains(words.at(0)))
                    m_functionArgs[words.at(0)];
                currentDefinition.clear();
            } else {
                currentDefinition.append(line.trimmed() + QLatin1Char(' '));
            }
        }
    }
}

QStringList CMakeTool::parseVariableOutput(const QString &output)
{
    const QStringList variableList = output.split(QLatin1Char('\n'));
    QStringList result;
    foreach (const QString &v, variableList) {
        if (v.contains(QLatin1String("<CONFIG>"))) {
            const QString tmp = QString(v).replace(QLatin1String("<CONFIG>"), QLatin1String("%1"));
            result << tmp.arg(QLatin1String("DEBUG")) << tmp.arg(QLatin1String("RELEASE"))
                   << tmp.arg(QLatin1String("MINSIZEREL")) << tmp.arg(QLatin1String("RELWITHDEBINFO"));
        } else if (v.contains(QLatin1String("<LANG>"))) {
            const QString tmp = QString(v).replace(QLatin1String("<LANG>"), QLatin1String("%1"));
            result << tmp.arg(QLatin1String("C")) << tmp.arg(QLatin1String("CXX"));
        } else if (!v.contains(QLatin1Char('<')) && !v.contains(QLatin1Char('['))) {
            result << v;
        }
    }
    return result;
}

} // namespace CMakeProjectManager
