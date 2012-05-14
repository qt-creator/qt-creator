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

#include <QCoreApplication>
#include <QIcon>
#include <QDebug>

namespace Qt4ProjectManager {
namespace Internal {

class QtQuickAppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit QtQuickAppWizardDialog(QWidget *parent, const Core::WizardDialogParameters &parameters,
                                    QtQuickAppWizard::Kind kind);

protected:
    bool validateCurrentPage();

private:
    QtQuickComponentSetOptionsPage *m_componentOptionsPage;
    int m_componentOptionsPageId;

    Utils::WizardProgressItem *m_componentItem;

    friend class QtQuickAppWizard;
};

QtQuickAppWizardDialog::QtQuickAppWizardDialog(QWidget *parent,
                                               const Core::WizardDialogParameters &parameters,
                                               QtQuickAppWizard::Kind kind)
    : AbstractMobileAppWizardDialog(parent,
                                    QtSupport::QtVersionNumber(4, 7, 0),
                                    QtSupport::QtVersionNumber(4, INT_MAX, INT_MAX), parameters)
{
    setWindowTitle(tr("New Qt Quick Application"));
    setIntroDescription(tr("This wizard generates a Qt Quick application project."));

    if (kind == QtQuickAppWizard::ImportQml) { //Choose existing qml file
        m_componentOptionsPage = new Internal::QtQuickComponentSetOptionsPage;
        m_componentOptionsPageId = addPageWithTitle(m_componentOptionsPage, tr("Select existing QML file"));
        m_componentItem = wizardProgress()->item(m_componentOptionsPageId);
    }

    AbstractMobileAppWizardDialog::addMobilePages();

    if (kind == QtQuickAppWizard::ImportQml) {
        if (targetsPageItem())
            m_componentItem->setNextItems(QList<Utils::WizardProgressItem *>()
                                          << targetsPageItem());
    }
}

bool QtQuickAppWizardDialog::validateCurrentPage()
{
    if (currentPage() == m_componentOptionsPage) {
        setIgnoreGenericOptionsPage(false);
    }
    return AbstractMobileAppWizardDialog::validateCurrentPage();
}

class QtQuickAppWizardPrivate
{
    class QtQuickApp *app;
    class QtQuickAppWizardDialog *wizardDialog;
    QtQuickAppWizard::Kind kind;
    friend class QtQuickAppWizard;
};

QtQuickAppWizard::QtQuickAppWizard()
    : AbstractMobileAppWizard(baseParameters())
    , d(new QtQuickAppWizardPrivate)
{
    d->app = new QtQuickApp;
    d->wizardDialog = 0;
}

QtQuickAppWizard::QtQuickAppWizard(const Core::BaseFileWizardParameters &params, QObject *parent)
    : AbstractMobileAppWizard(params, parent)
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

void QtQuickAppWizard::createInstances(ExtensionSystem::IPlugin *plugin)
{
    Core::BaseFileWizardParameters base = baseParameters();
    QList<Core::BaseFileWizardParameters> list;
    Core::BaseFileWizardParameters parameter;

    const QString basicDescription = tr("Creates a Qt Quick application project that can contain "
                                        "both QML and C++ code and includes a QDeclarativeView.\n\n");

    Core::FeatureSet basicFeatures;
    basicFeatures = Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK_1);

    parameter = base;
    parameter.setDisplayName(tr("Qt Quick Application (Built-in Elements)"));
    parameter.setDescription(basicDescription + tr("The built-in elements in the QtQuick namespace allow "
                                                   "you to write cross-platform applications with "
                                                   "a custom look and feel.\n\nRequires <b>Qt 4.7.0</b> or newer."));
    parameter.setRequiredFeatures(basicFeatures);
    list << parameter;

    parameter = base;
    parameter.setDisplayName(tr("Qt Quick Application for Symbian"));
    parameter.setDescription(basicDescription +  tr("The Qt Quick Components for Symbian are a set of "
                                                    "ready-made components that are designed with specific "
                                                    "native appearance for the Symbian platform.\n\nRequires "
                                                    "<b>Qt 4.7.4</b> or newer, and the component set installed for "
                                                    "your Qt version."));
    parameter.setRequiredFeatures(basicFeatures | Core::Feature(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_SYMBIAN)
                                  | QtSupport::Constants::FEATURE_QT_QUICK_1_1);
    list << parameter;

    parameter = base;
    parameter.setDisplayName(tr("Qt Quick Application for MeeGo Harmattan"));
    parameter.setDescription(basicDescription +  tr("The Qt Quick Components for MeeGo Harmattan are "
                                                    "a set of ready-made components that are designed "
                                                    "with specific native appearance for the MeeGo Harmattan "
                                                    "platform.\n\nRequires <b>Qt 4.7.4</b> or newer, and the "
                                                    "component set installed for your Qt version."));
    parameter.setRequiredFeatures(basicFeatures | Core::Feature(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_MEEGO)
                                  | QtSupport::Constants::FEATURE_QT_QUICK_1_1);
    list << parameter;

    parameter = base;
    parameter.setDisplayName(tr("Qt Quick Application (from Existing QML File)"));
    parameter.setDescription(basicDescription +  tr("Creates a deployable Qt Quick application from "
                                                    "existing QML files. All files and directories that "
                                                    "reside in the same directory as the main .qml file "
                                                    "are deployed. You can modify the contents of the "
                                                    "directory any time before deploying.\n\nRequires <b>Qt 4.7.0</b> or newer."));
    parameter.setRequiredFeatures(basicFeatures);
    list << parameter;

    QList<QtQuickAppWizard*> wizardList = Core::createMultipleBaseFileWizardInstances<QtQuickAppWizard>(list, plugin);

    Q_ASSERT(wizardList.count() == 4);

    for (int i = 0; i < wizardList.count(); i++) {
        wizardList.at(i)->setQtQuickKind(Kind(i));
    }
}

Core::BaseFileWizardParameters QtQuickAppWizard::baseParameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Qt4ProjectManager::Constants::ICON_QTQUICK_APP)));
    parameters.setId(QLatin1String("D.QMLA Application"));
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    parameters.setDisplayCategory(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY);

    return parameters;
}

AbstractMobileAppWizardDialog *QtQuickAppWizard::createWizardDialogInternal(QWidget *parent,
                                                                            const Core::WizardDialogParameters &parameters) const
{
    d->wizardDialog = new QtQuickAppWizardDialog(parent, parameters, qtQuickKind());

    switch (qtQuickKind()) {
    case QtQuick1_1:
        d->app->setComponentSet(QtQuickApp::QtQuick10Components);
        d->app->setMainQml(QtQuickApp::ModeGenerate);
        break;
    case SymbianComponents:
        d->app->setComponentSet(QtQuickApp::Symbian11Components);
        d->app->setMainQml(QtQuickApp::ModeGenerate);
        break;
    case MeegoComponents:
        d->app->setComponentSet(QtQuickApp::Meego10Components);
        d->app->setMainQml(QtQuickApp::ModeGenerate);
        break;
    case ImportQml:
        d->app->setComponentSet(QtQuickApp::QtQuick10Components);
        d->app->setMainQml(QtQuickApp::ModeImport);
        break;
    default:
        qWarning() << "QtQuickAppWizard illegal subOption:" << qtQuickKind();
        break;
    }

    return d->wizardDialog;
}

void QtQuickAppWizard::projectPathChanged(const QString &path) const
{
    if (d->wizardDialog->targetsPage())
        d->wizardDialog->targetsPage()->setProFilePath(path);
}

void QtQuickAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QtQuickAppWizardDialog *wizard = qobject_cast<const QtQuickAppWizardDialog*>(w);

    if (d->app->mainQmlMode() == QtQuickApp::ModeGenerate) {
        d->app->setMainQml(QtQuickApp::ModeGenerate);
    } else {
        const QString mainQmlFile = wizard->m_componentOptionsPage->mainQmlFile();
        d->app->setMainQml(QtQuickApp::ModeImport, mainQmlFile);
    }
}

void QtQuickAppWizard::setQtQuickKind(QtQuickAppWizard::Kind kind)
{
    d->kind = kind;
}

QtQuickAppWizard::Kind QtQuickAppWizard::qtQuickKind() const
{
    return d->kind;
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
