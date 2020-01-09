/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

namespace CppTools { class CppProjectUpdater; }

namespace AutotoolsProjectManager {
namespace Internal {

class MakefileParserThread;

class AutotoolsBuildSystem final : public ProjectExplorer::BuildSystem
{
public:
    explicit AutotoolsBuildSystem(ProjectExplorer::Target *target);
    ~AutotoolsBuildSystem() final;

private:
    void triggerParsing() final;

    /**
     * Is invoked when the makefile parsing by m_makefileParserThread has
     * been finished. Adds all sources and files into the project tree and
     * takes care listen to file changes for Makefile.am and configure.ac
     * files.
     */
    void makefileParsingFinished();

    /**
     * This function is in charge of the code completion.
     */
    void updateCppCodeModel();

    /// Return value for AutotoolsProject::files()
    QStringList m_files;

    /// Responsible for parsing the makefiles asynchronously in a thread
    MakefileParserThread *m_makefileParserThread = nullptr;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
};

} // namespace Internal
} // namespace AutotoolsProjectManager
