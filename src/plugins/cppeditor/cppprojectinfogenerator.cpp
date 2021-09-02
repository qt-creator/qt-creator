/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppprojectinfogenerator.h"

#include "cppprojectfilecategorizer.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/qtcassert.h>

#include <QTimer>

#include <set>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Internal {

ProjectInfoGenerator::ProjectInfoGenerator(
        const QFutureInterface<ProjectInfo::ConstPtr> &futureInterface,
        const ProjectUpdateInfo &projectUpdateInfo)
    : m_futureInterface(futureInterface)
    , m_projectUpdateInfo(projectUpdateInfo)
{
}

ProjectInfo::ConstPtr ProjectInfoGenerator::generate()
{
    QVector<ProjectPart::ConstPtr> projectParts;
    for (const RawProjectPart &rpp : m_projectUpdateInfo.rawProjectParts) {
        if (m_futureInterface.isCanceled())
            return {};
        for (const ProjectPart::ConstPtr &part : createProjectParts(
                 rpp, m_projectUpdateInfo.projectFilePath)) {
            projectParts << part;
        }
    }
    const auto projectInfo = ProjectInfo::create(m_projectUpdateInfo, projectParts);

    static const auto showWarning = [](const QString &message) {
        QTimer::singleShot(0, TaskHub::instance(), [message] {
            TaskHub::addTask(BuildSystemTask(Task::Warning, message));
        });
    };
    if (m_cToolchainMissing) {
        showWarning(QCoreApplication::translate("CppEditor",
                "The project contains C source files, but the currently active kit "
                "has no C compiler. The code model will not be fully functional."));
    }
    if (m_cxxToolchainMissing) {
        showWarning(QCoreApplication::translate("CppEditor",
                "The project contains C++ source files, but the currently active kit "
                "has no C++ compiler. The code model will not be fully functional."));
    }
    return projectInfo;
}

const QVector<ProjectPart::ConstPtr> ProjectInfoGenerator::createProjectParts(
    const RawProjectPart &rawProjectPart, const FilePath &projectFilePath)
{
    QVector<ProjectPart::ConstPtr> result;
    ProjectFileCategorizer cat(rawProjectPart.displayName,
                               rawProjectPart.files,
                               rawProjectPart.fileIsActive);

    if (!cat.hasParts())
        return result;

    if (m_projectUpdateInfo.cxxToolChainInfo.isValid()) {
        if (cat.hasCxxSources()) {
            result << createProjectPart(projectFilePath,
                                        rawProjectPart,
                                        cat.cxxSources(),
                                        cat.partName("C++"),
                                        Language::Cxx,
                                        LanguageExtension::None);
        }
        if (cat.hasObjcxxSources()) {
            result << createProjectPart(projectFilePath,
                                        rawProjectPart,
                                        cat.objcxxSources(),
                                        cat.partName("Obj-C++"),
                                        Language::Cxx,
                                        LanguageExtension::ObjectiveC);
        }
    } else if (cat.hasCxxSources() || cat.hasObjcxxSources()) {
        m_cxxToolchainMissing = true;
    }

    if (m_projectUpdateInfo.cToolChainInfo.isValid()) {
        if (cat.hasCSources()) {
            result << createProjectPart(projectFilePath,
                                        rawProjectPart,
                                        cat.cSources(),
                                        cat.partName("C"),
                                        Language::C,
                                        LanguageExtension::None);
        }

        if (cat.hasObjcSources()) {
            result << createProjectPart(projectFilePath,
                                        rawProjectPart,
                                        cat.objcSources(),
                                        cat.partName("Obj-C"),
                                        Language::C,
                                        LanguageExtension::ObjectiveC);
        }
    } else if (cat.hasCSources() || cat.hasObjcSources()) {
        m_cToolchainMissing = true;
    }

    return result;
}

ProjectPart::ConstPtr ProjectInfoGenerator::createProjectPart(
        const FilePath &projectFilePath,
        const RawProjectPart &rawProjectPart,
        const ProjectFiles &projectFiles,
        const QString &partName,
        Language language,
        LanguageExtensions languageExtensions)
{
    RawProjectPartFlags flags;
    ToolChainInfo tcInfo;
    if (language == Language::C) {
        flags = rawProjectPart.flagsForC;
        tcInfo = m_projectUpdateInfo.cToolChainInfo;
    }
    // Use Cxx toolchain for C projects without C compiler in kit and for C++ code
    if (!tcInfo.isValid()) {
        flags = rawProjectPart.flagsForCxx;
        tcInfo = m_projectUpdateInfo.cxxToolChainInfo;
    }

    return ProjectPart::create(projectFilePath, rawProjectPart, partName, projectFiles,
                               language, languageExtensions, flags, tcInfo);
}

} // namespace CppEditor::Internal
