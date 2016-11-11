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

#include "builddirmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "cmaketool.h"

#include <coreplugin/icore.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/projectpartbuilder.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BuildDirManager:
// --------------------------------------------------------------------

BuildDirManager::BuildDirManager(CMakeBuildConfiguration *bc) :
    m_buildConfiguration(bc)
{
    QTC_ASSERT(bc, return);

    m_reparseTimer.setSingleShot(true);

    connect(&m_reparseTimer, &QTimer::timeout, this, &BuildDirManager::parse);

    connect(&m_futureWatcher, &QFutureWatcher<QList<FileNode *>>::finished,
            this, &BuildDirManager::emitDataAvailable);
}

BuildDirManager::~BuildDirManager() = default;

const Utils::FileName BuildDirManager::workDirectory() const
{
    const Utils::FileName bdir = m_buildConfiguration->buildDirectory();
    if (bdir.exists())
        return bdir;
    if (!m_tempDir) {
        const QString path = QDir::tempPath() + QLatin1String("/qtc-cmake-XXXXXX");
        m_tempDir.reset(new QTemporaryDir(path));
        if (!m_tempDir->isValid())
            emit errorOccured(tr("Failed to create temporary directory using template \"%1\".").arg(path));
    }
    return Utils::FileName::fromString(m_tempDir->path());
}

void BuildDirManager::emitDataAvailable()
{
    if (!isParsing() && m_futureWatcher.isFinished())
        emit dataAvailable();
}

void BuildDirManager::updateReaderType(std::function<void()> todo)
{
    BuildDirReader::Parameters p(m_buildConfiguration);
    p.buildDirectory = workDirectory();

    if (!m_reader || !m_reader->isCompatible(p)) {
        m_reader.reset(BuildDirReader::createReader(p));
        connect(m_reader.get(), &BuildDirReader::configurationStarted,
                this, &BuildDirManager::configurationStarted);
        connect(m_reader.get(), &BuildDirReader::dataAvailable,
                this, &BuildDirManager::emitDataAvailable);
        connect(m_reader.get(), &BuildDirReader::errorOccured,
                this, &BuildDirManager::errorOccured);
        connect(m_reader.get(), &BuildDirReader::dirty, this, &BuildDirManager::becameDirty);
    }
    m_reader->setParameters(p);

    if (m_reader->isReady())
        todo();
    else
        connect(m_reader.get(), &BuildDirReader::isReadyNow, this, todo);
}

void BuildDirManager::updateReaderData()
{
    BuildDirReader::Parameters p(m_buildConfiguration);
    p.buildDirectory = workDirectory();

    m_reader->setParameters(p);
}

void BuildDirManager::parseOnceReaderReady(bool force)
{
    auto fi = new QFutureInterface<QList<ProjectExplorer::FileNode *>>();
    m_scanFuture = fi->future();
    m_futureWatcher.setFuture(m_scanFuture);

    Core::ProgressManager::addTask(fi->future(), "Scan CMake project tree", "CMake.Scan.Tree");
    Utils::runAsync([this, fi]() { BuildDirManager::asyncScanForFiles(fi); });

    checkConfiguration();
    m_reader->stop();
    m_reader->parse(force);
}

void BuildDirManager::maybeForceReparseOnceReaderReady()
{
    checkConfiguration();

    const QByteArray GENERATOR_KEY = "CMAKE_GENERATOR";
    const QByteArray EXTRA_GENERATOR_KEY = "CMAKE_EXTRA_GENERATOR";
    const QByteArray CMAKE_COMMAND_KEY = "CMAKE_COMMAND";

    const QByteArrayList criticalKeys
            = QByteArrayList() << GENERATOR_KEY << CMAKE_COMMAND_KEY;

    const CMakeConfig currentConfig = parsedConfiguration();

    Kit *k = m_buildConfiguration->target()->kit();
    const CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
    QTC_ASSERT(tool, return); // No cmake... we should not have ended up here in the first place
    const QString extraKitGenerator = CMakeGeneratorKitInformation::extraGenerator(k);
    const QString mainKitGenerator = CMakeGeneratorKitInformation::generator(k);
    CMakeConfig targetConfig = m_buildConfiguration->cmakeConfiguration();
    targetConfig.append(CMakeConfigItem(GENERATOR_KEY, CMakeConfigItem::INTERNAL,
                                        QByteArray(), mainKitGenerator.toUtf8()));
    if (!extraKitGenerator.isEmpty())
        targetConfig.append(CMakeConfigItem(EXTRA_GENERATOR_KEY, CMakeConfigItem::INTERNAL,
                                            QByteArray(), extraKitGenerator.toUtf8()));
    targetConfig.append(CMakeConfigItem(CMAKE_COMMAND_KEY, CMakeConfigItem::INTERNAL,
                                        QByteArray(), tool->cmakeExecutable().toUserOutput().toUtf8()));
    Utils::sort(targetConfig, CMakeConfigItem::sortOperator());

    bool mustReparse = false;
    auto ccit = currentConfig.constBegin();
    auto kcit = targetConfig.constBegin();

    while (ccit != currentConfig.constEnd() && kcit != targetConfig.constEnd()) {
        if (ccit->key == kcit->key) {
            if (ccit->value != kcit->value) {
                if (criticalKeys.contains(kcit->key)) {
                        clearCache();
                        return;
                }
                mustReparse = true;
            }
            ++ccit;
            ++kcit;
        } else {
            if (ccit->key < kcit->key) {
                ++ccit;
            } else {
                ++kcit;
                mustReparse = true;
            }
        }
    }

    // If we have keys that do not exist yet, then reparse.
    //
    // The critical keys *must* be set in cmake configuration, so those were already
    // handled above.
    if (mustReparse || kcit != targetConfig.constEnd())
        parseOnceReaderReady(true);
}

bool BuildDirManager::isParsing() const
{
    return m_reader && m_reader->isParsing();
}

void BuildDirManager::becameDirty()
{
    if (isParsing())
        return;

    Target *t = m_buildConfiguration->target()->project()->activeTarget();
    BuildConfiguration *bc = t ? t->activeBuildConfiguration() : nullptr;

    if (bc != m_buildConfiguration)
        return;

    const CMakeTool *tool = CMakeKitInformation::cmakeTool(m_buildConfiguration->target()->kit());
    if (!tool->isAutoRun())
        return;

    m_reparseTimer.start(1000);
}

void BuildDirManager::asyncScanForFiles(QFutureInterface<QList<FileNode *>> *fi)
{
    std::unique_ptr<QFutureInterface<QList<FileNode *>>> fip(fi);
    fip->reportStarted();
    Utils::MimeDatabase mdb;

    QList<FileNode *> nodes
            = FileNode::scanForFiles(m_buildConfiguration->target()->project()->projectDirectory(),
                                     [&mdb](const Utils::FileName &fn) -> FileNode * {
                                         QTC_ASSERT(!fn.isEmpty(), return nullptr);
                                         const Utils::MimeType mimeType = mdb.mimeTypeForFile(fn.toString());
                                         FileType type = FileType::Unknown;
                                         if (mimeType.isValid()) {
                                             const QString mt = mimeType.name();
                                             if (mt == CppTools::Constants::C_SOURCE_MIMETYPE
                                                     || mt == CppTools::Constants::CPP_SOURCE_MIMETYPE
                                                     || mt == CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE
                                                     || mt == CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE
                                                     || mt == CppTools::Constants::QDOC_MIMETYPE
                                                     || mt == CppTools::Constants::MOC_MIMETYPE)
                                                 type = FileType::Source;
                                             else if (mt == CppTools::Constants::C_HEADER_MIMETYPE
                                                      || mt == CppTools::Constants::CPP_HEADER_MIMETYPE)
                                                 type = FileType::Header;
                                             else if (mt == ProjectExplorer::Constants::FORM_MIMETYPE)
                                                 type = FileType::Form;
                                             else if (mt == ProjectExplorer::Constants::RESOURCE_MIMETYPE)
                                                 type = FileType::Resource;
                                             else if (mt == ProjectExplorer::Constants::SCXML_MIMETYPE)
                                                 type = FileType::StateChart;
                                             else if (mt == CMakeProjectManager::Constants::CMAKEPROJECTMIMETYPE
                                                      || mt == CMakeProjectManager::Constants::CMAKEMIMETYPE)
                                                 type = FileType::Project;
                                             else if (mt == ProjectExplorer::Constants::QML_MIMETYPE)
                                                 type = FileType::QML;
                                         }
                                         return new FileNode(fn, type, false);
                                     },
                                     fip.get());
    fip->setProgressValue(fip->progressMaximum());
    fip->reportResult(nodes);
    fip->reportFinished();
}

void BuildDirManager::forceReparse()
{
    if (m_buildConfiguration->target()->activeBuildConfiguration() != m_buildConfiguration)
        return;

    CMakeTool *tool = CMakeKitInformation::cmakeTool(m_buildConfiguration->target()->kit());
    QTC_ASSERT(tool, return);

    m_reader.reset(); // Force reparse by forcing in a new reader
    updateReaderType([this]() { parseOnceReaderReady(true); });
}

void BuildDirManager::resetData()
{
    if (m_reader)
        m_reader->resetData();

    m_cmakeCache.clear();
    m_futureWatcher.setFuture(QFuture<QList<FileNode *>>());
    m_reader.reset();
}

bool BuildDirManager::updateCMakeStateBeforeBuild()
{
    return m_reparseTimer.isActive();
}

bool BuildDirManager::persistCMakeState()
{
    if (!m_tempDir)
        return false;

    const QString buildDir = m_buildConfiguration->buildDirectory().toString();
    QDir dir(buildDir);
    dir.mkpath(buildDir);

    m_tempDir.reset(nullptr);

    QTimer::singleShot(0, this, &BuildDirManager::parse); // make sure signals only happen afterwards!
    return true;
}

void BuildDirManager::generateProjectTree(CMakeListsNode *root)
{
    QTC_ASSERT(m_reader, return);
    QTC_ASSERT(m_scanFuture.isFinished(), return);

    const Utils::FileName projectFile = m_buildConfiguration->target()->project()->projectFilePath();
    QList<FileNode *> tmp = Utils::filtered(m_scanFuture.result(),
                                            [projectFile](const FileNode *fn) -> bool {
        return !fn->filePath().toString().startsWith(projectFile.toString() + ".user");
    });
    Utils::sort(tmp, ProjectExplorer::Node::sortByPath);

    m_scanFuture = QFuture<QList<FileNode *>>(); // flush stale results

    const QList<FileNode *> allFiles = tmp;
    m_reader->generateProjectTree(root, allFiles);
    QSet<FileNode *> usedNodes;
    foreach (FileNode *fn, root->recursiveFileNodes())
        usedNodes.insert(fn);

    // Make sure the top level CMakeLists.txt is always visible:
    if (root->fileNodes().isEmpty()
            && root->folderNodes().isEmpty()
            && root->projectNodes().isEmpty()) {
        FileNode *cm = Utils::findOrDefault(allFiles, [&projectFile](const FileNode *fn) {
            return fn->filePath() == projectFile;
        });
        if (cm) {
            root->addFileNodes({ cm });
            usedNodes.insert(cm);
        }
    }

    QList<FileNode *> leftOvers = Utils::filtered(allFiles, [&usedNodes](FileNode *fn) {
            return !usedNodes.contains(fn);
    });
    qDeleteAll(leftOvers);
}

QSet<Core::Id> BuildDirManager::updateCodeModel(CppTools::ProjectPartBuilder &ppBuilder)
{
    QTC_ASSERT(m_reader, return QSet<Core::Id>());
    return m_reader->updateCodeModel(ppBuilder);
}

void BuildDirManager::parse()
{
    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    updateReaderType([this]() { parseOnceReaderReady(false); });
}

void BuildDirManager::clearCache()
{
    auto cmakeCache = Utils::FileName(workDirectory()).appendPath(QLatin1String("CMakeCache.txt"));
    auto cmakeFiles = Utils::FileName(workDirectory()).appendPath(QLatin1String("CMakeFiles"));

    const bool mustCleanUp = cmakeCache.exists() || cmakeFiles.exists();
    if (!mustCleanUp)
        return;

    Utils::FileUtils::removeRecursively(cmakeCache);
    Utils::FileUtils::removeRecursively(cmakeFiles);

    forceReparse();
}

QList<CMakeBuildTarget> BuildDirManager::buildTargets() const
{
    if (!m_reader)
        return QList<CMakeBuildTarget>();
    return m_reader->buildTargets();
}

CMakeConfig BuildDirManager::parsedConfiguration() const
{
    if (!m_reader)
        return m_cmakeCache;
    if (m_cmakeCache.isEmpty())
        m_cmakeCache = m_reader->parsedConfiguration();
    return m_cmakeCache;
}

void BuildDirManager::checkConfiguration()
{
    if (m_tempDir) // always throw away changes in the tmpdir!
        return;

    Kit *k = m_buildConfiguration->target()->kit();
    const CMakeConfig cache = m_reader->parsedConfiguration();
    if (cache.isEmpty())
        return; // No cache file yet.

    CMakeConfig newConfig;
    QSet<QString> changedKeys;
    QSet<QString> removedKeys;
    foreach (const CMakeConfigItem &iBc, m_buildConfiguration->cmakeConfiguration()) {
        const CMakeConfigItem &iCache
                = Utils::findOrDefault(cache, [&iBc](const CMakeConfigItem &i) { return i.key == iBc.key; });
        if (iCache.isNull()) {
            removedKeys << QString::fromUtf8(iBc.key);
        } else if (QString::fromUtf8(iCache.value) != iBc.expandedValue(k)) {
            changedKeys << QString::fromUtf8(iBc.key);
            newConfig.append(iCache);
        } else {
            newConfig.append(iBc);
        }
    }

    if (!changedKeys.isEmpty() || !removedKeys.isEmpty()) {
        QSet<QString> total = removedKeys + changedKeys;
        QStringList keyList = total.toList();
        Utils::sort(keyList);
        QString table = QLatin1String("<table>");
        foreach (const QString &k, keyList) {
            QString change;
            if (removedKeys.contains(k))
                change = tr("<removed>");
            else
                change = QString::fromUtf8(CMakeConfigItem::valueOf(k.toUtf8(), cache)).trimmed();
            if (change.isEmpty())
                change = tr("<empty>");
            table += QString::fromLatin1("\n<tr><td>%1</td><td>%2</td></tr>").arg(k).arg(change.toHtmlEscaped());
        }
        table += QLatin1String("\n</table>");

        QPointer<QMessageBox> box = new QMessageBox(Core::ICore::mainWindow());
        box->setText(tr("CMake configuration has changed on disk."));
        box->setInformativeText(tr("The CMakeCache.txt file has changed: %1").arg(table));
        auto *defaultButton = box->addButton(tr("Overwrite Changes in CMake"), QMessageBox::RejectRole);
        box->addButton(tr("Apply Changes to Project"), QMessageBox::AcceptRole);
        box->setDefaultButton(defaultButton);

        int ret = box->exec();
        if (ret == QMessageBox::Apply) {
            m_buildConfiguration->setCMakeConfiguration(newConfig);
            updateReaderData(); // Apply changes to reader
        }
    }
}

void BuildDirManager::maybeForceReparse()
{
    if (!m_reader || !m_reader->hasData()) {
        forceReparse();
        return;
    }

    updateReaderType([this]() { maybeForceReparseOnceReaderReady(); });
}

} // namespace Internal
} // namespace CMakeProjectManager
