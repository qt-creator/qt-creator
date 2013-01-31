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

#include "html5appwizardpages.h"
#include "ui_html5appwizardsourcespage.h"
#include <coreplugin/coreconstants.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileDialog>
#include <QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class Html5AppWizardOptionsPagePrivate
{
    Ui::Html5AppWizardSourcesPage ui;
    friend class Html5AppWizardOptionsPage;
};

Html5AppWizardOptionsPage::Html5AppWizardOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new Html5AppWizardOptionsPagePrivate)
{
    d->ui.setupUi(this);
    d->ui.importLineEdit->setExpectedKind(Utils::PathChooser::File);
    d->ui.importLineEdit->setPromptDialogFilter(QLatin1String("*.html"));
    d->ui.importLineEdit->setPromptDialogTitle(tr("Select HTML File"));
    connect(d->ui.importLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(d->ui.importRadioButton,
            SIGNAL(toggled(bool)), SIGNAL(completeChanged()));
    connect(d->ui.generateRadioButton, SIGNAL(toggled(bool)), SLOT(setLineEditsEnabled()));
    connect(d->ui.importRadioButton, SIGNAL(toggled(bool)), SLOT(setLineEditsEnabled()));
    connect(d->ui.urlRadioButton, SIGNAL(toggled(bool)), SLOT(setLineEditsEnabled()));
    d->ui.generateRadioButton->setChecked(true);
}

Html5AppWizardOptionsPage::~Html5AppWizardOptionsPage()
{
    delete d;
}

Html5App::Mode Html5AppWizardOptionsPage::mainHtmlMode() const
{
    Html5App::Mode result = Html5App::ModeGenerate;
    if (d->ui.importRadioButton->isChecked())
        result = Html5App::ModeImport;
    else if (d->ui.urlRadioButton->isChecked())
        result = Html5App::ModeUrl;
    return result;
}

QString Html5AppWizardOptionsPage::mainHtmlData() const
{
    switch (mainHtmlMode()) {
    case Html5App::ModeImport: return d->ui.importLineEdit->path();
    case Html5App::ModeUrl: return d->ui.urlLineEdit->text();
    default:
    case Html5App::ModeGenerate: return QString();
    }
}

void Html5AppWizardOptionsPage::setTouchOptimizationEndabled(bool enabled)
{
    d->ui.touchOptimizationCheckBox->setChecked(enabled);
}

bool Html5AppWizardOptionsPage::touchOptimizationEndabled() const
{
    return d->ui.touchOptimizationCheckBox->isChecked();
}

bool Html5AppWizardOptionsPage::isComplete() const
{
    return mainHtmlMode() != Html5App::ModeImport || d->ui.importLineEdit->isValid();
}

void Html5AppWizardOptionsPage::setLineEditsEnabled()
{
    d->ui.importLineEdit->setEnabled(d->ui.importRadioButton->isChecked());
    d->ui.urlLineEdit->setEnabled(d->ui.urlRadioButton->isChecked());
}

} // namespace Internal
} // namespace Qt4ProjectManager
