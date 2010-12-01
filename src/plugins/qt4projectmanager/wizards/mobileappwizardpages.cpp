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

#include "mobileappwizardpages.h"
#include "ui_mobileappwizardgenericoptionspage.h"
#include "ui_mobileappwizardmaemooptionspage.h"
#include "ui_mobileappwizardsymbianoptionspage.h"
#include <coreplugin/coreconstants.h>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class MobileAppWizardGenericOptionsPagePrivate
{
    Ui::MobileAppWizardGenericOptionsPage ui;
    friend class MobileAppWizardGenericOptionsPage;
};

class MobileAppWizardSymbianOptionsPagePrivate
{
    Ui::MobileAppWizardSymbianOptionsPage ui;
    QString svgIcon;
    friend class MobileAppWizardSymbianOptionsPage;
};

class MobileAppWizardMaemoOptionsPagePrivate
{
    Ui::MobileAppWizardMaemoOptionsPage ui;
    QString pngIcon;
    friend class MobileAppWizardMaemoOptionsPage;
};


MobileAppWizardGenericOptionsPage::MobileAppWizardGenericOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardGenericOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Automatically Rotate Orientation"),
        AbstractMobileApp::ScreenOrientationAuto);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Landscape Orientation"),
        AbstractMobileApp::ScreenOrientationLockLandscape);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Portrait Orientation"),
        AbstractMobileApp::ScreenOrientationLockPortrait);
}

MobileAppWizardGenericOptionsPage::~MobileAppWizardGenericOptionsPage()
{
    delete m_d;
}

void MobileAppWizardGenericOptionsPage::setOrientation(AbstractMobileApp::ScreenOrientation orientation)
{
    QComboBox *const comboBox = m_d->ui.orientationBehaviorComboBox;
    for (int i = 0; i < comboBox->count(); ++i)
        if (comboBox->itemData(i).toInt() == static_cast<int>(orientation)) {
            comboBox->setCurrentIndex(i);
            break;
        }
}

AbstractMobileApp::ScreenOrientation MobileAppWizardGenericOptionsPage::orientation() const
{
    const int index = m_d->ui.orientationBehaviorComboBox->currentIndex();
    return static_cast<AbstractMobileApp::ScreenOrientation>(m_d->ui.orientationBehaviorComboBox->itemData(index).toInt());
}


MobileAppWizardSymbianOptionsPage::MobileAppWizardSymbianOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardSymbianOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    const QIcon open = QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
    m_d->ui.appIconLoadToolButton->setIcon(open);
    connect(m_d->ui.appIconLoadToolButton, SIGNAL(clicked()), SLOT(openSvgIcon()));
}

MobileAppWizardSymbianOptionsPage::~MobileAppWizardSymbianOptionsPage()
{
    delete m_d;
}

QString MobileAppWizardSymbianOptionsPage::svgIcon() const
{
    return m_d->svgIcon;
}

void MobileAppWizardSymbianOptionsPage::setSvgIcon(const QString &icon)
{
    QPixmap iconPixmap(icon);
    if (!iconPixmap.isNull()) {
        const int symbianIconSize = 44;
        if (iconPixmap.height() > symbianIconSize || iconPixmap.width() > symbianIconSize)
            iconPixmap = iconPixmap.scaledToHeight(symbianIconSize, Qt::SmoothTransformation);
        m_d->ui.appIconPreview->setPixmap(iconPixmap);
        m_d->svgIcon = icon;
    }
}

QString MobileAppWizardSymbianOptionsPage::symbianUid() const
{
    return m_d->ui.uid3LineEdit->text();
}

void MobileAppWizardSymbianOptionsPage::setSymbianUid(const QString &uid)
{
    m_d->ui.uid3LineEdit->setText(uid);
}

void MobileAppWizardSymbianOptionsPage::setNetworkEnabled(bool enableIt)
{
    m_d->ui.enableNetworkCheckBox->setChecked(enableIt);
}

bool MobileAppWizardSymbianOptionsPage::networkEnabled() const
{
    return m_d->ui.enableNetworkCheckBox->isChecked();
}

void MobileAppWizardSymbianOptionsPage::openSvgIcon()
{
    const QString svgIcon = QFileDialog::getOpenFileName(
            this,
            m_d->ui.appIconLabel->text(),
            QDesktopServices::storageLocation(QDesktopServices::PicturesLocation),
            QLatin1String("*.svg"));
    if (!svgIcon.isEmpty())
        setSvgIcon(svgIcon);
}


MobileAppWizardMaemoOptionsPage::MobileAppWizardMaemoOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardMaemoOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    connect(m_d->ui.pngIconButton, SIGNAL(clicked()), this, SLOT(openPngIcon()));
}

MobileAppWizardMaemoOptionsPage::~MobileAppWizardMaemoOptionsPage()
{
    delete m_d;
}

QString MobileAppWizardMaemoOptionsPage::pngIcon() const
{
    return m_d->pngIcon;
}

void MobileAppWizardMaemoOptionsPage::setPngIcon(const QString &icon)
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
        m_d->ui.pngIconButton->setIcon(iconPixmap);
        m_d->pngIcon = icon;
    }
}

void MobileAppWizardMaemoOptionsPage::openPngIcon()
{
    const QString iconPath = QFileDialog::getOpenFileName(this,
        m_d->ui.appIconLabel->text(), m_d->pngIcon,
        QLatin1String("*.png"));
    if (!iconPath.isEmpty())
        setPngIcon(iconPath);
}

} // namespace Internal
} // namespace Qt4ProjectManager
