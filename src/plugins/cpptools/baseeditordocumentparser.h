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

#include "cpptools_global.h"
#include "cpptools_utils.h"
#include "cppworkingcopy.h"
#include "projectpart.h"

#include <QFutureInterface>
#include <QObject>
#include <QMutex>

namespace ProjectExplorer { class Project; }

namespace CppTools {

class CPPTOOLS_EXPORT BaseEditorDocumentParser : public QObject
{
    Q_OBJECT

public:
    using Ptr = QSharedPointer<BaseEditorDocumentParser>;
    static Ptr get(const QString &filePath);

    struct Configuration {
        bool usePrecompiledHeaders = false;
        QByteArray editorDefines;
        QString preferredProjectPartId;
    };

    struct UpdateParams {
        UpdateParams(const WorkingCopy &workingCopy,
                     const ProjectExplorer::Project *activeProject,
                     Language languagePreference,
                     bool projectsUpdated)
            : workingCopy(workingCopy)
            , activeProject(activeProject)
            , languagePreference(languagePreference)
            , projectsUpdated(projectsUpdated)
        {
        }

        WorkingCopy workingCopy;
        const ProjectExplorer::Project *activeProject = nullptr;
        Language languagePreference = Language::Cxx;
        bool projectsUpdated = false;
    };

public:
    BaseEditorDocumentParser(const QString &filePath);
    virtual ~BaseEditorDocumentParser();

    QString filePath() const;
    Configuration configuration() const;
    void setConfiguration(const Configuration &configuration);

    void update(const UpdateParams &updateParams);
    void update(const QFutureInterface<void> &future, const UpdateParams &updateParams);

    ProjectPartInfo projectPartInfo() const;

signals:
    void projectPartInfoUpdated(const CppTools::ProjectPartInfo &projectPartInfo);

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
            const ProjectExplorer::Project *activeProject,
            Language languagePreference,
            bool projectsUpdated);

    mutable QMutex m_stateAndConfigurationMutex;

private:
    virtual void updateImpl(const QFutureInterface<void> &future,
                            const UpdateParams &updateParams) = 0;

    const QString m_filePath;
    Configuration m_configuration;
    State m_state;
    mutable QMutex m_updateIsRunning;
};

} // namespace CppTools
