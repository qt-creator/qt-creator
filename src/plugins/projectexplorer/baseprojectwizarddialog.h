// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>

#include <utils/filepath.h>

#include <memory>

namespace Utils { class ProjectIntroPage; }

namespace ProjectExplorer {

struct BaseProjectWizardDialogPrivate;

// Documentation inside.
class PROJECTEXPLORER_EXPORT BaseProjectWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

protected:
    explicit BaseProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                     Utils::ProjectIntroPage *introPage, int introId,
                                     QWidget *parent, const Core::WizardDialogParameters &parameters);

public:
    explicit BaseProjectWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent,
                                     const Core::WizardDialogParameters &parameters);

    ~BaseProjectWizardDialog() override;

    QString projectName() const;
    Utils::FilePath filePath() const;

    // Generate a new project name (untitled<n>) in path.
    static QString uniqueProjectName(const Utils::FilePath &path);
    void addExtensionPages(const QList<QWizardPage *> &wizardPageList);

    void setIntroDescription(const QString &d);
    void setFilePath(const Utils::FilePath &path);
    void setProjectName(const QString &name);
    void setProjectList(const QStringList &projectList);
    void setProjectDirectories(const Utils::FilePaths &directories);
    void setForceSubProject(bool force);

signals:
    void projectParametersChanged(const QString &projectName, const Utils::FilePath &path);

protected:
    Utils::ProjectIntroPage *introPage() const;
    Utils::Id selectedPlatform() const;
    void setSelectedPlatform(Utils::Id platform);

    QSet<Utils::Id> requiredFeatures() const;
    void setRequiredFeatures(const QSet<Utils::Id> &featureSet);

private:
    void init();
    void slotAccepted();
    bool validateCurrentPage() override;

    std::unique_ptr<BaseProjectWizardDialogPrivate> d;
};

} // namespace ProjectExplorer
