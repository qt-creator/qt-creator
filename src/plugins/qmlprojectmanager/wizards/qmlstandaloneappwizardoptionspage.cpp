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

#include "qmlstandaloneappwizardoptionspage.h"
#include "ui_qmlstandaloneappwizardoptionspage.h"

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>

namespace QmlProjectManager {
namespace Internal {

class QmlStandaloneAppWizardOptionPagePrivate
{
    Ui::QmlStandaloneAppWizardOptionPage m_ui;
    QString symbianSvgIcon;
    friend class QmlStandaloneAppWizardOptionPage;
};

QmlStandaloneAppWizardOptionPage::QmlStandaloneAppWizardOptionPage(QWidget *parent) :
    QWizardPage(parent),
    m_d(new QmlStandaloneAppWizardOptionPagePrivate)
{
    m_d->m_ui.setupUi(this);

    const QIcon open = QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
    m_d->m_ui.symbianAppIconLoadToolButton->setIcon(open);
    connect(m_d->m_ui.symbianAppIconLoadToolButton, SIGNAL(clicked()), SLOT(openSymbianSvgIcon()));

    m_d->m_ui.orientationBehaviorComboBox->addItem(tr("Auto rotate orientation"),
                                                   QmlStandaloneApp::Auto);
    m_d->m_ui.orientationBehaviorComboBox->addItem(tr("Lock to landscape orientation"),
                                                   QmlStandaloneApp::LockLandscape);
    m_d->m_ui.orientationBehaviorComboBox->addItem(tr("Lock to portrait orientation"),
                                                   QmlStandaloneApp::LockPortrait);
}

QmlStandaloneAppWizardOptionPage::~QmlStandaloneAppWizardOptionPage()
{
    delete m_d;
}

bool QmlStandaloneAppWizardOptionPage::isComplete() const
{
    return true;
}

void QmlStandaloneAppWizardOptionPage::setOrientation(QmlStandaloneApp::Orientation orientation)
{
    QComboBox *const comboBox = m_d->m_ui.orientationBehaviorComboBox;
    for (int i = 0; i < comboBox->count(); ++i)
        if (comboBox->itemData(i).toInt() == static_cast<int>(orientation)) {
            comboBox->setCurrentIndex(i);
            break;
        }
}

QmlStandaloneApp::Orientation QmlStandaloneAppWizardOptionPage::orientation() const
{
    const int index = m_d->m_ui.orientationBehaviorComboBox->currentIndex();
    return static_cast<QmlStandaloneApp::Orientation>(m_d->m_ui.orientationBehaviorComboBox->itemData(index).toInt());
}

QString QmlStandaloneAppWizardOptionPage::symbianSvgIcon() const
{
    return m_d->symbianSvgIcon;
}

void QmlStandaloneAppWizardOptionPage::setSymbianSvgIcon(const QString &icon)
{
    QPixmap iconPixmap(icon);
    if (!iconPixmap.isNull()) {
        const int symbianIconSize = 44;
        if (iconPixmap.height() > symbianIconSize || iconPixmap.width() > symbianIconSize)
            iconPixmap = iconPixmap.scaledToHeight(symbianIconSize, Qt::SmoothTransformation);
        m_d->m_ui.symbianAppIconPreview->setPixmap(iconPixmap);
        m_d->symbianSvgIcon = icon;
    }
}

QString QmlStandaloneAppWizardOptionPage::symbianUid() const
{
    return m_d->m_ui.symbianTargetUid3LineEdit->text();
}

void QmlStandaloneAppWizardOptionPage::setSymbianUid(const QString &uid)
{
    m_d->m_ui.symbianTargetUid3LineEdit->setText(uid);
}

void QmlStandaloneAppWizardOptionPage::setLoadDummyData(bool loadIt)
{
    m_d->m_ui.loadDummyDataCheckBox->setChecked(loadIt);
}

bool QmlStandaloneAppWizardOptionPage::loadDummyData() const
{
    return m_d->m_ui.loadDummyDataCheckBox->isChecked();
}

void QmlStandaloneAppWizardOptionPage::setNetworkEnabled(bool enableIt)
{
    m_d->m_ui.symbianEnableNetworkChackBox->setChecked(enableIt);
}

bool QmlStandaloneAppWizardOptionPage::networkEnabled() const
{
    return m_d->m_ui.symbianEnableNetworkChackBox->isChecked();
}

void QmlStandaloneAppWizardOptionPage::openSymbianSvgIcon()
{
    const QString svgIcon = QFileDialog::getOpenFileName(
            this,
            m_d->m_ui.symbianAppIconLabel->text(),
            QDesktopServices::storageLocation(QDesktopServices::PicturesLocation),
            QLatin1String("*.svg"));
    if (!svgIcon.isEmpty())
        setSymbianSvgIcon(svgIcon);
}

} // namespace Internal
} // namespace QmlProjectManager
