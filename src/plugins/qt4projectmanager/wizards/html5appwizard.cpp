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

#include "mobileappwizardpages.h"

#include "html5app.h"
#include "html5appwizard.h"
#include "html5appwizardpages.h"
#include "targetsetuppage.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtsupportconstants.h>
#include <limits>

#include <QCoreApplication>
#include <QIcon>

namespace Qt4ProjectManager {
namespace Internal {

class Html5AppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit Html5AppWizardDialog(QWidget *parent, const Core::WizardDialogParameters &parameters);

private:
    class Html5AppWizardOptionsPage *m_htmlOptionsPage;
    friend class Html5AppWizard;
};

Html5AppWizardDialog::Html5AppWizardDialog(QWidget *parent,
                                           const Core::WizardDialogParameters &parameters)
    : AbstractMobileAppWizardDialog(parent, QtSupport::QtVersionNumber(),
      QtSupport::QtVersionNumber(4, INT_MAX, INT_MAX), parameters),
      m_htmlOptionsPage(0)
{
    setWindowTitle(tr("New HTML5 Application"));
    setIntroDescription(tr("This wizard generates a HTML5 application project."));

    m_htmlOptionsPage = new Html5AppWizardOptionsPage;
    addPageWithTitle(m_htmlOptionsPage, tr("HTML Options"));
    addMobilePages();
}


class Html5AppWizardPrivate
{
    class Html5App *app;
    class Html5AppWizardDialog *wizardDialog;
    friend class Html5AppWizard;
};

Html5AppWizard::Html5AppWizard()
    : AbstractMobileAppWizard(parameters())
    , d(new Html5AppWizardPrivate)
{
    d->app = new Html5App;
    d->wizardDialog = 0;
}

Html5AppWizard::~Html5AppWizard()
{
    delete d->app;
    delete d;
}

Core::FeatureSet Html5AppWizard::requiredFeatures() const
{
    return Core::Feature(QtSupport::Constants::FEATURE_QT_WEBKIT);
}

Core::BaseFileWizardParameters Html5AppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_HTML5_APP)));
    parameters.setDisplayName(tr("HTML5 Application"));
    parameters.setId(QLatin1String("QA.HTML5A Application"));
    parameters.setDescription(tr("Creates an HTML5 application project that can contain "
                                 "both HTML5 and C++ code and includes a WebKit view.\n\n"
                                 "You can build the application and deploy it on desktop and "
                                 "mobile target platforms."));
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY));
    return parameters;
}

AbstractMobileAppWizardDialog *Html5AppWizard::createWizardDialogInternal(QWidget *parent,
                                                                          const Core::WizardDialogParameters &parameters) const
{
    d->wizardDialog = new Html5AppWizardDialog(parent, parameters);
    d->wizardDialog->m_htmlOptionsPage->setTouchOptimizationEndabled(
                d->app->touchOptimizedNavigationEnabled());
    return d->wizardDialog;
}

void Html5AppWizard::projectPathChanged(const QString &path) const
{
    if (d->wizardDialog->targetsPage())
        d->wizardDialog->targetsPage()->setProFilePath(path);
}

void Html5AppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const Html5AppWizardDialog *wizard = qobject_cast<const Html5AppWizardDialog*>(w);
    d->app->setMainHtml(wizard->m_htmlOptionsPage->mainHtmlMode(),
                          wizard->m_htmlOptionsPage->mainHtmlData());
    d->app->setTouchOptimizedNavigationEnabled(
                wizard->m_htmlOptionsPage->touchOptimizationEndabled());
}

QString Html5AppWizard::fileToOpenPostGeneration() const
{
    return d->app->mainHtmlMode() == Html5App::ModeUrl ?
                d->app->path(AbstractMobileApp::MainCpp)
              : d->app->path(Html5App::MainHtml);
}

AbstractMobileApp *Html5AppWizard::app() const
{
    return d->app;
}

AbstractMobileAppWizardDialog *Html5AppWizard::wizardDialog() const
{
    return d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "html5appwizard.moc"
