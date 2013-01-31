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
        d->ui.formLayout->removeItem(d->ui.horizontalLayout);
        delete d->ui.horizontalLayout;
        d->ui.horizontalLayout = 0;
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
