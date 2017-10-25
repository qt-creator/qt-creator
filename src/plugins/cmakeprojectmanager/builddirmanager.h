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
#include "cmakebuildtarget.h"
#include "cmakeconfigitem.h"

#include <cpptools/cpprawprojectpart.h>

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <QObject>
#include <QTimer>

#include <functional>
#include <memory>
#include <unordered_map>

namespace ProjectExplorer { class FileNode; }

namespace CMakeProjectManager {

class CMakeTool;

namespace Internal {

class CMakeProjectNode;
class CMakeBuildConfiguration;

class BuildDirManager : public QObject
{
    Q_OBJECT

public:
    BuildDirManager();
    ~BuildDirManager() final;

    bool isParsing() const;

    void setParametersAndRequestParse(const BuildDirParameters &parameters,
                                      int newReaderReparseOptions, int existingReaderReparseOptions);
    CMakeBuildConfiguration *buildConfiguration() const;

    void clearCache();

    void resetData();
    bool persistCMakeState();

    void parse(int reparseParameters);

    void generateProjectTree(CMakeProjectNode *root,
                             const QList<const ProjectExplorer::FileNode *> &allFiles) const;
    void updateCodeModel(CppTools::RawProjectParts &rpps);

    QList<CMakeBuildTarget> takeBuildTargets() const;
    CMakeConfig takeCMakeConfiguration() const;

    static CMakeConfig parseCMakeConfiguration(const Utils::FileName &cacheFile,
                                              QString *errorMessage);

    enum ReparseParameters { REPARSE_DEFAULT = 0, // use defaults
                             REPARSE_URGENT = 1, // Do not wait for more requests, start ASAP
                             REPARSE_FORCE_CONFIGURATION = 2, // Force configuration arguments to cmake
                             REPARSE_CHECK_CONFIGURATION = 4, // Check and warn if on-disk config and QtC config differ
                             REPARSE_IGNORE = 8, // Do not reparse:-)
                             REPARSE_FAIL = 16 // Do not reparse and raise a warning
                           };

signals:
    void requestReparse(int reparseParameters) const;
    void parsingStarted() const;
    void dataAvailable() const;
    void errorOccured(const QString &err) const;

private:
    void emitDataAvailable();
    void emitErrorOccured(const QString &message) const;
    bool checkConfiguration();

    Utils::FileName workDirectory(const BuildDirParameters &parameters) const;

    void updateReaderType(const BuildDirParameters &p, std::function<void()> todo);

    bool hasConfigChanged();


    void becameDirty();

    BuildDirParameters m_parameters;
    mutable std::unordered_map<Utils::FileName, std::unique_ptr<Utils::TemporaryDirectory>> m_buildDirToTempDir;
    mutable std::unique_ptr<BuildDirReader> m_reader;
    mutable bool m_isHandlingError = false;
};

} // namespace Internal
} // namespace CMakeProjectManager
