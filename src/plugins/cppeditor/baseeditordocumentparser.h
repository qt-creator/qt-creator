// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cpptoolsreuse.h"
#include "cppworkingcopy.h"
#include "projectpart.h"

#include <projectexplorer/project.h>
#include <utils/cpplanguage_details.h>

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

    class Configuration {
    public:
        QString effectivePreferredProjectPartId() const
        {
            if (!m_preferredProjectPartId.isEmpty())
                return m_preferredProjectPartId;
            return m_softPreferredProjectPartId;
        }

        // "Preferred" means interactively set by the user.
        bool setPreferredProjectPartId(const QString &id)
        {
            return set(m_preferredProjectPartId, id);
        }

        // "Soft preferred" means deduced from the current node in the project tree.
        bool setSoftPreferredProjectPartId(const QString &id)
        {
            return set(m_softPreferredProjectPartId, id);
        }

        bool usePrecompiledHeaders() const  { return m_usePrecompiledHeaders; }
        void setUsePrecompiledHeaders(bool use) { m_usePrecompiledHeaders = use; }

        const QByteArray &editorDefines() const { return m_editorDefines; }
        bool setEditorDefines(const QByteArray &defines) { return set(m_editorDefines, defines); }

    private:
        friend bool operator==(const Configuration &left, const Configuration &right)
        {
            return left.m_usePrecompiledHeaders == right.m_usePrecompiledHeaders
                   && left.m_editorDefines == right.m_editorDefines
                   && left.m_softPreferredProjectPartId == right.m_softPreferredProjectPartId
                   && left.m_preferredProjectPartId == right.m_preferredProjectPartId;
        }

        template<typename T> bool set(T &tgt, const T &src)
        {
            if (tgt == src)
                return false;
            tgt = src;
            return true;
        }

        bool m_usePrecompiledHeaders = false;
        QByteArray m_editorDefines;
        QString m_preferredProjectPartId;
        QString m_softPreferredProjectPartId;
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

    static ProjectPartInfo determineProjectPart(const Utils::FilePath &filePath,
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
