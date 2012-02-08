/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASEPROJECTWIZARDDIALOG_H
#define BASEPROJECTWIZARDDIALOG_H

#include "projectexplorer_export.h"
#include <coreplugin/featureprovider.h>
#include <coreplugin/basefilewizard.h>
#include <utils/wizard.h>

#include <QtGui/QWizard>

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

public slots:
    void setIntroDescription(const QString &d);
    void setPath(const QString &path);
    void setProjectName(const QString &name);

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
