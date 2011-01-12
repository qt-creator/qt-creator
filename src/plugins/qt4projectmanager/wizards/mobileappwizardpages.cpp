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

#include "mobileappwizardpages.h"
#include "ui_mobileappwizardoptionspage.h"
#include <coreplugin/coreconstants.h>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class MobileAppWizardOptionsPagePrivate
{
    Ui::MobileAppWizardOptionPage ui;
    QString symbianSvgIcon;
    QString maemoPngIcon;
    friend class MobileAppWizardOptionsPage;
};

MobileAppWizardOptionsPage::MobileAppWizardOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardOptionsPagePrivate)
{
    m_d->ui.setupUi(this);

    const QIcon open = QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
    m_d->ui.symbianAppIconLoadToolButton->setIcon(open);
    connect(m_d->ui.symbianAppIconLoadToolButton, SIGNAL(clicked()), SLOT(openSymbianSvgIcon()));
    connect(m_d->ui.maemoPngIconButton, SIGNAL(clicked()), this,
        SLOT(openMaemoPngIcon()));

    m_d->ui.orientationBehaviorComboBox->addItem(tr("Automatically Rotate Orientation"),
        AbstractMobileApp::ScreenOrientationAuto);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Landscape Orientation"),
        AbstractMobileApp::ScreenOrientationLockLandscape);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Portrait Orientation"),
        AbstractMobileApp::ScreenOrientationLockPortrait);
}

MobileAppWizardOptionsPage::~MobileAppWizardOptionsPage()
{
    delete m_d;
}

void MobileAppWizardOptionsPage::setOrientation(AbstractMobileApp::ScreenOrientation orientation)
{
    QComboBox *const comboBox = m_d->ui.orientationBehaviorComboBox;
    for (int i = 0; i < comboBox->count(); ++i)
        if (comboBox->itemData(i).toInt() == static_cast<int>(orientation)) {
            comboBox->setCurrentIndex(i);
            break;
        }
}

AbstractMobileApp::ScreenOrientation MobileAppWizardOptionsPage::orientation() const
{
    const int index = m_d->ui.orientationBehaviorComboBox->currentIndex();
    return static_cast<AbstractMobileApp::ScreenOrientation>(m_d->ui.orientationBehaviorComboBox->itemData(index).toInt());
}

QString MobileAppWizardOptionsPage::symbianSvgIcon() const
{
    return m_d->symbianSvgIcon;
}

void MobileAppWizardOptionsPage::setSymbianSvgIcon(const QString &icon)
{
    QPixmap iconPixmap(icon);
    if (!iconPixmap.isNull()) {
        const int symbianIconSize = 44;
        if (iconPixmap.height() > symbianIconSize || iconPixmap.width() > symbianIconSize)
            iconPixmap = iconPixmap.scaledToHeight(symbianIconSize, Qt::SmoothTransformation);
        m_d->ui.symbianAppIconPreview->setPixmap(iconPixmap);
        m_d->symbianSvgIcon = icon;
    }
}

QString MobileAppWizardOptionsPage::maemoPngIcon() const
{
    return m_d->maemoPngIcon;
}

void MobileAppWizardOptionsPage::setMaemoPngIcon(const QString &icon)
{
    QString error;
    QPixmap iconPixmap(icon);
    if (iconPixmap.isNull())
        error = tr("The file is not a valid image.");
    else if (iconPixmap.size() != QSize(64, 64))
        error = tr("The icon has an invalid size.");
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Icon unusable"), error);
    } else {
        m_d->ui.maemoPngIconButton->setIcon(iconPixmap);
        m_d->maemoPngIcon = icon;
    }
}

QString MobileAppWizardOptionsPage::symbianUid() const
{
    return m_d->ui.symbianTargetUid3LineEdit->text();
}

void MobileAppWizardOptionsPage::setSymbianUid(const QString &uid)
{
    m_d->ui.symbianTargetUid3LineEdit->setText(uid);
}

void MobileAppWizardOptionsPage::setNetworkEnabled(bool enableIt)
{
    m_d->ui.symbianEnableNetworkChackBox->setChecked(enableIt);
}

bool MobileAppWizardOptionsPage::networkEnabled() const
{
    return m_d->ui.symbianEnableNetworkChackBox->isChecked();
}

void MobileAppWizardOptionsPage::openSymbianSvgIcon()
{
    const QString svgIcon = QFileDialog::getOpenFileName(
            this,
            m_d->ui.symbianAppIconLabel->text(),
            QDesktopServices::storageLocation(QDesktopServices::PicturesLocation),
            QLatin1String("*.svg"));
    if (!svgIcon.isEmpty())
        setSymbianSvgIcon(svgIcon);
}

void MobileAppWizardOptionsPage::openMaemoPngIcon()
{
    const QString iconPath = QFileDialog::getOpenFileName(this,
        m_d->ui.maemoAppIconLabel->text(), m_d->maemoPngIcon,
        QLatin1String("*.png"));
    if (!iconPath.isEmpty())
        setMaemoPngIcon(iconPath);
}

} // namespace Internal
} // namespace Qt4ProjectManager
