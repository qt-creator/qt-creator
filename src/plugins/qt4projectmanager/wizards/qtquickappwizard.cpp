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

#include "mobileappwizardpages.h"

#include "qtquickapp.h"
#include "qtquickappwizard.h"
#include "qtquickappwizardpages.h"
#include "targetsetuppage.h"
#include "qt4projectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/dialogs/iwizard.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

class QtQuickAppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit QtQuickAppWizardDialog(QWidget *parent, const Core::WizardDialogParameters &parameters);

protected:
    bool validateCurrentPage();

private:
    class QtQuickComponentSetOptionsPage *m_componentOptionsPage;
    int m_componentOptionsPageId;

    Utils::WizardProgressItem *m_componentItem;

    friend class QtQuickAppWizard;
};

QtQuickAppWizardDialog::QtQuickAppWizardDialog(QWidget *parent,
                                               const Core::WizardDialogParameters &parameters)
    : AbstractMobileAppWizardDialog(parent,
                                    QtSupport::QtVersionNumber(4, 7, 0),
                                    QtSupport::QtVersionNumber(4, INT_MAX, INT_MAX), parameters)
{
    setWindowTitle(tr("New Qt Quick Application"));
    setIntroDescription(tr("This wizard generates a Qt Quick application project."));

    m_componentOptionsPage = new Internal::QtQuickComponentSetOptionsPage;
    m_componentOptionsPageId = addPageWithTitle(m_componentOptionsPage, tr("Application Type"));
    m_componentItem = wizardProgress()->item(m_componentOptionsPageId);

    AbstractMobileAppWizardDialog::addMobilePages();

    m_componentItem->setNextItems(QList<Utils::WizardProgressItem *>()
                                  << targetsPageItem());
}

bool QtQuickAppWizardDialog::validateCurrentPage()
{
    if (currentPage() == m_componentOptionsPage) {
        setIgnoreGenericOptionsPage(false);
        if (m_componentOptionsPage->componentSet() == QtQuickApp::Symbian11Components) {
            setIgnoreGenericOptionsPage(true);
            targetsPage()->setRequiredQtFeatures(Core::FeatureSet(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_MEEGO));
        } else if (m_componentOptionsPage->componentSet() == QtQuickApp::Meego10Components) {
            targetsPage()->setRequiredQtFeatures(Core::FeatureSet(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_MEEGO));
        } else {
            targetsPage()->setMinimumQtVersion(QtSupport::QtVersionNumber(4, 7, 0));
            targetsPage()->setRequiredQtFeatures(Core::FeatureSet());
        }
    }
    return AbstractMobileAppWizardDialog::validateCurrentPage();
}

class QtQuickAppWizardPrivate
{
    class QtQuickApp *app;
    class QtQuickAppWizardDialog *wizardDialog;
    friend class QtQuickAppWizard;
};

QtQuickAppWizard::QtQuickAppWizard()
    : AbstractMobileAppWizard(parameters())
    , d(new QtQuickAppWizardPrivate)
{
    d->app = new QtQuickApp;
    d->wizardDialog = 0;
}

QtQuickAppWizard::~QtQuickAppWizard()
{
    delete d->app;
    delete d;
}

Core::FeatureSet QtQuickAppWizard::requiredFeatures() const
{
    return Core::Feature(QtSupport::Constants::FEATURE_GENERIC_CPP_ENTRY_POINT) |
            Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK);

}

Core::BaseFileWizardParameters QtQuickAppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Qt4ProjectManager::Constants::ICON_QTQUICK_APP)));
    parameters.setDisplayName(tr("Qt Quick Application"));
    parameters.setId(QLatin1String("QA.QMLA Application"));
    parameters.setDescription(tr("Creates a Qt Quick application project that can contain "
                                 "both QML and C++ code and includes a QDeclarativeView.\n\n"
                                 "You can build the application and deploy it on desktop and "
                                 "mobile target platforms. For example, you can create signed "
                                 "Symbian Installation System (SIS) packages for this type of "
                                 "projects. Moreover, you can select to use a set of premade "
                                 "UI components in your Qt Quick application. "
                                 "To utilize the components, Qt 4.7.4 or newer is required."));
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    parameters.setDisplayCategory(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY);
    return parameters;
}

AbstractMobileAppWizardDialog *QtQuickAppWizard::createWizardDialogInternal(QWidget *parent,
                                                                            const Core::WizardDialogParameters &parameters) const
{
    d->wizardDialog = new QtQuickAppWizardDialog(parent, parameters);
    d->wizardDialog->m_componentOptionsPage->setComponentSet(d->app->componentSet());
    return d->wizardDialog;
}

void QtQuickAppWizard::projectPathChanged(const QString &path) const
{
    d->wizardDialog->targetsPage()->setProFilePath(path);
}

void QtQuickAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QtQuickAppWizardDialog *wizard = qobject_cast<const QtQuickAppWizardDialog*>(w);
    if (wizard->m_componentOptionsPage->mainQmlMode() == QtQuickApp::ModeGenerate) {
        d->app->setMainQml(QtQuickApp::ModeGenerate);
    } else {
        const QString mainQmlFile = wizard->m_componentOptionsPage->mainQmlFile();
        d->app->setMainQml(QtQuickApp::ModeImport, mainQmlFile);
    }
    d->app->setComponentSet(wizard->m_componentOptionsPage->componentSet());
    if (d->app->componentSet() == QtQuickApp::Symbian11Components)
        d->app->setOrientation(AbstractMobileApp::ScreenOrientationImplicit);
}

QString QtQuickAppWizard::fileToOpenPostGeneration() const
{
    return d->app->path(QtQuickApp::MainQml);
}

AbstractMobileApp *QtQuickAppWizard::app() const
{
    return d->app;
}

AbstractMobileAppWizardDialog *QtQuickAppWizard::wizardDialog() const
{
    return d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "qtquickappwizard.moc"
