/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASEPROJECTWIZARDDIALOG_H
#define BASEPROJECTWIZARDDIALOG_H

#include "projectexplorer_export.h"
#include <coreplugin/featureprovider.h>
#include <coreplugin/basefilewizard.h>
#include <utils/wizard.h>

#include <QWizard>

namespace Utils {
    class ProjectIntroPage;
}

namespace ProjectExplorer {

struct BaseProjectWizardDialogPrivate;

// Documentation inside.
class PROJECTEXPLORER_EXPORT BaseProjectWizardDialog : public Utils::Wizard
{
    Q_OBJECT

protected:
    explicit BaseProjectWizardDialog(Utils::ProjectIntroPage *introPage,
                                     int introId,
                                     QWidget *parent, const Core::WizardDialogParameters &parameters);

public:
    explicit BaseProjectWizardDialog(QWidget *parent, const Core::WizardDialogParameters &parameters);

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
