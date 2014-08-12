/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qtquickappwizard.h"

#include "qtquickapp.h"
#include "qtquickappwizardpages.h"
#include "../qmakeprojectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/baseqtversion.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/targetsetuppage.h>

#include <QIcon>

namespace QmakeProjectManager {
namespace Internal {

class QtQuickAppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit QtQuickAppWizardDialog(QWidget *parent, const Core::WizardDialogParameters &parameters);
    TemplateInfo templateInfo() const;

protected:
    void initializePage(int id);

private:
    QtQuickComponentSetPage *m_componentSetPage;
};

QtQuickAppWizardDialog::QtQuickAppWizardDialog(QWidget *parent,
                                               const Core::WizardDialogParameters &parameters)
    : AbstractMobileAppWizardDialog(parent,
                                    QtSupport::QtVersionNumber(4, 7, 0),
                                    QtSupport::QtVersionNumber(5, INT_MAX, INT_MAX), parameters)
{
    setWindowTitle(tr("New Qt Quick Application"));
    setIntroDescription(tr("This wizard generates a Qt Quick Application project."));

    m_componentSetPage = new Internal::QtQuickComponentSetPage;
    addPage(m_componentSetPage);

    addKitsPage();

    setRequiredFeatures(Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK));
}

void QtQuickAppWizardDialog::initializePage(int id)
{
    if (page(id) == kitsPage()) {
        QStringList stringList =
                templateInfo().featuresRequired.split(QLatin1Char(','), QString::SkipEmptyParts);
        Core::FeatureSet features;
        foreach (const QString &string, stringList) {
            Core::Feature feature(Core::Id::fromString(string.trimmed()));
            features |= feature;
        }

        setRequiredFeatures(features);
        updateKitsPage();
    }
    AbstractMobileAppWizardDialog::initializePage(id);
}

TemplateInfo QtQuickAppWizardDialog::templateInfo() const
{
    return m_componentSetPage->templateInfo();
}


class QtQuickAppWizardPrivate
{
    class QtQuickApp *app;
    class QtQuickAppWizardDialog *wizardDialog;
    friend class QtQuickAppWizard;
};

QtQuickAppWizard::QtQuickAppWizard()
    : d(new QtQuickAppWizardPrivate)
{
    setWizardKind(ProjectWizard);
    setIcon(QIcon(QLatin1String(QmakeProjectManager::Constants::ICON_QTQUICK_APP)));
    setId(QLatin1String("D.QMLA Application"));
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(tr("Qt Quick Application"));
    setDescription(tr("Creates a Qt Quick application project that can contain both QML and C++ code."));
    setRequiredFeatures(Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK));

    d->app = new QtQuickApp;
    d->wizardDialog = 0;
}

QtQuickAppWizard::~QtQuickAppWizard()
{
    delete d->app;
    delete d;
}

AbstractMobileAppWizardDialog *QtQuickAppWizard::createInternal(QWidget *parent,
                                                                const Core::WizardDialogParameters &parameters) const
{
    d->wizardDialog = new QtQuickAppWizardDialog(parent, parameters);
    return d->wizardDialog;
}

void QtQuickAppWizard::projectPathChanged(const QString &path) const
{
    if (d->wizardDialog->kitsPage())
        d->wizardDialog->kitsPage()->setProjectPath(path);
}

void QtQuickAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QtQuickAppWizardDialog *wizard = qobject_cast<const QtQuickAppWizardDialog*>(w);
    d->app->setTemplateInfo(wizard->templateInfo());
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
} // namespace QmakeProjectManager

#include "qtquickappwizard.moc"
