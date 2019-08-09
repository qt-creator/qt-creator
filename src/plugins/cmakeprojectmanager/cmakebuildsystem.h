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

#include <projectexplorer/buildsystem.h>

namespace CppTools {
class CppProjectUpdater;
} // namespace CppTools

namespace CMakeProjectManager {

class CMakeProject;

namespace Internal {
class CMakeBuildConfiguration;
} // namespace Internal

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

class CMakeBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit CMakeBuildSystem(CMakeProject *project);
    ~CMakeBuildSystem() final;

protected:
    bool validateParsingContext(const ParsingContext &ctx) final;
    void parseProject(ParsingContext &&ctx) final;

private:
    // Treescanner states:
    void handleTreeScanningFinished();

    // Parser states:
    void handleParsingSuccess(Internal::CMakeBuildConfiguration *bc);
    void handleParsingError(Internal::CMakeBuildConfiguration *bc);

    // Combining Treescanner and Parser states:
    void combineScanAndParse();

    void updateProjectData(CMakeProject *p, Internal::CMakeBuildConfiguration *bc);
    QList<ProjectExplorer::ExtraCompiler *> findExtraCompilers(CMakeProject *p);
    void updateQmlJSCodeModel(CMakeProject *p, Internal::CMakeBuildConfiguration *bc);

    ProjectExplorer::TreeScanner m_treeScanner;
    QHash<QString, bool> m_mimeBinaryCache;
    QList<const ProjectExplorer::FileNode *> m_allFiles;

    bool m_waitingForScan = false;
    bool m_waitingForParse = false;
    bool m_combinedScanAndParseResult = false;

    ParsingContext m_currentContext;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    friend class Internal::CMakeBuildConfiguration; // For handleParsing* callbacks
};

} // namespace CMakeProjectManager
