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
#include "cmakeprojectnodes.h"
#include "cmaketool.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QSet>

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
}

BuildDirManager::~BuildDirManager() = default;

const Utils::FileName BuildDirManager::workDirectory() const
{
    const Utils::FileName bdir = m_buildConfiguration->buildDirectory();
    if (bdir.exists())
        return bdir;
    if (!m_tempDir) {
        m_tempDir.reset(new Utils::TemporaryDirectory("qtc-cmake-XXXXXXXX"));
        if (!m_tempDir->isValid())
            emitErrorOccured(tr("Failed to create temporary directory \"%1\".").arg(m_tempDir->path()));
    }
    return Utils::FileName::fromString(m_tempDir->path());
}

void BuildDirManager::emitDataAvailable()
{
    if (!isParsing())
        emit dataAvailable();
}

void BuildDirManager::emitErrorOccured(const QString &message) const
{
    m_isHandlingError = true;
    emit errorOccured(message);
    m_isHandlingError = false;
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
                this, &BuildDirManager::emitErrorOccured);
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
    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    m_buildTargets.clear();
    m_cmakeCache.clear();
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
    const QByteArray CMAKE_C_COMPILER_KEY = "CMAKE_C_COMPILER";
    const QByteArray CMAKE_CXX_COMPILER_KEY = "CMAKE_CXX_COMPILER";

    const QByteArrayList criticalKeys
            = {GENERATOR_KEY, CMAKE_COMMAND_KEY, CMAKE_C_COMPILER_KEY, CMAKE_CXX_COMPILER_KEY};

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

void BuildDirManager::forceReparse()
{
    QTC_ASSERT(!m_isHandlingError, return);

    if (m_buildConfiguration->target()->activeBuildConfiguration() != m_buildConfiguration)
        return;

    CMakeTool *tool = CMakeKitInformation::cmakeTool(m_buildConfiguration->target()->kit());
    QTC_ASSERT(tool, return);

    m_reader.reset(); // Force reparse by forcing in a new reader
    updateReaderType([this]() { parseOnceReaderReady(true); });
}

void BuildDirManager::resetData()
{
    QTC_ASSERT(!m_isHandlingError, return);

    if (m_reader)
        m_reader->resetData();

    m_cmakeCache.clear();
    m_reader.reset();

    m_buildTargets.clear();
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

void BuildDirManager::generateProjectTree(CMakeProjectNode *root, const QList<const FileNode *> &allFiles)
{
    QTC_ASSERT(!m_isHandlingError, return);
    QTC_ASSERT(m_reader, return);

    const Utils::FileName projectFile = m_buildConfiguration->target()->project()->projectFilePath();

    m_reader->generateProjectTree(root, allFiles);

    // Make sure the top level CMakeLists.txt is always visible:
    if (root->isEmpty())
        root->addNode(new FileNode(projectFile, FileType::Project, false));
}

void BuildDirManager::updateCodeModel(CppTools::RawProjectParts &rpps)
{
    QTC_ASSERT(!m_isHandlingError, return);
    QTC_ASSERT(m_reader, return);
    return m_reader->updateCodeModel(rpps);
}

void BuildDirManager::parse()
{
    updateReaderType([this]() { parseOnceReaderReady(false); });
}

void BuildDirManager::clearCache()
{
    QTC_ASSERT(!m_isHandlingError, return);

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
    QTC_ASSERT(!m_isHandlingError, return {});

    if (!m_reader)
        return QList<CMakeBuildTarget>();
    if (m_buildTargets.isEmpty())
        m_buildTargets = m_reader->buildTargets();
    return m_buildTargets;
}

CMakeConfig BuildDirManager::parsedConfiguration() const
{
    QTC_ASSERT(!m_isHandlingError, return {});

    if (!m_reader)
        return m_cmakeCache;
    if (m_cmakeCache.isEmpty())
        m_cmakeCache = m_reader->takeParsedConfiguration();

    for (auto &ci : m_cmakeCache)
        ci.inCMakeCache = true;

    return m_cmakeCache;
}

CMakeConfig BuildDirManager::parseConfiguration(const Utils::FileName &cacheFile, QString *errorMessage)
{
    if (!cacheFile.exists()) {
        if (errorMessage)
            *errorMessage = tr("CMakeCache.txt file not found.");
        return { };
    }
    CMakeConfig result = CMakeConfigItem::itemsFromFile(cacheFile, errorMessage);
    if (!errorMessage->isEmpty())
        return { };
    return result;
}

void BuildDirManager::checkConfiguration()
{
    if (m_tempDir) // always throw away changes in the tmpdir!
        return;

    Kit *k = m_buildConfiguration->target()->kit();
    const CMakeConfig cache = parsedConfiguration();
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
        if (ret == QMessageBox::Apply)
            m_buildConfiguration->setCMakeConfiguration(newConfig);
    }
}

void BuildDirManager::maybeForceReparse()
{
    if (m_isHandlingError)
        return;

    if (!m_reader || !m_reader->hasData()) {
        forceReparse();
        return;
    }

    updateReaderType([this]() { maybeForceReparseOnceReaderReady(); });
}

} // namespace Internal
} // namespace CMakeProjectManager
