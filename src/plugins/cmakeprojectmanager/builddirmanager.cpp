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
#include <QMessageBox>
#include <QPushButton>
#include <QSet>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BuildDirManager:
// --------------------------------------------------------------------

BuildDirManager::BuildDirManager() = default;
BuildDirManager::~BuildDirManager() = default;

Utils::FileName BuildDirManager::workDirectory(const BuildDirParameters &parameters) const
{
    const Utils::FileName bdir = parameters.buildDirectory;
    const CMakeTool *cmake = parameters.cmakeTool;
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
    return Utils::FileName::fromString(tmpDirIt->second->path());
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
    if (!m_reader || !m_reader->isCompatible(p)) {
        m_reader.reset(BuildDirReader::createReader(p));
        connect(m_reader.get(), &BuildDirReader::configurationStarted,
                this, &BuildDirManager::parsingStarted);
        connect(m_reader.get(), &BuildDirReader::dataAvailable,
                this, &BuildDirManager::emitDataAvailable);
        connect(m_reader.get(), &BuildDirReader::errorOccured,
                this, &BuildDirManager::emitErrorOccured);
        connect(m_reader.get(), &BuildDirReader::dirty, this, &BuildDirManager::becameDirty);
    }
    QTC_ASSERT(m_reader, return);

    m_reader->setParameters(p);

    if (m_reader->isReady())
        todo();
    else
        connect(m_reader.get(), &BuildDirReader::isReadyNow, this, todo);
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

    const CMakeConfig currentConfig = takeCMakeConfiguration();

    const CMakeTool *tool = m_parameters.cmakeTool;
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

void BuildDirManager::setParametersAndRequestParse(const BuildDirParameters &parameters,
                                                   int newReaderReparseOptions,
                                                   int existingReaderReparseOptions)
{
    QTC_ASSERT(parameters.isValid(), return);

    if (m_reader)
        m_reader->stop();

    BuildDirReader *old = m_reader.get();

    m_parameters = parameters;
    m_parameters.workDirectory = workDirectory(parameters);

    updateReaderType(m_parameters,
                     [this, old, newReaderReparseOptions, existingReaderReparseOptions]() {
        int options = REPARSE_DEFAULT;
        if (old != m_reader.get()) {
            options = newReaderReparseOptions;
        } else {
            if (!QFileInfo::exists(m_parameters.workDirectory.toString() + "/CMakeCache.txt"))
                options = newReaderReparseOptions;
            else
                options = existingReaderReparseOptions;
        }
        emit requestReparse(options);
    });
}

CMakeBuildConfiguration *BuildDirManager::buildConfiguration() const
{
    return m_parameters.buildConfiguration;
}

void BuildDirManager::becameDirty()
{
    if (isParsing())
        return;

    if (!m_parameters.buildConfiguration || !m_parameters.buildConfiguration->isActive())
        return;

    const CMakeTool *tool = m_parameters.cmakeTool;
    if (!tool->isAutoRun())
        return;

    emit requestReparse(REPARSE_CHECK_CONFIGURATION);
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

    const Utils::FileName buildDir = m_parameters.buildDirectory;
    QDir dir(buildDir.toString());
    dir.mkpath(buildDir.toString());

    BuildDirParameters newParameters = m_parameters;
    newParameters.workDirectory.clear();
    setParametersAndRequestParse(newParameters, REPARSE_URGENT | REPARSE_FORCE_CONFIGURATION | REPARSE_CHECK_CONFIGURATION,
                                 REPARSE_FAIL);
    return true;
}

void BuildDirManager::parse(int reparseParameters)
{
    QTC_ASSERT(m_parameters.isValid(), return);
    QTC_ASSERT(m_reader, return);
    QTC_ASSERT((reparseParameters & REPARSE_FAIL) == 0, return);
    QTC_ASSERT((reparseParameters & REPARSE_IGNORE) == 0, return);

    m_reader->stop();

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    if (reparseParameters & REPARSE_CHECK_CONFIGURATION) {
        if (checkConfiguration())
            reparseParameters |= REPARSE_FORCE_CONFIGURATION;
    }

    m_reader->parse(reparseParameters & REPARSE_FORCE_CONFIGURATION);
}

void BuildDirManager::generateProjectTree(CMakeProjectNode *root, const QList<const FileNode *> &allFiles) const
{
    QTC_ASSERT(!m_isHandlingError, return);
    QTC_ASSERT(m_reader, return);

    m_reader->generateProjectTree(root, allFiles);
}

void BuildDirManager::updateCodeModel(CppTools::RawProjectParts &rpps)
{
    QTC_ASSERT(!m_isHandlingError, return);
    QTC_ASSERT(m_reader, return);
    return m_reader->updateCodeModel(rpps);
}

void BuildDirManager::clearCache()
{
    QTC_ASSERT(m_parameters.isValid(), return);
    QTC_ASSERT(!m_isHandlingError, return);

    auto cmakeCache = m_parameters.workDirectory.appendPath("CMakeCache.txt");
    auto cmakeFiles = m_parameters.workDirectory.appendPath("CMakeFiles");

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
    target.workingDirectory = bdm->buildConfiguration()->buildDirectory();
    target.sourceDirectory = bdm->buildConfiguration()->target()->project()->projectDirectory();

    return target;
}

QList<CMakeBuildTarget> BuildDirManager::takeBuildTargets() const
{
    QList<CMakeBuildTarget> result = { utilityTarget(CMakeBuildStep::allTarget(), this),
                                       utilityTarget(CMakeBuildStep::cleanTarget(), this),
                                       utilityTarget(CMakeBuildStep::installTarget(), this),
                                       utilityTarget(CMakeBuildStep::testTarget(), this) };
    QTC_ASSERT(!m_isHandlingError, return result);

    if (m_reader) {
        result.append(Utils::filtered(m_reader->takeBuildTargets(), [](const CMakeBuildTarget &bt) {
            return bt.title != CMakeBuildStep::allTarget()
                    && bt.title != CMakeBuildStep::cleanTarget()
                    && bt.title != CMakeBuildStep::installTarget()
                    && bt.title != CMakeBuildStep::testTarget();
        }));
    }
    return result;
}

CMakeConfig BuildDirManager::takeCMakeConfiguration() const
{
    QTC_ASSERT(!m_isHandlingError, return {});

    if (!m_reader)
        return CMakeConfig();

    CMakeConfig result = m_reader->takeParsedConfiguration();
    for (auto &ci : result)
        ci.inCMakeCache = true;

    return result;
}

CMakeConfig BuildDirManager::parseCMakeConfiguration(const Utils::FileName &cacheFile,
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

bool BuildDirManager::checkConfiguration()
{
    QTC_ASSERT(m_parameters.isValid(), return false);

    if (m_parameters.workDirectory != m_parameters.buildDirectory) // always throw away changes in the tmpdir!
        return false;

    const CMakeConfig cache = m_parameters.buildConfiguration->configurationFromCMake();
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
                .arg(tr("Key")).arg(tr("CMake")).arg(tr("Project"));
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
        auto *defaultButton = box->addButton(tr("Overwrite Changes in CMake"), QMessageBox::RejectRole);
        auto *applyButton = box->addButton(tr("Apply Changes to Project"), QMessageBox::ApplyRole);
        box->setDefaultButton(defaultButton);

        box->exec();
        if (box->clickedButton() == applyButton) {
            m_parameters.configuration = newConfig;
            QSignalBlocker blocker(m_parameters.buildConfiguration);
            m_parameters.buildConfiguration->setConfigurationForCMake(newConfig);
            return false;
        } else if (box->clickedButton() == defaultButton)
            return true;
    }
    return false;
}

} // namespace Internal
} // namespace CMakeProjectManager
