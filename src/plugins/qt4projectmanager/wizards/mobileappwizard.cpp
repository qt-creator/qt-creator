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

#include "mobileappwizard.h"

#include "mobileappwizardpages.h"
#include "mobileapp.h"
#include "targetsetuppage.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>
#include <QIcon>

namespace Qt4ProjectManager {
namespace Internal {
namespace {
const QString DisplayName
    = QCoreApplication::translate("MobileAppWizard", "Mobile Qt Application");
const QString Description
    = QCoreApplication::translate("MobileAppWizard",
        "Creates a Qt application optimized for mobile devices "
        "with a Qt Designer-based main window.\n\n"
        "Preselects Qt for Simulator and mobile targets if available.");
}

class MobileAppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT
public:
    explicit MobileAppWizardDialog(QWidget *parent, const Core::WizardDialogParameters &parameters)
        : AbstractMobileAppWizardDialog(parent,
                                        QtSupport::QtVersionNumber(),
                                        QtSupport::QtVersionNumber(4, INT_MAX, INT_MAX), parameters)
    {
        setWindowTitle(DisplayName);
        setIntroDescription(Description);
        addMobilePages();
    }
};

class MobileAppWizardPrivate
{
    class MobileApp *mobileApp;
    class MobileAppWizardDialog *wizardDialog;
    friend class MobileAppWizard;
};

MobileAppWizard::MobileAppWizard()
    : AbstractMobileAppWizard(parameters())
    , d(new MobileAppWizardPrivate)
{
    d->mobileApp = new MobileApp;
    d->wizardDialog = 0;
}

MobileAppWizard::~MobileAppWizard()
{
    delete d->mobileApp;
    delete d;
}

Core::FeatureSet MobileAppWizard::requiredFeatures() const
{
    return Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE)
            | Core::Feature(QtSupport::Constants::FEATURE_QWIDGETS);
}

Core::BaseFileWizardParameters MobileAppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_QT_PROJECT)));
    parameters.setDisplayName(DisplayName);
    parameters.setId(QLatin1String("C.Qt4GuiMobile"));
    parameters.setDescription(Description);
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY));
    return parameters;
}

AbstractMobileAppWizardDialog *MobileAppWizard::createWizardDialogInternal(QWidget *parent,
                                                                           const Core::WizardDialogParameters &parameters) const
{
    d->wizardDialog = new MobileAppWizardDialog(parent, parameters);
    return d->wizardDialog;
}

void MobileAppWizard::projectPathChanged(const QString &path) const
{
    if (d->wizardDialog->targetsPage())
        d->wizardDialog->targetsPage()->setProFilePath(path);
}

void MobileAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(w);
    Q_UNUSED(errorMessage)
}

QString MobileAppWizard::fileToOpenPostGeneration() const
{
    return QString();
}

AbstractMobileApp *MobileAppWizard::app() const
{
    return d->mobileApp;
}

AbstractMobileAppWizardDialog *MobileAppWizard::wizardDialog() const
{
    return d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "mobileappwizard.moc"
