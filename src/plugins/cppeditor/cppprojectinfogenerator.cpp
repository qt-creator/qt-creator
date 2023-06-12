// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppprojectinfogenerator.h"

#include "cppeditortr.h"
#include "cppprojectfilecategorizer.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <QPromise>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Internal {

ProjectInfoGenerator::ProjectInfoGenerator(const ProjectUpdateInfo &projectUpdateInfo)
    : m_projectUpdateInfo(projectUpdateInfo)
{
}

ProjectInfo::ConstPtr ProjectInfoGenerator::generate(const QPromise<ProjectInfo::ConstPtr> &promise)
{
    QVector<ProjectPart::ConstPtr> projectParts;
    for (const RawProjectPart &rpp : m_projectUpdateInfo.rawProjectParts) {
        if (promise.isCanceled())
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
        showWarning(
            ::CppEditor::Tr::tr("The project contains C source files, but the currently active kit "
                                "has no C compiler. The code model will not be fully functional."));
    }
    if (m_cxxToolchainMissing) {
        showWarning(::CppEditor::Tr::tr(
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
                               rawProjectPart.fileIsActive,
                               rawProjectPart.getMimeType);

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

    QString explicitTarget;
    if (!tcInfo.targetTripleIsAuthoritative) {
        for (int i = 0; i < flags.commandLineFlags.size(); ++i) {
            const QString &flag = flags.commandLineFlags.at(i);
            if (flag == "-target") {
                if (i + 1 < flags.commandLineFlags.size())
                    explicitTarget = flags.commandLineFlags.at(i + 1);
                break;
            } else if (flag.startsWith("--target=")) {
                explicitTarget = flag.mid(9);
                break;
            }
        }
    }
    if (!explicitTarget.isEmpty()) {
        tcInfo.targetTriple = explicitTarget;
        tcInfo.targetTripleIsAuthoritative = true;
        if (const Abi abi = Abi::fromString(tcInfo.targetTriple); abi.isValid())
            tcInfo.abi = abi;
    }

    return ProjectPart::create(projectFilePath, rawProjectPart, partName, projectFiles,
                               language, languageExtensions, flags, tcInfo);
}

} // namespace CppEditor::Internal
