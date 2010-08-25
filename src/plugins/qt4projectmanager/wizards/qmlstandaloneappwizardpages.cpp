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
#include <coreplugin/coreconstants.h>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>

namespace Qt4ProjectManager {
namespace Internal {

class QmlStandaloneAppWizardSourcesPagePrivate
{
    Ui::QmlStandaloneAppWizardSourcesPage ui;
    bool mainQmlFileChooserVisible;
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
    m_d->ui.addModuleUriButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PLUS)));
    m_d->ui.removeModuleUriButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_MINUS)));
    m_d->ui.addImportPathButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PLUS)));
    m_d->ui.removeImportPathButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_MINUS)));
    setMainQmlFileChooserVisible(true);
    setModulesError(QString());
    connect(m_d->ui.mainQmlFileLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(m_d->ui.urisListWidget, SIGNAL(itemChanged(QListWidgetItem*)), SLOT(handleModulesChanged()));
    connect(m_d->ui.importPathsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), SLOT(handleModulesChanged()));
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
    return (!m_d->mainQmlFileChooserVisible || m_d->ui.mainQmlFileLineEdit->isValid())
            && m_d->ui.errorLabel->text().isEmpty();
}

void QmlStandaloneAppWizardSourcesPage::setMainQmlFileChooserVisible(bool visible)
{
    m_d->mainQmlFileChooserVisible = visible;
    m_d->ui.mainQmlFileGroupBox->setVisible(m_d->mainQmlFileChooserVisible);
}

void QmlStandaloneAppWizardSourcesPage::setModulesError(const QString &error)
{
    m_d->ui.errorLabel->setText(error);
    m_d->ui.errorLabel->setVisible(!error.isEmpty());
}

void QmlStandaloneAppWizardSourcesPage::on_addModuleUriButton_clicked()
{
    setFocus(); // Interrupt any current editing
    QListWidgetItem *item = new QListWidgetItem(m_d->ui.urisListWidget);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_d->ui.urisListWidget->editItem(item);
}

void QmlStandaloneAppWizardSourcesPage::on_removeModuleUriButton_clicked()
{
    const int currentRow = m_d->ui.urisListWidget->currentRow();
    if (currentRow >= 0) {
        m_d->ui.urisListWidget->takeItem(currentRow);
        handleModulesChanged();
    }
}

void QmlStandaloneAppWizardSourcesPage::on_addImportPathButton_clicked()
{
    const QString path = QFileDialog::getExistingDirectory(this,
        tr("Select an import path for QML modules."), mainQmlFile());
    if (!path.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem(QDir::toNativeSeparators(path), m_d->ui.importPathsListWidget);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
}

void QmlStandaloneAppWizardSourcesPage::on_removeImportPathButton_clicked()
{
    const int currentRow = m_d->ui.importPathsListWidget->currentRow();
    if (currentRow >= 0) {
        m_d->ui.importPathsListWidget->takeItem(currentRow);
        handleModulesChanged();
    }
}

static inline QStringList ertriesFromListWidget(const QListWidget &listWidget)
{
    QStringList result;
    for (int i = 0; i < listWidget.count(); ++i) {
        const QString text = listWidget.item(i)->text().trimmed();
        if (!text.isEmpty())
            result.append(text);
    }
    return result;
}

void QmlStandaloneAppWizardSourcesPage::handleModulesChanged()
{
    const QStringList uris = ertriesFromListWidget(*m_d->ui.urisListWidget);
    const QStringList paths = ertriesFromListWidget(*m_d->ui.importPathsListWidget);
    emit externalModulesChanged(uris, paths);
    emit completeChanged();
}

QStringList QmlStandaloneAppWizardSourcesPage::moduleUris() const
{
    return ertriesFromListWidget(*m_d->ui.urisListWidget);
}

QStringList QmlStandaloneAppWizardSourcesPage::moduleImportPaths() const
{
    return ertriesFromListWidget(*m_d->ui.importPathsListWidget);
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
} // namespace Qt4ProjectManager
