/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlstandaloneappwizardpages.h"
#include "ui_qmlstandaloneappwizardsourcespage.h"
#include <coreplugin/coreconstants.h>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class QmlStandaloneAppWizardSourcesPagePrivate
{
    Ui::QmlStandaloneAppWizardSourcesPage ui;
    friend class QmlStandaloneAppWizardSourcesPage;
};

QmlStandaloneAppWizardSourcesPage::QmlStandaloneAppWizardSourcesPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new QmlStandaloneAppWizardSourcesPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.mainQmlFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    m_d->ui.mainQmlFileLineEdit->setPromptDialogFilter(QLatin1String("*.qml"));
    m_d->ui.mainQmlFileLineEdit->setPromptDialogTitle(tr("Select QML File"));
    connect(m_d->ui.mainQmlFileLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(m_d->ui.importExistingQmlRadioButton,
            SIGNAL(toggled(bool)), SIGNAL(completeChanged()));
    connect(m_d->ui.newQmlRadioButton, SIGNAL(toggled(bool)),
            m_d->ui.mainQmlFileLineEdit, SLOT(setDisabled(bool)));
    m_d->ui.newQmlRadioButton->setChecked(true);
}

QmlStandaloneAppWizardSourcesPage::~QmlStandaloneAppWizardSourcesPage()
{
    delete m_d;
}

QString QmlStandaloneAppWizardSourcesPage::mainQmlFile() const
{
    return m_d->ui.importExistingQmlRadioButton->isChecked() ?
                m_d->ui.mainQmlFileLineEdit->path() : QString();
}

bool QmlStandaloneAppWizardSourcesPage::isComplete() const
{
    return !m_d->ui.importExistingQmlRadioButton->isChecked()
            || m_d->ui.mainQmlFileLineEdit->isValid();
}

} // namespace Internal
} // namespace Qt4ProjectManager
