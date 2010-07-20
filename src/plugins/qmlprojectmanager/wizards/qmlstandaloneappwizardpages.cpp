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
#include "ui_qmlstandaloneappwizardoptionspage.h"

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>

namespace QmlProjectManager {
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
    m_d->ui.mainQmlFileLineEdit->setPromptDialogTitle(tr("Select the main QML file of the application."));
    m_d->ui.qmlModulesGroupBox->setEnabled(false); // TODO: implement modules selection
    connect(m_d->ui.mainQmlFileLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
}

QmlStandaloneAppWizardSourcesPage::~QmlStandaloneAppWizardSourcesPage()
{
    delete m_d;
}

QString QmlStandaloneAppWizardSourcesPage::mainQmlFile() const
{
    return m_d->ui.mainQmlFileLineEdit->path();
}

bool QmlStandaloneAppWizardSourcesPage::isComplete() const
{
    return m_d->ui.mainQmlFileLineEdit->isValid();
}

void QmlStandaloneAppWizardSourcesPage::setMainQmlFileChooserVisible(bool visible)
{
    m_d->ui.mainQmlFileLineEdit->setVisible(visible);
}

class QmlStandaloneAppWizardOptionsPagePrivate
{
    Ui::QmlStandaloneAppWizardOptionPage ui;
    QString symbianSvgIcon;
    friend class QmlStandaloneAppWizardOptionsPage;
};

QmlStandaloneAppWizardOptionsPage::QmlStandaloneAppWizardOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new QmlStandaloneAppWizardOptionsPagePrivate)
{
    m_d->ui.setupUi(this);

    const QIcon open = QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
    m_d->ui.symbianAppIconLoadToolButton->setIcon(open);
    connect(m_d->ui.symbianAppIconLoadToolButton, SIGNAL(clicked()), SLOT(openSymbianSvgIcon()));

    m_d->ui.orientationBehaviorComboBox->addItem(tr("Auto rotate orientation"),
                                                   QmlStandaloneApp::Auto);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to landscape orientation"),
                                                   QmlStandaloneApp::LockLandscape);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to portrait orientation"),
                                                   QmlStandaloneApp::LockPortrait);
}

QmlStandaloneAppWizardOptionsPage::~QmlStandaloneAppWizardOptionsPage()
{
    delete m_d;
}

void QmlStandaloneAppWizardOptionsPage::setOrientation(QmlStandaloneApp::Orientation orientation)
{
    QComboBox *const comboBox = m_d->ui.orientationBehaviorComboBox;
    for (int i = 0; i < comboBox->count(); ++i)
        if (comboBox->itemData(i).toInt() == static_cast<int>(orientation)) {
            comboBox->setCurrentIndex(i);
            break;
        }
}

QmlStandaloneApp::Orientation QmlStandaloneAppWizardOptionsPage::orientation() const
{
    const int index = m_d->ui.orientationBehaviorComboBox->currentIndex();
    return static_cast<QmlStandaloneApp::Orientation>(m_d->ui.orientationBehaviorComboBox->itemData(index).toInt());
}

QString QmlStandaloneAppWizardOptionsPage::symbianSvgIcon() const
{
    return m_d->symbianSvgIcon;
}

void QmlStandaloneAppWizardOptionsPage::setSymbianSvgIcon(const QString &icon)
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

QString QmlStandaloneAppWizardOptionsPage::symbianUid() const
{
    return m_d->ui.symbianTargetUid3LineEdit->text();
}

void QmlStandaloneAppWizardOptionsPage::setSymbianUid(const QString &uid)
{
    m_d->ui.symbianTargetUid3LineEdit->setText(uid);
}

void QmlStandaloneAppWizardOptionsPage::setLoadDummyData(bool loadIt)
{
    m_d->ui.loadDummyDataCheckBox->setChecked(loadIt);
}

bool QmlStandaloneAppWizardOptionsPage::loadDummyData() const
{
    return m_d->ui.loadDummyDataCheckBox->isChecked();
}

void QmlStandaloneAppWizardOptionsPage::setNetworkEnabled(bool enableIt)
{
    m_d->ui.symbianEnableNetworkChackBox->setChecked(enableIt);
}

bool QmlStandaloneAppWizardOptionsPage::networkEnabled() const
{
    return m_d->ui.symbianEnableNetworkChackBox->isChecked();
}

void QmlStandaloneAppWizardOptionsPage::openSymbianSvgIcon()
{
    const QString svgIcon = QFileDialog::getOpenFileName(
            this,
            m_d->ui.symbianAppIconLabel->text(),
            QDesktopServices::storageLocation(QDesktopServices::PicturesLocation),
            QLatin1String("*.svg"));
    if (!svgIcon.isEmpty())
        setSymbianSvgIcon(svgIcon);
}

} // namespace Internal
} // namespace QmlProjectManager
