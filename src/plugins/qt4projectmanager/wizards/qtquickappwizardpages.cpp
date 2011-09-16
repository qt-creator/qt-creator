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

QtQuickComponentSetOptionsPage::QtQuickComponentSetOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new QtQuickComponentSetOptionsPagePrivate)
{
    d->ui.setupUi(this);

    d->ui.importLineEdit->setExpectedKind(Utils::PathChooser::File);
    d->ui.importLineEdit->setPromptDialogFilter(QLatin1String("*.qml"));
    d->ui.importLineEdit->setPromptDialogTitle(tr("Select QML File"));
    connect(d->ui.importLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(d->ui.importRadioButton,
            SIGNAL(toggled(bool)), SIGNAL(completeChanged()));

    connect(d->ui.importRadioButton, SIGNAL(toggled(bool)),
            d->ui.importLineEdit, SLOT(setEnabled(bool)));

    d->ui.buttonGroup->setId(d->ui.qtquick10RadioButton, 0);
    d->ui.buttonGroup->setId(d->ui.symbian10RadioButton, 1);
    d->ui.buttonGroup->setId(d->ui.meego10RadioButton, 2);
    d->ui.buttonGroup->setId(d->ui.importRadioButton, 3);
    connect(d->ui.buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(radioButtonChecked(int)));

    setTitle(tr("Qt Quick Application Type"));
}

QtQuickComponentSetOptionsPage::~QtQuickComponentSetOptionsPage()
{
    delete d;
}

QtQuickApp::ComponentSet QtQuickComponentSetOptionsPage::componentSet() const
{
    switch (d->ui.buttonGroup->checkedId()) {
    case 2: return QtQuickApp::Meego10Components;
    case 1: return QtQuickApp::Symbian10Components;
    case 0:
    default: return QtQuickApp::QtQuick10Components;
    }
}

void QtQuickComponentSetOptionsPage::setComponentSet(QtQuickApp::ComponentSet componentSet)
{
    switch (componentSet) {
    case QtQuickApp::Meego10Components: d->ui.meego10RadioButton->click(); break;
    case QtQuickApp::Symbian10Components: d->ui.symbian10RadioButton->click(); break;
    case QtQuickApp::QtQuick10Components:
    default: d->ui.qtquick10RadioButton->click(); break;
    }
}

void QtQuickComponentSetOptionsPage::radioButtonChecked(int index)
{
    d->ui.descriptionStackedWidget->setCurrentIndex(index);
}

QtQuickApp::Mode QtQuickComponentSetOptionsPage::mainQmlMode() const
{
    return  d->ui.importRadioButton->isChecked() ? QtQuickApp::ModeImport
                                                 : QtQuickApp::ModeGenerate;
}

QString QtQuickComponentSetOptionsPage::mainQmlFile() const
{
    return mainQmlMode() == QtQuickApp::ModeImport ?
                d->ui.importLineEdit->path() : QString();
}

bool QtQuickComponentSetOptionsPage::isComplete() const
{
    return mainQmlMode() != QtQuickApp::ModeImport
            || d->ui.importLineEdit->isValid();
}

} // namespace Internal
} // namespace Qt4ProjectManager
