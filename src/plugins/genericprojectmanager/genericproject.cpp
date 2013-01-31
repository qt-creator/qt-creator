/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "genericproject.h"

#include "genericbuildconfiguration.h"
#include "genericmakestep.h"
#include "genericprojectconstants.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/ModelManagerInterface.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QProcessEnvironment>

using namespace Core;
using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

////////////////////////////////////////////////////////////////////////////////////
//
// GenericProject
//
////////////////////////////////////////////////////////////////////////////////////

GenericProject::GenericProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName)
{
    setProjectContext(Context(GenericProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguage(Context(ProjectExplorer::Constants::LANG_CXX));

    QFileInfo fileInfo(m_fileName);
    QDir dir = fileInfo.dir();

    m_projectName      = fileInfo.completeBaseName();
    m_filesFileName    = QFileInfo(dir, m_projectName + QLatin1String(".files")).absoluteFilePath();
    m_includesFileName = QFileInfo(dir, m_projectName + QLatin1String(".includes")).absoluteFilePath();
    m_configFileName   = QFileInfo(dir, m_projectName + QLatin1String(".config")).absoluteFilePath();

    m_creatorIDocument  = new GenericProjectFile(this, m_fileName, GenericProject::Everything);
    m_filesIDocument    = new GenericProjectFile(this, m_filesFileName, GenericProject::Files);
    m_includesIDocument = new GenericProjectFile(this, m_includesFileName, GenericProject::Configuration);
    m_configIDocument   = new GenericProjectFile(this, m_configFileName, GenericProject::Configuration);

    DocumentManager::addDocument(m_creatorIDocument);
    DocumentManager::addDocument(m_filesIDocument);
    DocumentManager::addDocument(m_includesIDocument);
    DocumentManager::addDocument(m_configIDocument);

    m_rootNode = new GenericProjectNode(this, m_creatorIDocument);

    m_manager->registerProject(this);
}

GenericProject::~GenericProject()
{
    m_codeModelFuture.cancel();
    m_manager->unregisterProject(this);

    delete m_rootNode;
}

QString GenericProject::filesFileName() const
{
    return m_filesFileName;
}

QString GenericProject::includesFileName() const
{
    return m_includesFileName;
}

QString GenericProject::configFileName() const
{
    return m_configFileName;
}

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        forever {
            QString line = stream.readLine();
            if (line.isNull())
                break;

            lines.append(line);
        }
    }

    return lines;
}

bool GenericProject::saveRawFileList(const QStringList &rawFileList)
{
    // Make sure we can open the file for writing
    Utils::FileSaver saver(filesFileName(), QIODevice::Text);
    if (!saver.hasError()) {
        QTextStream stream(saver.file());
        foreach (const QString &filePath, rawFileList)
            stream << filePath << QLatin1Char('\n');
        saver.setResult(&stream);
    }
    if (!saver.finalize(ICore::mainWindow()))
        return false;
    refresh(GenericProject::Files);
    return true;
}

bool GenericProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

bool GenericProject::removeFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    foreach (const QString &filePath, filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList);
}

bool GenericProject::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

bool GenericProject::renameFile(const QString &filePath, const QString &newFilePath)
{
    QStringList newList = m_rawFileList;

    QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
    if (i != m_rawListEntries.end()) {
        int index = newList.indexOf(i.value());
        if (index != -1) {
            QDir baseDir(QFileInfo(m_fileName).dir());
            newList.replace(index, baseDir.relativeFilePath(newFilePath));
        }
    }

    return saveRawFileList(newList);
}

void GenericProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        m_rawListEntries.clear();
        m_rawFileList = readLines(filesFileName());
        m_files = processEntries(m_rawFileList, &m_rawListEntries);
    }

    if (options & Configuration) {
        m_projectIncludePaths = processEntries(readLines(includesFileName()));

        // TODO: Possibly load some configuration from the project file
        //QSettings projectInfo(m_fileName, QSettings::IniFormat);

        m_defines.clear();

        QFile configFile(configFileName());
        if (configFile.open(QFile::ReadOnly))
            m_defines = configFile.readAll();
    }

    if (options & Files)
        emit fileListChanged();
}

void GenericProject::refresh(RefreshOptions options)
{
    QSet<QString> oldFileList;
    if (!(options & Configuration))
        oldFileList = m_files.toSet();

    parseProject(options);

    if (options & Files)
        m_rootNode->refresh(oldFileList);

    CPlusPlus::CppModelManagerInterface *modelManager =
        CPlusPlus::CppModelManagerInterface::instance();

    if (modelManager) {
        CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);
        pinfo.clearProjectParts();
        CPlusPlus::CppModelManagerInterface::ProjectPart::Ptr part(
                    new CPlusPlus::CppModelManagerInterface::ProjectPart);

        Kit *k = activeTarget() ? activeTarget()->kit() : KitManager::instance()->defaultKit();
        if (ToolChain *tc = ToolChainKitInformation::toolChain(k)) {
            QStringList cxxflags; // FIXME: Can we do better?
            part->defines = tc->predefinedMacros(cxxflags);
            part->defines += '\n';

            foreach (const HeaderPath &headerPath, tc->systemHeaderPaths(cxxflags, SysRootKitInformation::sysRoot(k))) {
                if (headerPath.kind() == HeaderPath::FrameworkHeaderPath)
                    part->frameworkPaths.append(headerPath.path());
                else
                    part->includePaths.append(headerPath.path());
            }
        }

        part->includePaths += allIncludePaths();
        part->defines += m_defines;

        // ### add _defines.

        // Add any C/C++ files to be parsed
        QStringList cppMimeTypes;
        cppMimeTypes << QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)
                     << QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)
                     << QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)
                     << QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
                     << QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE);

        const Core::MimeDatabase *mimeDatabase = Core::ICore::mimeDatabase();
        foreach (const QString &file, files()) {
            const Core::MimeType mimeType = mimeDatabase->findByFile(QFileInfo(file));
            if (cppMimeTypes.contains(mimeType.type()))
                part->sourceFiles += file;
        }

        QStringList filesToUpdate;

        if (options & Configuration) {
            filesToUpdate = part->sourceFiles;
            filesToUpdate.append(CPlusPlus::CppModelManagerInterface::configurationFileName());
            // Full update, if there's a code model update, cancel it
            m_codeModelFuture.cancel();
        } else if (options & Files) {
            // Only update files that got added to the list
            QSet<QString> newFileList = part->sourceFiles.toSet();
            newFileList.subtract(oldFileList);
            filesToUpdate.append(newFileList.toList());
        }

        pinfo.appendProjectPart(part);

        modelManager->updateProjectInfo(pinfo);
        m_codeModelFuture = modelManager->updateSourceFiles(filesToUpdate);
    }
}

/**
 * Expands environment variables in the given \a string when they are written
 * like $$(VARIABLE).
 */
static void expandEnvironmentVariables(const QProcessEnvironment &env, QString &string)
{
    static QRegExp candidate(QLatin1String("\\$\\$\\((.+)\\)"));

    int index = candidate.indexIn(string);
    while (index != -1) {
        const QString value = env.value(candidate.cap(1));

        string.replace(index, candidate.matchedLength(), value);
        index += value.length();

        index = candidate.indexIn(string, index);
    }
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList GenericProject::processEntries(const QStringList &paths,
                                           QHash<QString, QString> *map) const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QDir projectDir(QFileInfo(m_fileName).dir());

    QStringList absolutePaths;
    foreach (const QString &path, paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        expandEnvironmentVariables(env, trimmedPath);

        trimmedPath = Utils::FileName::fromUserInput(trimmedPath).toString();

        const QString absPath = QFileInfo(projectDir, trimmedPath).absoluteFilePath();
        absolutePaths.append(absPath);
        if (map)
            map->insert(absPath, trimmedPath);
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList GenericProject::allIncludePaths() const
{
    QStringList paths;
    paths += m_includePaths;
    paths += m_projectIncludePaths;
    paths.removeDuplicates();
    return paths;
}

QStringList GenericProject::projectIncludePaths() const
{
    return m_projectIncludePaths;
}

QStringList GenericProject::files() const
{
    return m_files;
}

QStringList GenericProject::includePaths() const
{
    return m_includePaths;
}

void GenericProject::setIncludePaths(const QStringList &includePaths)
{
    m_includePaths = includePaths;
}

QByteArray GenericProject::defines() const
{
    return m_defines;
}

QString GenericProject::displayName() const
{
    return m_projectName;
}

Id GenericProject::id() const
{
    return Id(Constants::GENERICPROJECT_ID);
}

IDocument *GenericProject::document() const
{
    return m_creatorIDocument;
}

IProjectManager *GenericProject::projectManager() const
{
    return m_manager;
}

GenericProjectNode *GenericProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList GenericProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_files;
}

QStringList GenericProject::buildTargets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

bool GenericProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    Kit *defaultKit = KitManager::instance()->defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

    // Sanity check: We need both a buildconfiguration and a runconfiguration!
    QList<Target *> targetList = targets();
    foreach (Target *t, targetList) {
        if (!t->activeBuildConfiguration()) {
            removeTarget(t);
            delete t;
            continue;
        }
        if (!t->activeRunConfiguration())
            t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    }

    setIncludePaths(allIncludePaths());

    refresh(Everything);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////
//
// GenericProjectFile
//
////////////////////////////////////////////////////////////////////////////////////

GenericProjectFile::GenericProjectFile(GenericProject *parent, QString fileName, GenericProject::RefreshOptions options)
    : IDocument(parent),
      m_project(parent),
      m_fileName(fileName),
      m_options(options)
{ }

bool GenericProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

QString GenericProjectFile::fileName() const
{
    return m_fileName;
}

QString GenericProjectFile::defaultPath() const
{
    return QString();
}

QString GenericProjectFile::suggestedFileName() const
{
    return QString();
}

QString GenericProjectFile::mimeType() const
{
    return QLatin1String(Constants::GENERICMIMETYPE);
}

bool GenericProjectFile::isModified() const
{
    return false;
}

bool GenericProjectFile::isSaveAsAllowed() const
{
    return false;
}

void GenericProjectFile::rename(const QString &newName)
{
    // Can't happen
    Q_UNUSED(newName);
    QTC_CHECK(false);
}

IDocument::ReloadBehavior GenericProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool GenericProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;
    m_project->refresh(m_options);
    return true;
}

} // namespace Internal
} // namespace GenericProjectManager
