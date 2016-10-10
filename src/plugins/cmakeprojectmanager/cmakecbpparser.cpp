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

#include "cmakecbpparser.h"
#include "cmakekitinformation.h"
#include "cmaketool.h"

#include <utils/fileutils.h>
#include <utils/stringutils.h>
#include <utils/algorithm.h>
#include <projectexplorer/projectnodes.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

/////
// CMakeCbpParser
////

namespace {
int distance(const QString &targetDirectory, const FileName &fileName)
{
    const QString commonParent = Utils::commonPath(QStringList() << targetDirectory << fileName.toString());
    return targetDirectory.mid(commonParent.size()).count(QLatin1Char('/'))
            + fileName.toString().mid(commonParent.size()).count(QLatin1Char('/'));
}
}

// called after everything is parsed
// this function tries to figure out to which CMakeBuildTarget
// each file belongs, so that it gets the appropriate defines and
// compiler flags
void CMakeCbpParser::sortFiles()
{
    QLoggingCategory log("qtc.cmakeprojectmanager.filetargetmapping");
    FileNameList fileNames = Utils::transform(m_fileList, &FileNode::filePath);

    Utils::sort(fileNames);


    CMakeBuildTarget *last = 0;
    FileName parentDirectory;

    qCDebug(log) << "###############";
    qCDebug(log) << "# Pre Dump    #";
    qCDebug(log) << "###############";
    foreach (const CMakeBuildTarget &target, m_buildTargets)
        qCDebug(log) << target.title << target.sourceDirectory <<
                 target.includeFiles << target.defines << target.files << "\n";

    // find a good build target to fall back
    int fallbackIndex = 0;
    {
        int bestIncludeCount = -1;
        for (int i = 0; i < m_buildTargets.size(); ++i) {
            const CMakeBuildTarget &target = m_buildTargets.at(i);
            if (target.includeFiles.isEmpty())
                continue;
            if (target.sourceDirectory == m_sourceDirectory
                    && target.includeFiles.count() > bestIncludeCount) {
                bestIncludeCount = target.includeFiles.count();
                fallbackIndex = i;
            }
        }
    }

    qCDebug(log) << "###############";
    qCDebug(log) << "# Sorting     #";
    qCDebug(log) << "###############";

    foreach (const FileName &fileName, fileNames) {
        qCDebug(log) << fileName;
        const QStringList unitTargets = m_unitTargetMap[fileName];
        if (!unitTargets.isEmpty()) {
            // cmake >= 3.3:
            foreach (const QString &unitTarget, unitTargets) {
                int index = Utils::indexOf(m_buildTargets, Utils::equal(&CMakeBuildTarget::title, unitTarget));
                if (index != -1) {
                    m_buildTargets[index].files.append(fileName.toString());
                    qCDebug(log) << "  into" << m_buildTargets[index].title << "(target attribute)";
                    continue;
                }
            }
            continue;
        }

        // fallback for cmake < 3.3:
        if (fileName.parentDir() == parentDirectory && last) {
            // easy case, same parent directory as last file
            last->files.append(fileName.toString());
            qCDebug(log) << "  into" << last->title << "(same parent)";
        } else {
            int bestDistance = std::numeric_limits<int>::max();
            int bestIndex = -1;
            int bestIncludeCount = -1;

            for (int i = 0; i < m_buildTargets.size(); ++i) {
                const CMakeBuildTarget &target = m_buildTargets.at(i);
                if (target.includeFiles.isEmpty())
                    continue;
                int dist = distance(target.sourceDirectory, fileName);
                qCDebug(log) << "distance to target" << target.title << dist;
                if (dist < bestDistance ||
                     (dist == bestDistance &&
                      target.includeFiles.count() > bestIncludeCount)) {
                    bestDistance = dist;
                    bestIncludeCount = target.includeFiles.count();
                    bestIndex = i;
                }
            }

            if (bestIndex == -1 && !m_buildTargets.isEmpty()) {
                bestIndex = fallbackIndex;
                qCDebug(log) << "  using fallbackIndex";
            }

            if (bestIndex != -1) {
                m_buildTargets[bestIndex].files.append(fileName.toString());
                last = &m_buildTargets[bestIndex];
                parentDirectory = fileName.parentDir();
                qCDebug(log) << "  into" << last->title;
            }
        }
    }

    qCDebug(log) << "###############";
    qCDebug(log) << "# After Dump  #";
    qCDebug(log) << "###############";
    foreach (const CMakeBuildTarget &target, m_buildTargets)
        qCDebug(log) << target.title << target.sourceDirectory << target.includeFiles << target.defines << target.files << "\n";
}

bool CMakeCbpParser::parseCbpFile(const Kit *const kit, const QString &fileName, const QString &sourceDirectory)
{
    m_kit = kit;
    m_buildDirectory = QFileInfo(fileName).absolutePath();
    m_sourceDirectory = sourceDirectory;

    QFile fi(fileName);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        setDevice(&fi);

        while (!atEnd()) {
            readNext();
            if (name() == QLatin1String("CodeBlocks_project_file"))
                parseCodeBlocks_project_file();
            else if (isStartElement())
                parseUnknownElement();
        }

        sortFiles();

        fi.close();

        // There is always a clean target:
        CMakeBuildTarget cleanTarget;
        cleanTarget.title = QLatin1String("clean");
        cleanTarget.targetType = UtilityType;
        cleanTarget.workingDirectory = m_buildDirectory;
        cleanTarget.sourceDirectory = m_sourceDirectory;

        m_buildTargets.append(cleanTarget);

        return true;
    }
    return false;
}

void CMakeCbpParser::parseCodeBlocks_project_file()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Project"))
            parseProject();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseProject()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Option"))
            parseOption();
        else if (name() == QLatin1String("Unit"))
            parseUnit();
        else if (name() == QLatin1String("Build"))
            parseBuild();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuild()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Target"))
            parseBuildTarget();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuildTarget()
{
    m_buildTarget.clear();

    if (attributes().hasAttribute(QLatin1String("title")))
        m_buildTarget.title = attributes().value(QLatin1String("title")).toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (!m_buildTarget.title.endsWith(QLatin1String("/fast"))
                    && !m_buildTarget.title.endsWith(QLatin1String("_automoc"))) {
                if (m_buildTarget.executable.isEmpty() && m_buildTarget.targetType == ExecutableType)
                    m_buildTarget.targetType = UtilityType;
                m_buildTargets.append(m_buildTarget);
            }
            return;
        } else if (name() == QLatin1String("Compiler")) {
            parseCompiler();
        } else if (name() == QLatin1String("Option")) {
            parseBuildTargetOption();
        } else if (name() == QLatin1String("MakeCommands")) {
            parseMakeCommands();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuildTargetOption()
{
    if (attributes().hasAttribute(QLatin1String("output"))) {
        m_buildTarget.executable = attributes().value(QLatin1String("output")).toString();
        CMakeTool *tool = CMakeKitInformation::cmakeTool(m_kit);
        if (tool)
            m_buildTarget.executable = tool->mapAllPaths(m_kit, m_buildTarget.executable);
    } else if (attributes().hasAttribute(QLatin1String("type"))) {
        const QStringRef value = attributes().value(QLatin1String("type"));
        if (value == "0" || value == "1")
            m_buildTarget.targetType = ExecutableType;
        else if (value == "2")
            m_buildTarget.targetType = StaticLibraryType;
        else if (value == "3")
            m_buildTarget.targetType = DynamicLibraryType;
        else
            m_buildTarget.targetType = UtilityType;
    } else if (attributes().hasAttribute(QLatin1String("working_dir"))) {
        m_buildTarget.workingDirectory = attributes().value(QLatin1String("working_dir")).toString();

        QFile cmakeSourceInfoFile(m_buildTarget.workingDirectory
                                  + QStringLiteral("/CMakeFiles/CMakeDirectoryInformation.cmake"));
        if (cmakeSourceInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&cmakeSourceInfoFile);
            const QLatin1String searchSource("SET(CMAKE_RELATIVE_PATH_TOP_SOURCE \"");
            while (!stream.atEnd()) {
                const QString lineTopSource = stream.readLine().trimmed();
                if (lineTopSource.startsWith(searchSource, Qt::CaseInsensitive)) {
                    m_buildTarget.sourceDirectory = lineTopSource.mid(searchSource.size());
                    m_buildTarget.sourceDirectory.chop(2); // cut off ")
                    break;
                }
            }
        }

        if (m_buildTarget.sourceDirectory.isEmpty()) {
            QDir dir(m_buildDirectory);
            const QString relative = dir.relativeFilePath(m_buildTarget.workingDirectory);
            m_buildTarget.sourceDirectory
                    = FileName::fromString(m_sourceDirectory).appendPath(relative).toString();
        }
    }
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

QString CMakeCbpParser::projectName() const
{
    return m_projectName;
}

void CMakeCbpParser::parseOption()
{
    if (attributes().hasAttribute(QLatin1String("title")))
        m_projectName = attributes().value(QLatin1String("title")).toString();

    if (attributes().hasAttribute(QLatin1String("compiler")))
        m_compiler = attributes().value(QLatin1String("compiler")).toString();

    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseMakeCommands()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Build"))
            parseBuildTargetBuild();
        else if (name() == QLatin1String("Clean"))
            parseBuildTargetClean();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuildTargetBuild()
{
    if (attributes().hasAttribute(QLatin1String("command"))) {
        m_buildTarget.makeCommand = attributes().value(QLatin1String("command")).toString();

        CMakeTool *tool = CMakeKitInformation::cmakeTool(m_kit);
        if (tool)
            m_buildTarget.makeCommand = tool->mapAllPaths(m_kit, m_buildTarget.makeCommand);
    }
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseBuildTargetClean()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseCompiler()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == QLatin1String("Add"))
            parseAdd();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseAdd()
{
    // CMake only supports <Add option=\> and <Add directory=\>
    const QXmlStreamAttributes addAttributes = attributes();

    QString includeDirectory = addAttributes.value(QLatin1String("directory")).toString();

    CMakeTool *tool = CMakeKitInformation::cmakeTool(m_kit);
    if (tool)
        includeDirectory = tool->mapAllPaths(m_kit, includeDirectory);

    // allow adding multiple times because order happens
    if (!includeDirectory.isEmpty())
        m_buildTarget.includeFiles.append(includeDirectory);

    QString compilerOption = addAttributes.value(QLatin1String("option")).toString();
    // defining multiple times a macro to the same value makes no sense
    if (!compilerOption.isEmpty() && !m_buildTarget.compilerOptions.contains(compilerOption)) {
        m_buildTarget.compilerOptions.append(compilerOption);
        int macroNameIndex = compilerOption.indexOf(QLatin1String("-D")) + 2;
        if (macroNameIndex != 1) {
            int assignIndex = compilerOption.indexOf(QLatin1Char('='), macroNameIndex);
            if (assignIndex != -1)
                compilerOption[assignIndex] = ' ';
            m_buildTarget.defines.append("#define ");
            m_buildTarget.defines.append(compilerOption.mid(macroNameIndex).toUtf8());
            m_buildTarget.defines.append('\n');
        }
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseUnit()
{
    //qDebug()<<stream.attributes().value("filename");
    FileName fileName =
            FileName::fromUserInput(attributes().value(QLatin1String("filename")).toString());

    CMakeTool *tool = CMakeKitInformation::cmakeTool(m_kit);
    if (tool) {
        QString mappedFile = tool->mapAllPaths(m_kit, fileName.toString());
        fileName = FileName::fromUserInput(mappedFile);
    }

    m_parsingCMakeUnit = false;
    m_unitTargets.clear();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (!fileName.endsWith(QLatin1String(".rule")) && !m_processedUnits.contains(fileName)) {
                // Now check whether we found a virtual element beneath
                if (m_parsingCMakeUnit) {
                    m_cmakeFileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::ProjectFileType, false));
                } else {
                    bool generated = false;
                    QString onlyFileName = fileName.fileName();
                    if (   (onlyFileName.startsWith(QLatin1String("moc_")) && onlyFileName.endsWith(QLatin1String(".cxx")))
                        || (onlyFileName.startsWith(QLatin1String("ui_")) && onlyFileName.endsWith(QLatin1String(".h")))
                        || (onlyFileName.startsWith(QLatin1String("qrc_")) && onlyFileName.endsWith(QLatin1String(".cxx"))))
                        generated = true;

                    if (fileName.endsWith(QLatin1String(".qrc")))
                        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::ResourceType, generated));
                    else
                        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::SourceType, generated));
                }
                m_unitTargetMap.insert(fileName, m_unitTargets);
                m_processedUnits.insert(fileName);
            }
            return;
        } else if (name() == QLatin1String("Option")) {
            parseUnitOption();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseUnitOption()
{
    const QXmlStreamAttributes optionAttributes = attributes();
    m_parsingCMakeUnit = optionAttributes.hasAttribute(QLatin1String("virtualFolder"));
    const QString target = optionAttributes.value(QLatin1String("target")).toString();
    if (!target.isEmpty())
        m_unitTargets.append(target);

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            parseUnknownElement();
    }
}

void CMakeCbpParser::parseUnknownElement()
{
    Q_ASSERT(isStartElement());

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            parseUnknownElement();
    }
}

QList<ProjectExplorer::FileNode *> CMakeCbpParser::fileList()
{
    return m_fileList;
}

QList<ProjectExplorer::FileNode *> CMakeCbpParser::cmakeFileList()
{
    return m_cmakeFileList;
}

bool CMakeCbpParser::hasCMakeFiles()
{
    return !m_cmakeFileList.isEmpty();
}

QList<CMakeBuildTarget> CMakeCbpParser::buildTargets()
{
    return m_buildTargets;
}

QString CMakeCbpParser::compilerName() const
{
    return m_compiler;
}


} // namespace Internal
} // namespace CMakeProjectManager
