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

#include "fileapireader.h"

#include "cmakebuildconfiguration.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanager.h"
#include "fileapidataextractor.h"
#include "projecttreehelper.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QDateTime>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

Q_LOGGING_CATEGORY(cmakeFileApiMode, "qtc.cmake.fileApiMode", QtWarningMsg);

using namespace FileApiDetails;

// --------------------------------------------------------------------
// FileApiReader:
// --------------------------------------------------------------------

FileApiReader::FileApiReader()
{
    connect(Core::EditorManager::instance(),
            &Core::EditorManager::aboutToSave,
            this,
            [this](const Core::IDocument *document) {
                if (m_cmakeFiles.contains(document->filePath())) {
                    qCDebug(cmakeFileApiMode) << "FileApiReader: DIRTY SIGNAL";
                    emit dirty();
                }
            });
}

FileApiReader::~FileApiReader()
{
    stop();
    resetData();
}

void FileApiReader::setParameters(const BuildDirParameters &p)
{
    qCDebug(cmakeFileApiMode)
        << "\n\n\n\n\n=============================================================\n";

    // Update:
    m_parameters = p;
    qCDebug(cmakeFileApiMode) << "Work directory:" << m_parameters.workDirectory.toUserOutput();

    resetData();

    m_fileApi = std::make_unique<FileApiParser>(m_parameters.sourceDirectory, m_parameters.workDirectory);
    connect(m_fileApi.get(), &FileApiParser::dirty, this, [this]() {
        if (!m_isParsing)
            emit dirty();
    });

    qCDebug(cmakeFileApiMode) << "FileApiReader: IS READY NOW SIGNAL";
    emit isReadyNow();
}

bool FileApiReader::isCompatible(const BuildDirParameters &p)
{
    const CMakeTool *cmakeTool = p.cmakeTool();
    return cmakeTool && cmakeTool->readerType() == CMakeTool::FileApi;
}

void FileApiReader::resetData()
{
    m_cmakeFiles.clear();
    if (!m_parameters.sourceDirectory.isEmpty())
        m_cmakeFiles.insert(m_parameters.sourceDirectory.pathAppended("CMakeLists.txt"));

    m_cache.clear();
    m_buildTargets.clear();
    m_projectParts.clear();
    m_rootProjectNode.reset();
    m_knownHeaders.clear();
}

void FileApiReader::parse(bool forceCMakeRun, bool forceConfiguration)
{
    qCDebug(cmakeFileApiMode) << "Parse called with arguments: ForceCMakeRun:" << forceCMakeRun
                              << " - forceConfiguration:" << forceConfiguration;
    startState();

    if (forceConfiguration) {
        // Initial create:
        qCDebug(cmakeFileApiMode) << "FileApiReader: Starting CMake with forced configuration.";
        startCMakeState(
            CMakeProcess::toArguments(m_parameters.configuration, m_parameters.expander));
        // Keep m_isParsing enabled!
        return;
    }

    const QFileInfo replyFi = m_fileApi->scanForCMakeReplyFile();
    // Only need to update when one of the following conditions is met:
    //  * The user forces the update,
    //  * There is no reply file,
    //  * One of the cmakefiles is newer than the replyFile and the user asked
    //    for creator to run CMake as needed,
    //  * A query files are newer than the reply file
    const bool mustUpdate = forceCMakeRun || !replyFi.exists()
                            || (m_parameters.cmakeTool() && m_parameters.cmakeTool()->isAutoRun()
                                && anyOf(m_cmakeFiles,
                                         [&replyFi](const FilePath &f) {
                                             return f.toFileInfo().lastModified()
                                                    > replyFi.lastModified();
                                         }))
                            || anyOf(m_fileApi->cmakeQueryFilePaths(), [&replyFi](const QString &qf) {
                                   return QFileInfo(qf).lastModified() > replyFi.lastModified();
                               });

    if (mustUpdate) {
        qCDebug(cmakeFileApiMode) << "FileApiReader: Starting CMake with no arguments.";
        startCMakeState(QStringList());
        // Keep m_isParsing enabled!
        return;
    }

    endState(replyFi);
}

void FileApiReader::stop()
{
    m_cmakeProcess.reset();
}

bool FileApiReader::isParsing() const
{
    return m_isParsing;
}

QVector<FilePath> FileApiReader::takeProjectFilesToWatch()
{
    return QVector<FilePath>::fromList(Utils::toList(m_cmakeFiles));
}

QList<CMakeBuildTarget> FileApiReader::takeBuildTargets(QString &errorMessage){
    Q_UNUSED(errorMessage)

    auto result = std::move(m_buildTargets);
    m_buildTargets.clear();
    return result;
}

CMakeConfig FileApiReader::takeParsedConfiguration(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    CMakeConfig cache = m_cache;
    m_cache.clear();
    return cache;
}

std::unique_ptr<CMakeProjectNode> FileApiReader::generateProjectTree(
    const QList<const FileNode *> &allFiles, QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    addHeaderNodes(m_rootProjectNode.get(), m_knownHeaders, allFiles);
    return std::move(m_rootProjectNode);
}

RawProjectParts FileApiReader::createRawProjectParts(QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    RawProjectParts result = std::move(m_projectParts);
    m_projectParts.clear();
    return result;
}

void FileApiReader::startState()
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: START STATE.";
    QTC_ASSERT(!m_isParsing, return );
    QTC_ASSERT(!m_future.has_value(), return );

    m_isParsing = true;

    qCDebug(cmakeFileApiMode) << "FileApiReader: CONFIGURATION STARTED SIGNAL";
    emit configurationStarted();
}

void FileApiReader::endState(const QFileInfo &replyFi)
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: END STATE.";
    QTC_ASSERT(m_isParsing, return );
    QTC_ASSERT(!m_future.has_value(), return );

    const FilePath sourceDirectory = m_parameters.sourceDirectory;
    const FilePath buildDirectory = m_parameters.workDirectory;

    m_fileApi->setParsedReplyFilePath(replyFi.filePath());

    m_future = runAsync(ProjectExplorerPlugin::sharedThreadPool(),
                        [replyFi, sourceDirectory, buildDirectory]() {
                            auto result = std::make_unique<FileApiQtcData>();
                            FileApiData data = FileApiParser::parseData(replyFi,
                                                                        result->errorMessage);
                            if (!result->errorMessage.isEmpty()) {
                                qWarning() << result->errorMessage;
                                return result.release();
                            }
                            *result = extractData(data, sourceDirectory, buildDirectory);
                            if (!result->errorMessage.isEmpty()) {
                                qWarning() << result->errorMessage;
                            }

                            return result.release();
                        });
    onFinished(m_future.value(), this, [this](const QFuture<FileApiQtcData *> &f) {
        std::unique_ptr<FileApiQtcData> value(f.result()); // Adopt the pointer again:-)

        m_future = {};
        m_isParsing = false;
        m_cache = std::move(value->cache);
        m_cmakeFiles = std::move(value->cmakeFiles);
        m_buildTargets = std::move(value->buildTargets);
        m_projectParts = std::move(value->projectParts);
        m_rootProjectNode = std::move(value->rootProjectNode);
        m_knownHeaders = std::move(value->knownHeaders);

        if (value->errorMessage.isEmpty()) {
            emit this->dataAvailable();
        } else {
            emit this->errorOccured(value->errorMessage);
        }
    });
}

void FileApiReader::startCMakeState(const QStringList &configurationArguments)
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: START CMAKE STATE.";
    QTC_ASSERT(!m_cmakeProcess, return );

    m_cmakeProcess = std::make_unique<CMakeProcess>();

    connect(m_cmakeProcess.get(), &CMakeProcess::finished, this, &FileApiReader::cmakeFinishedState);

    qCDebug(cmakeFileApiMode) << ">>>>>> Running cmake with arguments:" << configurationArguments;
    m_cmakeProcess->run(m_parameters, configurationArguments);
}

void FileApiReader::cmakeFinishedState(int code, QProcess::ExitStatus status)
{
    qCDebug(cmakeFileApiMode) << "FileApiReader: CMAKE FINISHED STATE.";

    Q_UNUSED(code)
    Q_UNUSED(status)

    m_cmakeProcess.release()->deleteLater();

    endState(m_fileApi->scanForCMakeReplyFile());
}

} // namespace Internal
} // namespace CMakeProjectManager
