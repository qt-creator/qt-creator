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

#pragma once

#include "builddirparameters.h"
#include "builddirreader.h"
#include "cmakebuildsystem.h"
#include "cmakebuildtarget.h"
#include "cmakeconfigitem.h"

#include <projectexplorer/rawprojectpart.h>

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <QObject>
#include <QTimer>

#include <functional>
#include <memory>
#include <unordered_map>

namespace ProjectExplorer { class FileNode; }

namespace CMakeProjectManager {

class CMakeProject;
class CMakeTool;

namespace Internal {

class CMakeProjectNode;
class CMakeBuildConfiguration;

class BuildDirManager : public QObject
{
    Q_OBJECT

public:
    enum ReparseParameters {
        REPARSE_DEFAULT = 0,                    // Nothing special:-)
        REPARSE_FORCE_CMAKE_RUN = (1 << 0),     // Force cmake to run
        REPARSE_FORCE_CONFIGURATION = (1 << 1), // Force configuration arguments to cmake
        REPARSE_CHECK_CONFIGURATION = (1 << 2), // Check for on-disk config and QtC config diff
        REPARSE_SCAN = (1 << 3),                // Run filesystem scan
        REPARSE_URGENT = (1 << 4),              // Do not delay the parser run by 1s
    };

    static QString flagsString(int reparseFlags);

    BuildDirManager(CMakeProject *project);
    ~BuildDirManager() final;

    bool isParsing() const;

    void stopParsingAndClearState();

    void setParametersAndRequestParse(const BuildDirParameters &parameters,
                                      const int reparseOptions);
    // nullptr if the BC is not active anymore!
    CMakeBuildConfiguration *buildConfiguration() const;
    CMakeProject *project() const {return m_project; }
    Utils::FilePath buildDirectory() const;

    void clearCache();

    void resetData();
    bool persistCMakeState();

    void requestFilesystemScan();
    bool isFilesystemScanRequested() const;
    void parse();

    QVector<Utils::FilePath> takeProjectFilesToWatch();
    std::unique_ptr<CMakeProjectNode> generateProjectTree(const QList<const ProjectExplorer::FileNode *> &allFiles,
                             QString &errorMessage) const;
    ProjectExplorer::RawProjectParts createRawProjectParts(QString &errorMessage) const;

    QList<CMakeBuildTarget> takeBuildTargets(QString &errorMessage) const;
    CMakeConfig takeCMakeConfiguration(QString &errorMessage) const;

    static CMakeConfig parseCMakeConfiguration(const Utils::FilePath &cacheFile,
                                               QString *errorMessage);

signals:
    void requestReparse() const;
    void requestDelayedReparse() const;
    void parsingStarted() const;
    void dataAvailable() const;
    void errorOccured(const QString &err) const;

private:
    void updateReparseParameters(const int parameters);
    int takeReparseParameters();

    void emitDataAvailable();
    void emitErrorOccured(const QString &message) const;
    void emitReparseRequest() const;
    bool checkConfiguration();

    Utils::FilePath workDirectory(const BuildDirParameters &parameters) const;

    void updateReaderType(const BuildDirParameters &p, std::function<void()> todo);

    bool hasConfigChanged();

    void writeConfigurationIntoBuildDirectory(const Utils::MacroExpander *expander);

    void becameDirty();

    BuildDirParameters m_parameters;
    int m_reparseParameters;
    CMakeProject *m_project = nullptr;
    mutable std::unordered_map<Utils::FilePath, std::unique_ptr<Utils::TemporaryDirectory>> m_buildDirToTempDir;
    mutable std::unique_ptr<BuildDirReader> m_reader;
    mutable bool m_isHandlingError = false;
};

} // namespace Internal
} // namespace CMakeProjectManager
