/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "mobileappwizard.h"

#include "mobileappwizardpages.h"
#include "mobileapp.h"
#include "targetsetuppage.h"

#include "qt4projectmanagerconstants.h"

#include <projectexplorer/task.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QIcon>

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
    explicit MobileAppWizardDialog(QWidget *parent = 0)
        : AbstractMobileAppWizardDialog(parent)
    {
        setWindowTitle(DisplayName);
        setIntroDescription(Description);
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
    , m_d(new MobileAppWizardPrivate)
{
    m_d->mobileApp = new MobileApp;
    m_d->wizardDialog = 0;
}

MobileAppWizard::~MobileAppWizard()
{
    delete m_d->mobileApp;
    delete m_d;
}

Core::BaseFileWizardParameters MobileAppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_QT_PROJECT)));
    parameters.setDisplayName(DisplayName);
    parameters.setId(QLatin1String("C.Qt4GuiMobile"));
    parameters.setDescription(Description);
    parameters.setCategory(QLatin1String(Constants::QT_APP_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::QT_APP_WIZARD_TR_SCOPE,
                                                              Constants::QT_APP_WIZARD_TR_CATEGORY));
    return parameters;
}

AbstractMobileAppWizardDialog *MobileAppWizard::createWizardDialogInternal(QWidget *parent) const
{
    m_d->wizardDialog = new MobileAppWizardDialog(parent);
    m_d->wizardDialog->targetsPage()->setPreferMobile(true);
    return m_d->wizardDialog;
}

void MobileAppWizard::projectPathChanged(const QString &path) const
{
    m_d->wizardDialog->targetsPage()->setProFilePath(path);
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
    return m_d->mobileApp;
}

AbstractMobileAppWizardDialog *MobileAppWizard::wizardDialog() const
{
    return m_d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "mobileappwizard.moc"
