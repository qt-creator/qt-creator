/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "mobilelibrarywizardoptionpage.h"
#include "ui_mobilelibrarywizardoptionpage.h"
#include "qtprojectparameters.h"

#include <coreplugin/coreconstants.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class MobileLibraryWizardOptionPagePrivate
{
    Ui::MobileLibraryWizardOptionPage ui;
    friend class MobileLibraryWizardOptionPage;

    QtProjectParameters::Type libraryType;
};

MobileLibraryWizardOptionPage::MobileLibraryWizardOptionPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new MobileLibraryWizardOptionPagePrivate)
{
    d->ui.setupUi(this);
}

MobileLibraryWizardOptionPage::~MobileLibraryWizardOptionPage()
{
    delete d;
}

QString MobileLibraryWizardOptionPage::symbianUid() const
{
    return d->ui.symbianTargetUid3LineEdit->text();
}

void MobileLibraryWizardOptionPage::setSymbianUid(const QString &uid)
{
    d->ui.symbianTargetUid3LineEdit->setText(uid);
}

void MobileLibraryWizardOptionPage::setNetworkEnabled(bool enableIt)
{
    d->ui.symbianEnableNetworkCheckBox->setChecked(enableIt);
}

bool MobileLibraryWizardOptionPage::networkEnabled() const
{
    return d->ui.symbianEnableNetworkCheckBox->isChecked();
}

QString MobileLibraryWizardOptionPage::qtPluginDirectory() const
{
    return d->ui.qtPluginLocationLineEdit->text();
}

void MobileLibraryWizardOptionPage::setQtPluginDirectory(const QString &directory)
{
    d->ui.qtPluginLocationLineEdit->setText(directory);
}

void MobileLibraryWizardOptionPage::setLibraryType(int type)
{
    d->libraryType = static_cast<QtProjectParameters::Type>(type);

    if (type != QtProjectParameters::Qt4Plugin) {
        d->ui.qtPluginLocationLineEdit->setVisible(false);
        d->ui.qtPluginLocationLabel->setVisible(false);
        d->ui.formLayout_2->removeItem(d->ui.horizontalLayout_2);
        delete d->ui.horizontalLayout_2;
        d->ui.horizontalLayout_2 = 0;
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
