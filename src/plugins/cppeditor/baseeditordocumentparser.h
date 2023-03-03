// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cpptoolsreuse.h"
#include "cppworkingcopy.h"

#include <projectexplorer/project.h>

#include <QObject>
#include <QMutex>

QT_BEGIN_NAMESPACE
template <typename T>
class QPromise;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace CppEditor {

class CPPEDITOR_EXPORT BaseEditorDocumentParser : public QObject
{
    Q_OBJECT

public:
    using Ptr = QSharedPointer<BaseEditorDocumentParser>;
    static Ptr get(const Utils::FilePath &filePath);

    struct Configuration {
        bool usePrecompiledHeaders = false;
        QByteArray editorDefines;
        QString preferredProjectPartId;

        bool operator==(const Configuration &other)
        {
            return usePrecompiledHeaders == other.usePrecompiledHeaders
                    && editorDefines == other.editorDefines
                    && preferredProjectPartId == other.preferredProjectPartId;
        }
    };

    struct UpdateParams {
        UpdateParams(const WorkingCopy &workingCopy,
                     const ProjectExplorer::Project *activeProject,
                     Utils::Language languagePreference,
                     bool projectsUpdated)
            : workingCopy(workingCopy)
            , activeProject(activeProject ? activeProject->projectFilePath() : Utils::FilePath())
            , languagePreference(languagePreference)
            , projectsUpdated(projectsUpdated)
        {
        }

        WorkingCopy workingCopy;
        const Utils::FilePath activeProject;
        Utils::Language languagePreference = Utils::Language::Cxx;
        bool projectsUpdated = false;
    };

public:
    BaseEditorDocumentParser(const Utils::FilePath &filePath);
    ~BaseEditorDocumentParser() override;

    const Utils::FilePath &filePath() const;
    Configuration configuration() const;
    void setConfiguration(const Configuration &configuration);

    void update(const UpdateParams &updateParams);
    void update(const QPromise<void> &promise, const UpdateParams &updateParams);

    ProjectPartInfo projectPartInfo() const;

signals:
    void projectPartInfoUpdated(const ProjectPartInfo &projectPartInfo);

protected:
    struct State {
        QByteArray editorDefines;
        ProjectPartInfo projectPartInfo;
    };
    State state() const;
    void setState(const State &state);

    static ProjectPartInfo determineProjectPart(const QString &filePath,
            const QString &preferredProjectPartId,
            const ProjectPartInfo &currentProjectPartInfo,
            const Utils::FilePath &activeProject,
            Utils::Language languagePreference,
            bool projectsUpdated);

    mutable QMutex m_stateAndConfigurationMutex;

private:
    virtual void updateImpl(const QPromise<void> &promise,
                            const UpdateParams &updateParams) = 0;

    const Utils::FilePath m_filePath;
    Configuration m_configuration;
    State m_state;
    mutable QMutex m_updateIsRunning;
};

} // CppEditor
