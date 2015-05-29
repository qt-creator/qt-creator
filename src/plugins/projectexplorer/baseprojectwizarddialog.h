/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASEPROJECTWIZARDDIALOG_H
#define BASEPROJECTWIZARDDIALOG_H

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

    virtual ~BaseProjectWizardDialog();

    QString projectName() const;
    QString path() const;

    // Generate a new project name (untitled<n>) in path.
    static QString uniqueProjectName(const QString &path);
    void addExtensionPages(const QList<QWizardPage *> &wizardPageList);

public slots:
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
    QString selectedPlatform() const;
    void setSelectedPlatform(const QString &platform);

    Core::FeatureSet requiredFeatures() const;
    void setRequiredFeatures(const Core::FeatureSet &featureSet);

private slots:
    void slotAccepted();
    void nextClicked();

private:
    void init();

    BaseProjectWizardDialogPrivate *d;
};

} // namespace ProjectExplorer

#endif // BASEPROJECTWIZARDDIALOG_H
