/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "mobilelibrarywizardoptionpage.h"
#include "ui_mobilelibrarywizardoptionpage.h"
#include "qtprojectparameters.h"

#include <coreplugin/coreconstants.h>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

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
    , m_d(new MobileLibraryWizardOptionPagePrivate)
{
    m_d->ui.setupUi(this);
}

MobileLibraryWizardOptionPage::~MobileLibraryWizardOptionPage()
{
    delete m_d;
}

QString MobileLibraryWizardOptionPage::symbianUid() const
{
    return m_d->ui.symbianTargetUid3LineEdit->text();
}

void MobileLibraryWizardOptionPage::setSymbianUid(const QString &uid)
{
    m_d->ui.symbianTargetUid3LineEdit->setText(uid);
}

void MobileLibraryWizardOptionPage::setNetworkEnabled(bool enableIt)
{
    m_d->ui.symbianEnableNetworkCheckBox->setChecked(enableIt);
}

bool MobileLibraryWizardOptionPage::networkEnabled() const
{
    return m_d->ui.symbianEnableNetworkCheckBox->isChecked();
}

void MobileLibraryWizardOptionPage::setLibraryType(int type)
{
    m_d->libraryType = static_cast<QtProjectParameters::Type>(type);
}

} // namespace Internal
} // namespace Qt4ProjectManager
