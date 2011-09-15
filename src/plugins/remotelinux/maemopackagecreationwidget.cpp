/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "maemopackagecreationwidget.h"
#include "ui_maemopackagecreationwidget.h"

#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "qt4maemotarget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtGui/QFileDialog>
#include <QtGui/QImageReader>
#include <QtGui/QMessageBox>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

// TODO: Split up into dedicated widgets for Debian and RPM steps.
MaemoPackageCreationWidget::MaemoPackageCreationWidget(AbstractMaemoPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::MaemoPackageCreationWidget)
{
    m_ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QTimer::singleShot(0, this, SLOT(initGui()));
}

MaemoPackageCreationWidget::~MaemoPackageCreationWidget()
{
    delete m_ui;
}

void MaemoPackageCreationWidget::initGui()
{
    m_ui->shortDescriptionLineEdit->setMaxLength(60);
    updateVersionInfo();
    const AbstractDebBasedQt4MaemoTarget * const debBasedMaemoTarget
        = m_step->debBasedMaemoTarget();
    if (debBasedMaemoTarget) {
        const QSize iconSize = debBasedMaemoTarget->packageManagerIconSize();
        m_ui->packageManagerIconButton->setFixedSize(iconSize);
        m_ui->packageManagerIconButton->setToolTip(tr("Size should be %1x%2 pixels")
            .arg(iconSize.width()).arg(iconSize.height()));
        m_ui->editSpecFileButton->setVisible(false);
        updateDebianFileList();
        handleControlFileUpdate();
        connect(m_ui->packageManagerNameLineEdit, SIGNAL(editingFinished()),
            SLOT(setPackageManagerName()));
        connect(debBasedMaemoTarget, SIGNAL(debianDirContentsChanged()),
            SLOT(updateDebianFileList()));
        connect(debBasedMaemoTarget, SIGNAL(changeLogChanged()),
            SLOT(updateVersionInfo()));
        connect(debBasedMaemoTarget, SIGNAL(controlChanged()),
            SLOT(handleControlFileUpdate()));
    } else {
        m_ui->packageManagerNameLabel->hide();
        m_ui->packageManagerNameLineEdit->hide();
        m_ui->packageManagerIconLabel->hide();
        m_ui->packageManagerIconButton->hide();
        m_ui->editDebianFileLabel->hide();
        m_ui->debianFilesComboBox->hide();
        m_ui->editDebianFileButton->hide();

        // This is fragile; be careful when editing the UI file.
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(4, QFormLayout::LabelRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(4, QFormLayout::FieldRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(5, QFormLayout::LabelRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(5, QFormLayout::FieldRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(6, QFormLayout::LabelRole));
        m_ui->formLayout->removeItem(m_ui->formLayout->itemAt(6, QFormLayout::FieldRole));
        handleSpecFileUpdate();
        connect(m_step->rpmBasedMaemoTarget(), SIGNAL(specFileChanged()),
            SLOT(handleSpecFileUpdate()));
        connect(m_ui->editSpecFileButton, SIGNAL(clicked()),
            SLOT(editSpecFile()));
    }
    connect(m_step, SIGNAL(packageFilePathChanged()), this,
        SIGNAL(updateSummary()));
    connect(m_ui->packageNameLineEdit, SIGNAL(editingFinished()),
        SLOT(setPackageName()));
    connect(m_ui->shortDescriptionLineEdit, SIGNAL(editingFinished()),
        SLOT(setShortDescription()));
}

void MaemoPackageCreationWidget::updateDebianFileList()
{
    m_ui->debianFilesComboBox->clear();
    const QStringList &debianFiles = m_step->debBasedMaemoTarget()->debianFiles();
    foreach (const QString &fileName, debianFiles) {
        if (fileName != QLatin1String("compat")
                && !fileName.endsWith(QLatin1Char('~')))
            m_ui->debianFilesComboBox->addItem(fileName);
    }
}

void MaemoPackageCreationWidget::updateVersionInfo()
{
    QString error;
    QString versionString = m_step->versionString(&error);
    if (versionString.isEmpty()) {
        QMessageBox::critical(this, tr("No Version Available."), error);
        versionString = AbstractMaemoPackageCreationStep::DefaultVersionNumber;
    }
    const QStringList list = versionString.split(QLatin1Char('.'),
        QString::SkipEmptyParts);
    const bool blocked = m_ui->major->signalsBlocked();
    m_ui->major->blockSignals(true);
    m_ui->minor->blockSignals(true);
    m_ui->patch->blockSignals(true);
    m_ui->major->setValue(list.value(0, QLatin1String("0")).toInt());
    m_ui->minor->setValue(list.value(1, QLatin1String("0")).toInt());
    m_ui->patch->setValue(list.value(2, QLatin1String("0")).toInt());
    m_ui->major->blockSignals(blocked);
    m_ui->minor->blockSignals(blocked);
    m_ui->patch->blockSignals(blocked);
    updateSummary();
}

void MaemoPackageCreationWidget::handleControlFileUpdate()
{
    updatePackageName();
    updateShortDescription();
    updatePackageManagerName();
    updatePackageManagerIcon();
    updateSummary();
}

void MaemoPackageCreationWidget::handleSpecFileUpdate()
{
    updatePackageName();
    updateShortDescription();
    updateVersionInfo();
    updateSummary();
}

void MaemoPackageCreationWidget::updatePackageManagerIcon()
{
    QString error;
    const QIcon &icon = m_step->debBasedMaemoTarget()->packageManagerIcon(&error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, tr("Could not read icon"), error);
    } else {
        m_ui->packageManagerIconButton->setIcon(icon);
        m_ui->packageManagerIconButton->setIconSize(m_ui->packageManagerIconButton->size());
    }
}

void MaemoPackageCreationWidget::updatePackageName()
{
    m_ui->packageNameLineEdit->setText(m_step->maemoTarget()->packageName());
}

void MaemoPackageCreationWidget::updatePackageManagerName()
{
    m_ui->packageManagerNameLineEdit->setText(m_step->debBasedMaemoTarget()->packageManagerName());
}

void MaemoPackageCreationWidget::updateShortDescription()
{
    m_ui->shortDescriptionLineEdit->setText(m_step->maemoTarget()->shortDescription());
}

void MaemoPackageCreationWidget::setPackageManagerIcon()
{
    QString imageFilter = tr("Images") + QLatin1String("( ");
    const QList<QByteArray> &imageTypes = QImageReader::supportedImageFormats();
    foreach (const QByteArray &imageType, imageTypes)
        imageFilter += "*." + QString::fromAscii(imageType) + QLatin1Char(' ');
    imageFilter += QLatin1Char(')');
    const QString iconFileName = QFileDialog::getOpenFileName(this,
        tr("Choose Image (will be scaled to 48x48 pixels if necessary)"),
        QString(), imageFilter);
    if (!iconFileName.isEmpty()) {
        QString error;
        if (!m_step->debBasedMaemoTarget()->setPackageManagerIcon(iconFileName, &error))
            QMessageBox::critical(this, tr("Could Not Set New Icon"), error);
    }
}

void MaemoPackageCreationWidget::setPackageName()
{
    if (!m_step->maemoTarget()->setPackageName(m_ui->packageNameLineEdit->text())) {
        QMessageBox::critical(this, tr("File Error"),
            tr("Could not set project name."));
    }
}

void MaemoPackageCreationWidget::setPackageManagerName()
{
    if (!m_step->debBasedMaemoTarget()->setPackageManagerName(m_ui->packageManagerNameLineEdit->text())) {
        QMessageBox::critical(this, tr("File Error"),
            tr("Could not set package name for project manager."));
    }
}

void MaemoPackageCreationWidget::setShortDescription()
{
    if (!m_step->maemoTarget()->setShortDescription(m_ui->shortDescriptionLineEdit->text())) {
        QMessageBox::critical(this, tr("File Error"),
            tr("Could not set project description."));
    }
}

QString MaemoPackageCreationWidget::summaryText() const
{
    return tr("<b>Create Package:</b> ")
        + QDir::toNativeSeparators(m_step->packageFilePath());
}

QString MaemoPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

void MaemoPackageCreationWidget::versionInfoChanged()
{
    QString error;
    const bool success = m_step->setVersionString(m_ui->major->text()
        + QLatin1Char('.') + m_ui->minor->text() + QLatin1Char('.')
        + m_ui->patch->text(), &error);
    if (!success) {
        QMessageBox::critical(this, tr("Could Not Set Version Number"), error);
        updateVersionInfo();
    }
}

void MaemoPackageCreationWidget::editDebianFile()
{
    editFile(m_step->debBasedMaemoTarget()->debianDirPath()
        + QLatin1Char('/') + m_ui->debianFilesComboBox->currentText());
}

void MaemoPackageCreationWidget::editSpecFile()
{
    editFile(m_step->rpmBasedMaemoTarget()->specFilePath());
}

void MaemoPackageCreationWidget::editFile(const QString &filePath)
{
    Core::EditorManager::instance()->openEditor(filePath, QString(),
        Core::EditorManager::ModeSwitch);
}

} // namespace Internal
} // namespace RemoteLinux
