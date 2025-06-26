// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QFuture>

namespace ProjectExplorer {
class BuildConfiguration;
class QmlCodeModelInfo;
class Project;
}

namespace QmlJSTools::Internal {

class QMLJSTOOLS_EXPORT ModelManager: public QmlJS::ModelManagerInterface
{
    Q_OBJECT

public:
    ModelManager();
    ~ModelManager() override;

    void delayedInitialization();

private:
    QHash<QString, QmlJS::Dialect> languageForSuffix() const override;
    void writeMessageInternal(const QString &msg) const override;
    WorkingCopy workingCopyInternal() const override;
    void addTaskInternal(const QFuture<void> &result, const QString &msg,
                         const Utils::Id taskId) const override;

    void updateDefaultProjectInfo();

    void updateFromBuildConfig(ProjectExplorer::BuildConfiguration *bc,
                               const ProjectExplorer::QmlCodeModelInfo &extra);

    void loadDefaultQmlTypeDescriptions();
    QHash<QString, QmlJS::Dialect> initLanguageForSuffix() const;
};

QMLJSTOOLS_EXPORT ProjectExplorer::Project *
    projectFromProjectInfo(const ModelManager::ProjectInfo &projectInfo);

} // namespace QmlJSTools::Internal
