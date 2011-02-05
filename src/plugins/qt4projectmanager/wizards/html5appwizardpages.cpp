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

class Html5AppWizardSourcesPagePrivate
{
    Ui::Html5AppWizardSourcesPage ui;
    friend class Html5AppWizardSourcesPage;
};

Html5AppWizardSourcesPage::Html5AppWizardSourcesPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new Html5AppWizardSourcesPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.mainHtmlFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    m_d->ui.mainHtmlFileLineEdit->setPromptDialogFilter(QLatin1String("*.html"));
    m_d->ui.mainHtmlFileLineEdit->setPromptDialogTitle(tr("Select Html File"));
    connect(m_d->ui.mainHtmlFileLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(m_d->ui.importExistingHtmlRadioButton,
            SIGNAL(toggled(bool)), SIGNAL(completeChanged()));
    connect(m_d->ui.newHtmlRadioButton, SIGNAL(toggled(bool)),
            m_d->ui.mainHtmlFileLineEdit, SLOT(setDisabled(bool)));
    m_d->ui.newHtmlRadioButton->setChecked(true);
}

Html5AppWizardSourcesPage::~Html5AppWizardSourcesPage()
{
    delete m_d;
}

QString Html5AppWizardSourcesPage::mainHtmlFile() const
{
    return m_d->ui.importExistingHtmlRadioButton->isChecked() ?
                m_d->ui.mainHtmlFileLineEdit->path() : QString();
}

bool Html5AppWizardSourcesPage::isComplete() const
{
    return !m_d->ui.importExistingHtmlRadioButton->isChecked()
            || m_d->ui.mainHtmlFileLineEdit->isValid();
}

} // namespace Internal
} // namespace Qt4ProjectManager
