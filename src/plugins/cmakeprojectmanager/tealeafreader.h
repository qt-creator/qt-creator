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

#include "builddirreader.h"

namespace Utils { class QtcProcess; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeFile;

class TeaLeafReader : public BuildDirReader
{
    Q_OBJECT

public:
    TeaLeafReader();
    ~TeaLeafReader() final;

    bool isCompatible(const Parameters &p) final;
    void resetData() final;
    void parse(bool force) final;
    void stop() final;

    bool isParsing() const final;
    bool hasData() const final;

    QList<CMakeBuildTarget> buildTargets() const final;
    CMakeConfig parsedConfiguration() const final;
    void generateProjectTree(CMakeProjectNode *root) final;
    QSet<Core::Id> updateCodeModel(CppTools::ProjectPartBuilder &ppBuilder) final;

private:
    void cleanUpProcess();
    void extractData();

    void startCMake(const QStringList &configurationArguments);

    void cmakeFinished(int code, QProcess::ExitStatus status);
    void processCMakeOutput();
    void processCMakeError();

    QStringList getCXXFlagsFor(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache);
    bool extractCXXFlagsFromMake(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache);
    bool extractCXXFlagsFromNinja(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache);

    CMakeConfig parseConfiguration(const Utils::FileName &cacheFile, QString *errorMessage) const;

    Utils::QtcProcess *m_cmakeProcess = nullptr;

    // For error reporting:
    ProjectExplorer::IOutputParser *m_parser = nullptr;
    QFutureInterface<void> *m_future = nullptr;

    bool m_hasData = false;

    mutable CMakeConfig m_cmakeCache;
    QSet<Utils::FileName> m_cmakeFiles;
    QString m_projectName;
    QList<CMakeBuildTarget> m_buildTargets;
    QList<ProjectExplorer::FileNode *> m_files;
    QSet<Internal::CMakeFile *> m_watchedFiles;

    friend class CMakeFile;
};

} // namespace Internal
} // namespace CMakeProjectManager
