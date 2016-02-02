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

#include "projectexplorer_export.h"

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>

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
    QString path() const;

    // Generate a new project name (untitled<n>) in path.
    static QString uniqueProjectName(const QString &path);
    void addExtensionPages(const QList<QWizardPage *> &wizardPageList);

    void setIntroDescription(const QString &d);
    void setPath(const QString &path);
    void setProjectName(const QString &name);
    void setProjectList(const QStringList &projectList);
    void setProjectDirectories(const QStringList &directories);
    void setForceSubProject(bool force);

signals:
    void projectParametersChanged(const QString &projectName, const QString &path);

protected:
    Utils::ProjectIntroPage *introPage() const;
    Core::Id selectedPlatform() const;
    void setSelectedPlatform(Core::Id platform);

    QSet<Core::Id> requiredFeatures() const;
    void setRequiredFeatures(const QSet<Core::Id> &featureSet);

private:
    void init();
    void slotAccepted();
    bool validateCurrentPage() override;

    BaseProjectWizardDialogPrivate *d;
};

} // namespace ProjectExplorer
