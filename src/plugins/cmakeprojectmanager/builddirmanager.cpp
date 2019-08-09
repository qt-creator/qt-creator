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
#include "cmakebuildstep.h"
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

#include <QDir>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

Q_LOGGING_CATEGORY(cmakeBuildDirManagerLog, "qtc.cmake.builddirmanager", QtWarningMsg);

// --------------------------------------------------------------------
// BuildDirManager:
// --------------------------------------------------------------------

BuildDirManager::BuildDirManager(CMakeProject *project) : m_project(project) { assert(project); }

BuildDirManager::~BuildDirManager() = default;

Utils::FilePath BuildDirManager::workDirectory(const BuildDirParameters &parameters) const
{
    const Utils::FilePath bdir = parameters.buildDirectory;
    const CMakeTool *cmake = parameters.cmakeTool();
    if (bdir.exists()) {
        m_buildDirToTempDir.erase(bdir);
        return bdir;
    } else {
        if (cmake && cmake->autoCreateBuildDirectory()) {
            if (!QDir().mkpath(bdir.toString()))
                emitErrorOccured(tr("Failed to create build directory \"%1\".").arg(bdir.toUserOutput()));
            return bdir;
        }
    }
    auto tmpDirIt = m_buildDirToTempDir.find(bdir);
    if (tmpDirIt == m_buildDirToTempDir.end()) {
        auto ret = m_buildDirToTempDir.emplace(std::make_pair(bdir, std::make_unique<Utils::TemporaryDirectory>("qtc-cmake-XXXXXXXX")));
        QTC_ASSERT(ret.second, return bdir);
        tmpDirIt = ret.first;

        if (!tmpDirIt->second->isValid()) {
            emitErrorOccured(tr("Failed to create temporary directory \"%1\".")
                             .arg(QDir::toNativeSeparators(tmpDirIt->second->path())));
            return bdir;
        }
    }
    return Utils::FilePath::fromString(tmpDirIt->second->path());
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

void BuildDirManager::updateReaderType(const BuildDirParameters &p,
                                       std::function<void()> todo)
{
    if (!m_reader || !m_reader->isCompatible(p))
        m_reader = BuildDirReader::createReader(p);

    QTC_ASSERT(m_reader, return);

    connect(m_reader.get(), &BuildDirReader::configurationStarted,
            this, &BuildDirManager::parsingStarted);
    connect(m_reader.get(), &BuildDirReader::dataAvailable,
            this, &BuildDirManager::emitDataAvailable);
    connect(m_reader.get(), &BuildDirReader::errorOccured,
            this, &BuildDirManager::emitErrorOccured);
    connect(m_reader.get(), &BuildDirReader::dirty, this, &BuildDirManager::becameDirty);
    connect(m_reader.get(), &BuildDirReader::isReadyNow, this, todo);

    m_reader->setParameters(p);
}

bool BuildDirManager::hasConfigChanged()
{
    checkConfiguration();

    const QByteArray GENERATOR_KEY = "CMAKE_GENERATOR";
    const QByteArray EXTRA_GENERATOR_KEY = "CMAKE_EXTRA_GENERATOR";
    const QByteArray CMAKE_COMMAND_KEY = "CMAKE_COMMAND";
    const QByteArray CMAKE_C_COMPILER_KEY = "CMAKE_C_COMPILER";
    const QByteArray CMAKE_CXX_COMPILER_KEY = "CMAKE_CXX_COMPILER";

    const QByteArrayList criticalKeys
            = {GENERATOR_KEY, CMAKE_COMMAND_KEY, CMAKE_C_COMPILER_KEY, CMAKE_CXX_COMPILER_KEY};

    QString errorMessage;
    const CMakeConfig currentConfig = takeCMakeConfiguration(errorMessage);
    if (!errorMessage.isEmpty())
        return false;

    const CMakeTool *tool = m_parameters.cmakeTool();
    QTC_ASSERT(tool, return false); // No cmake... we should not have ended up here in the first place
    const QString extraKitGenerator = m_parameters.extraGenerator;
    const QString mainKitGenerator = m_parameters.generator;
    CMakeConfig targetConfig = m_parameters.configuration;
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
                    return false; // no need to trigger a new reader, clearCache will do that
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
    return mustReparse || kcit != targetConfig.constEnd();
}

bool BuildDirManager::isParsing() const
{
    return m_reader && m_reader->isParsing();
}

void BuildDirManager::stopParsingAndClearState()
{
    if (m_reader) {
        disconnect(m_reader.get(), nullptr, this, nullptr);
        m_reader->stop();
    }
    m_reader.reset();
    resetData();
}

void BuildDirManager::setParametersAndRequestParse(const BuildDirParameters &parameters,
                                                   int reparseOptions)
{
    if (!parameters.cmakeTool()) {
        TaskHub::addTask(Task::Error,
                         tr("The kit needs to define a CMake tool to parse this project."),
                         ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        return;
    }
    QTC_ASSERT(parameters.isValid(), return );

    stopParsingAndClearState();

    m_parameters = parameters;
    m_parameters.workDirectory = workDirectory(parameters);

    updateReaderType(m_parameters,
                     [this, reparseOptions]() { emit requestReparse(reparseOptions); });
}

CMakeBuildConfiguration *BuildDirManager::buildConfiguration() const
{
    if (m_project->activeTarget() && m_project->activeTarget()->activeBuildConfiguration() == m_parameters.buildConfiguration)
        return m_parameters.buildConfiguration;
    return nullptr;
}

FilePath BuildDirManager::buildDirectory() const
{
    return buildConfiguration() ? m_parameters.buildDirectory : FilePath();
}

void BuildDirManager::becameDirty()
{
    if (isParsing() || !buildConfiguration())
        return;

    const CMakeTool *tool = m_parameters.cmakeTool();
    if (!tool->isAutoRun())
        return;

    emit requestReparse(REPARSE_CHECK_CONFIGURATION | REPARSE_SCAN);
}

void BuildDirManager::resetData()
{
    if (m_reader)
        m_reader->resetData();
}

bool BuildDirManager::persistCMakeState()
{
    QTC_ASSERT(m_parameters.isValid(), return false);

    if (m_parameters.workDirectory == m_parameters.buildDirectory)
        return false;

    const Utils::FilePath buildDir = m_parameters.buildDirectory;
    QDir dir(buildDir.toString());
    dir.mkpath(buildDir.toString());

    BuildDirParameters newParameters = m_parameters;
    newParameters.workDirectory.clear();
    setParametersAndRequestParse(newParameters,
                                 REPARSE_URGENT | REPARSE_FORCE_CMAKE_RUN
                                     | REPARSE_FORCE_CONFIGURATION | REPARSE_CHECK_CONFIGURATION);
    return true;
}

void BuildDirManager::parse(int reparseParameters)
{
    qCDebug(cmakeBuildDirManagerLog)
        << "Parse called with flags:" << flagsString(reparseParameters);

    QTC_ASSERT(m_parameters.isValid(), return );
    QTC_ASSERT(m_reader, return);
    QTC_ASSERT((reparseParameters & REPARSE_IGNORE) == 0, return);

    m_reader->stop();

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    const QString cache = m_parameters.workDirectory.pathAppended("CMakeCache.txt").toString();
    if (!QFileInfo::exists(cache)) {
        reparseParameters |= REPARSE_FORCE_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN;
        qCDebug(cmakeBuildDirManagerLog)
            << "No" << cache << "file found, new flags:" << flagsString(reparseParameters);
    } else if (reparseParameters & REPARSE_CHECK_CONFIGURATION) {
        if (checkConfiguration()) {
            reparseParameters |= REPARSE_FORCE_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN;
            qCDebug(cmakeBuildDirManagerLog)
                << "Config check triggered flags change:" << flagsString(reparseParameters);
        }
    }

    m_reader->parse(reparseParameters & REPARSE_FORCE_CMAKE_RUN,
                    reparseParameters & REPARSE_FORCE_CONFIGURATION);
}

std::unique_ptr<CMakeProjectNode> BuildDirManager::generateProjectTree(
    const QList<const FileNode *> &allFiles, QString &errorMessage) const
{
    QTC_ASSERT(!m_isHandlingError, return {});
    QTC_ASSERT(m_reader, return {});

    return m_reader->generateProjectTree(allFiles, errorMessage);
}

CppTools::RawProjectParts BuildDirManager::createRawProjectParts(QString &errorMessage) const
{
    QTC_ASSERT(!m_isHandlingError, return {});
    QTC_ASSERT(m_reader, return {});
    return m_reader->createRawProjectParts(errorMessage);
}

void BuildDirManager::clearCache()
{
    QTC_ASSERT(m_parameters.isValid(), return);
    QTC_ASSERT(!m_isHandlingError, return);

    const FilePath cmakeCache = m_parameters.workDirectory.pathAppended("CMakeCache.txt");
    const FilePath cmakeFiles = m_parameters.workDirectory.pathAppended("CMakeFiles");

    const bool mustCleanUp = cmakeCache.exists() || cmakeFiles.exists();
    if (!mustCleanUp)
        return;

    Utils::FileUtils::removeRecursively(cmakeCache);
    Utils::FileUtils::removeRecursively(cmakeFiles);

    m_reader.reset();
}

static CMakeBuildTarget utilityTarget(const QString &title, const BuildDirManager *bdm)
{
    CMakeBuildTarget target;

    target.title = title;
    target.targetType = UtilityType;
    target.workingDirectory = bdm->buildDirectory();
    target.sourceDirectory = bdm->project()->projectDirectory();

    return target;
}

QList<CMakeBuildTarget> BuildDirManager::takeBuildTargets(QString &errorMessage) const
{
    QList<CMakeBuildTarget> result = { utilityTarget(CMakeBuildStep::allTarget(), this),
                                       utilityTarget(CMakeBuildStep::cleanTarget(), this),
                                       utilityTarget(CMakeBuildStep::installTarget(), this),
                                       utilityTarget(CMakeBuildStep::testTarget(), this) };
    QTC_ASSERT(!m_isHandlingError, return result);

    if (m_reader) {
        QList<CMakeBuildTarget> readerTargets
            = Utils::filtered(m_reader->takeBuildTargets(errorMessage),
                              [](const CMakeBuildTarget &bt) {
                                  return bt.title != CMakeBuildStep::allTarget()
                                         && bt.title != CMakeBuildStep::cleanTarget()
                                         && bt.title != CMakeBuildStep::installTarget()
                                         && bt.title != CMakeBuildStep::testTarget();
                              });

        // Guess at the target definition position when no details are known
        for (CMakeBuildTarget &t : readerTargets) {
            if (t.backtrace.isEmpty()) {
                t.backtrace.append(
                    FolderNode::LocationInfo(tr("CMakeLists.txt in source directory"),
                                             t.sourceDirectory.pathAppended("CMakeLists.txt")));
            }
        }
        result.append(readerTargets);
    }
    return result;
}

CMakeConfig BuildDirManager::takeCMakeConfiguration(QString &errorMessage) const
{
    if (!m_reader)
        return CMakeConfig();

    CMakeConfig result = m_reader->takeParsedConfiguration(errorMessage);
    for (auto &ci : result)
        ci.inCMakeCache = true;

    return result;
}

CMakeConfig BuildDirManager::parseCMakeConfiguration(const Utils::FilePath &cacheFile,
                                                     QString *errorMessage)
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

QString BuildDirManager::flagsString(int reparseFlags)
{
    QString result;
    if (reparseFlags == REPARSE_DEFAULT) {
        result = "<NONE>";
    } else {
        if (reparseFlags & REPARSE_URGENT)
            result += " URGENT";
        if (reparseFlags & REPARSE_FORCE_CMAKE_RUN)
            result += " FORCE_CMAKE_RUN";
        if (reparseFlags & REPARSE_FORCE_CONFIGURATION)
            result += " FORCE_CONFIG";
        if (reparseFlags & REPARSE_CHECK_CONFIGURATION)
            result += " CHECK_CONFIG";
        if (reparseFlags & REPARSE_SCAN)
            result += " SCAN";
        if (reparseFlags & REPARSE_IGNORE)
            result += " IGNORE";
    }
    return result.trimmed();
}

bool BuildDirManager::checkConfiguration()
{
    CMakeBuildConfiguration *bc = buildConfiguration();
    QTC_ASSERT(m_parameters.isValid() || !bc, return false);

    if (m_parameters.workDirectory != m_parameters.buildDirectory) // always throw away changes in the tmpdir!
        return false;

    const CMakeConfig cache = bc->configurationFromCMake();
    if (cache.isEmpty())
        return false; // No cache file yet.

    CMakeConfig newConfig;
    QHash<QString, QPair<QString, QString>> changedKeys;
    foreach (const CMakeConfigItem &projectItem, m_parameters.configuration) {
        const QString projectKey = QString::fromUtf8(projectItem.key);
        const QString projectValue = projectItem.expandedValue(m_parameters.expander);
        const CMakeConfigItem &cmakeItem
                = Utils::findOrDefault(cache, [&projectItem](const CMakeConfigItem &i) { return i.key == projectItem.key; });
        const QString iCacheValue = QString::fromUtf8(cmakeItem.value);
        if (cmakeItem.isNull()) {
            changedKeys.insert(projectKey, qMakePair(tr("<removed>"), projectValue));
        } else if (iCacheValue != projectValue) {
            changedKeys.insert(projectKey, qMakePair(iCacheValue, projectValue));
            newConfig.append(cmakeItem);
        } else {
            newConfig.append(projectItem);
        }
    }

    if (!changedKeys.isEmpty()) {
        QStringList keyList = changedKeys.keys();
        Utils::sort(keyList);
        QString table = QString::fromLatin1("<table><tr><th>%1</th><th>%2</th><th>%3</th></tr>")
                .arg(tr("Key")).arg(tr("CMakeCache.txt")).arg(tr("Project"));
        foreach (const QString &k, keyList) {
            const QPair<QString, QString> data = changedKeys.value(k);
            table += QString::fromLatin1("\n<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                    .arg(k)
                    .arg(data.first.toHtmlEscaped())
                    .arg(data.second.toHtmlEscaped());
        }
        table += QLatin1String("\n</table>");

        QPointer<QMessageBox> box = new QMessageBox(Core::ICore::mainWindow());
        box->setText(tr("CMake configuration has changed on disk."));
        box->setInformativeText(table);
        auto *defaultButton = box->addButton(tr("Overwrite Changes in CMakeCache.txt"), QMessageBox::RejectRole);
        auto *applyButton = box->addButton(tr("Apply Changes to Project"), QMessageBox::ApplyRole);
        box->setDefaultButton(defaultButton);

        box->exec();
        if (box->clickedButton() == applyButton) {
            m_parameters.configuration = newConfig;
            QSignalBlocker blocker(bc);
            bc->setConfigurationForCMake(newConfig);
            return false;
        } else if (box->clickedButton() == defaultButton)
            return true;
    }
    return false;
}

} // namespace Internal
} // namespace CMakeProjectManager
