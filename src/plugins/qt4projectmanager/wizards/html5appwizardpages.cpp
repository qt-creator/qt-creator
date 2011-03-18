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

#include "html5appwizardpages.h"
#include "ui_html5appwizardsourcespage.h"
#include <coreplugin/coreconstants.h>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class Html5AppWizardOptionsPagePrivate
{
    Ui::Html5AppWizardSourcesPage ui;
    friend class Html5AppWizardOptionsPage;
};

Html5AppWizardOptionsPage::Html5AppWizardOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new Html5AppWizardOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.importLineEdit->setExpectedKind(Utils::PathChooser::File);
    m_d->ui.importLineEdit->setPromptDialogFilter(QLatin1String("*.html"));
    m_d->ui.importLineEdit->setPromptDialogTitle(tr("Select HTML File"));
    connect(m_d->ui.importLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(m_d->ui.importRadioButton,
            SIGNAL(toggled(bool)), SIGNAL(completeChanged()));
    connect(m_d->ui.generateRadioButton, SIGNAL(toggled(bool)), SLOT(setLineEditsEnabled()));
    connect(m_d->ui.importRadioButton, SIGNAL(toggled(bool)), SLOT(setLineEditsEnabled()));
    connect(m_d->ui.urlRadioButton, SIGNAL(toggled(bool)), SLOT(setLineEditsEnabled()));
    m_d->ui.generateRadioButton->setChecked(true);
}

Html5AppWizardOptionsPage::~Html5AppWizardOptionsPage()
{
    delete m_d;
}

Html5App::Mode Html5AppWizardOptionsPage::mainHtmlMode() const
{
    Html5App::Mode result = Html5App::ModeGenerate;
    if (m_d->ui.importRadioButton->isChecked())
        result = Html5App::ModeImport;
    else if (m_d->ui.urlRadioButton->isChecked())
        result = Html5App::ModeUrl;
    return result;
}

QString Html5AppWizardOptionsPage::mainHtmlData() const
{
    switch (mainHtmlMode()) {
    case Html5App::ModeImport: return m_d->ui.importLineEdit->path();
    case Html5App::ModeUrl: return m_d->ui.urlLineEdit->text();
    default:
    case Html5App::ModeGenerate: return QString();
    }
}

void Html5AppWizardOptionsPage::setTouchOptimizationEndabled(bool enabled)
{
    m_d->ui.touchOptimizationCheckBox->setChecked(enabled);
}

bool Html5AppWizardOptionsPage::touchOptimizationEndabled() const
{
    return m_d->ui.touchOptimizationCheckBox->isChecked();
}

bool Html5AppWizardOptionsPage::isComplete() const
{
    return mainHtmlMode() != Html5App::ModeImport || m_d->ui.importLineEdit->isValid();
}

void Html5AppWizardOptionsPage::setLineEditsEnabled()
{
    m_d->ui.importLineEdit->setEnabled(m_d->ui.importRadioButton->isChecked());
    m_d->ui.urlLineEdit->setEnabled(m_d->ui.urlRadioButton->isChecked());
}

} // namespace Internal
} // namespace Qt4ProjectManager
