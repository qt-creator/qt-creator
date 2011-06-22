/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qtquickappwizardpages.h"
#include "ui_qtquickcomponentsetoptionspage.h"
#include "ui_qtquickappwizardsourcespage.h"
#include <coreplugin/coreconstants.h>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {


class QtQuickComponentSetOptionsPagePrivate
{
    Ui::QtQuickComponentSetOptionsPage ui;
    friend class QtQuickComponentSetOptionsPage;
};

class QtQuickAppWizardSourcesPagePrivate
{
    Ui::QtQuickAppWizardSourcesPage ui;
    friend class QtQuickAppWizardSourcesPage;
};


QtQuickComponentSetOptionsPage::QtQuickComponentSetOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new QtQuickComponentSetOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.buttonGroup->setId(m_d->ui.qtquick10RadioButton, 0);
    m_d->ui.buttonGroup->setId(m_d->ui.qtquick11RadioButton, 1);
    m_d->ui.buttonGroup->setId(m_d->ui.symbian10RadioButton, 2);
    connect(m_d->ui.buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(radioButtonChecked(int)));
}

QtQuickComponentSetOptionsPage::~QtQuickComponentSetOptionsPage()
{
    delete m_d;
}

QtQuickApp::ComponentSet QtQuickComponentSetOptionsPage::componentSet() const
{
    switch (m_d->ui.buttonGroup->checkedId()) {
    case 1: return QtQuickApp::QtQuick11Components;
    case 2: return QtQuickApp::Symbian10Components;
    case 0:
    default: return QtQuickApp::QtQuick10Components;
    }
}

void QtQuickComponentSetOptionsPage::setComponentSet(QtQuickApp::ComponentSet componentSet)
{
    switch (componentSet) {
    case QtQuickApp::Symbian10Components: m_d->ui.symbian10RadioButton->click(); break;
    case QtQuickApp::QtQuick11Components: m_d->ui.qtquick11RadioButton->click(); break;
    case QtQuickApp::QtQuick10Components:
    default: m_d->ui.qtquick10RadioButton->click(); break;
    }
}

void QtQuickComponentSetOptionsPage::radioButtonChecked(int index)
{
    m_d->ui.descriptionStackedWidget->setCurrentIndex(index);
}

QtQuickAppWizardSourcesPage::QtQuickAppWizardSourcesPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new QtQuickAppWizardSourcesPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.importLineEdit->setExpectedKind(Utils::PathChooser::File);
    m_d->ui.importLineEdit->setPromptDialogFilter(QLatin1String("*.qml"));
    m_d->ui.importLineEdit->setPromptDialogTitle(tr("Select QML File"));
    connect(m_d->ui.importLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(m_d->ui.importRadioButton,
            SIGNAL(toggled(bool)), SIGNAL(completeChanged()));
    connect(m_d->ui.generateRadioButton, SIGNAL(toggled(bool)),
            m_d->ui.importLineEdit, SLOT(setDisabled(bool)));
    m_d->ui.generateRadioButton->setChecked(true);
}

QtQuickAppWizardSourcesPage::~QtQuickAppWizardSourcesPage()
{
    delete m_d;
}

QtQuickApp::Mode QtQuickAppWizardSourcesPage::mainQmlMode() const
{
    return  m_d->ui.generateRadioButton->isChecked() ? QtQuickApp::ModeGenerate
                                                     : QtQuickApp::ModeImport;
}

QString QtQuickAppWizardSourcesPage::mainQmlFile() const
{
    return mainQmlMode() == QtQuickApp::ModeImport ?
                m_d->ui.importLineEdit->path() : QString();
}

bool QtQuickAppWizardSourcesPage::isComplete() const
{
    return mainQmlMode() != QtQuickApp::ModeImport
            || m_d->ui.importLineEdit->isValid();
}

} // namespace Internal
} // namespace Qt4ProjectManager
