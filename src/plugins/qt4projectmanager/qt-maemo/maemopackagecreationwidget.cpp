/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationWidget::MaemoPackageCreationWidget(MaemoPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::MaemoPackageCreationWidget)
{
    m_ui->setupUi(this);
    m_ui->skipCheckBox->setChecked(!m_step->isPackagingEnabled());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QTimer::singleShot(0, this, SLOT(initGui()));
}

MaemoPackageCreationWidget::~MaemoPackageCreationWidget()
{
    delete m_ui;
}

void MaemoPackageCreationWidget::init()
{
}

void MaemoPackageCreationWidget::initGui()
{
    const Qt4BuildConfiguration * const bc = m_step->qt4BuildConfiguration();
    if (bc) {
        m_ui->skipCheckBox->setVisible(m_step->maemoTarget()->allowsPackagingDisabling());
        m_ui->skipCheckBox->setChecked(!m_step->isPackagingEnabled());
    }

    updateDebianFileList();
    updateVersionInfo();
    handleControlFileUpdate();
    connect(m_step, SIGNAL(packageFilePathChanged()), this,
        SIGNAL(updateSummary()));
    versionInfoChanged();
    const AbstractDebBasedQt4MaemoTarget * const debBasedMaemoTarget
        = m_step->debBasedMaemoTarget();
    if (debBasedMaemoTarget) {
        connect(debBasedMaemoTarget, SIGNAL(debianDirContentsChanged()),
            SLOT(updateDebianFileList()));
        connect(debBasedMaemoTarget, SIGNAL(changeLogChanged()),
            SLOT(updateVersionInfo()));
        connect(debBasedMaemoTarget, SIGNAL(controlChanged()),
            SLOT(handleControlFileUpdate()));
    } else {
        // TODO: Connect the respective signals for RPM-based target
    }
    connect(m_ui->nameLineEdit, SIGNAL(editingFinished()), SLOT(setName()));
    m_ui->shortDescriptionLineEdit->setMaxLength(60);
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
        versionString = MaemoPackageCreationStep::DefaultVersionNumber;
    }
    const QStringList list = versionString.split(QLatin1Char('.'),
        QString::SkipEmptyParts);
    m_ui->major->setValue(list.value(0, QLatin1String("0")).toInt());
    m_ui->minor->setValue(list.value(1, QLatin1String("0")).toInt());
    m_ui->patch->setValue(list.value(2, QLatin1String("0")).toInt());
}

void MaemoPackageCreationWidget::handleControlFileUpdate()
{
    updatePackageManagerIcon();
    updateName();
    updateShortDescription();
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

void MaemoPackageCreationWidget::updateName()
{
    m_ui->nameLineEdit->setText(m_step->maemoTarget()->name());
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

void MaemoPackageCreationWidget::setName()
{
    if (!m_step->maemoTarget()->setName(m_ui->nameLineEdit->text())) {
        QMessageBox::critical(this, tr("File Error"),
            tr("Could not set project name."));
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
    const QString constantString = tr("<b>Create Package:</b> ");
    const QString dynamicString = m_step->isPackagingEnabled()
        ? QDir::toNativeSeparators(m_step->packageFilePath())
        : tr("(Packaging disabled)");
    return constantString + dynamicString;
}

QString MaemoPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

void MaemoPackageCreationWidget::handleSkipButtonToggled(bool checked)
{
    m_ui->major->setEnabled(!checked);
    m_ui->minor->setEnabled(!checked);
    m_ui->patch->setEnabled(!checked);
    m_ui->debianFilesComboBox->setEnabled(!checked);
    m_ui->editDebianFileButton->setEnabled(!checked);
    m_ui->packageManagerIconButton->setEnabled(!checked);
    m_ui->nameLineEdit->setEnabled(!checked);
    m_ui->shortDescriptionLineEdit->setEnabled(!checked);
    m_step->setPackagingEnabled(!checked);
    emit updateSummary();
}

void MaemoPackageCreationWidget::versionInfoChanged()
{
    QString error;
    const bool success = m_step->setVersionString(m_ui->major->text()
        + QLatin1Char('.') + m_ui->minor->text() + QLatin1Char('.')
        + m_ui->patch->text(), &error);
    if (!success)
        QMessageBox::critical(this, tr("Could Not Set Version Number"), error);
}

void MaemoPackageCreationWidget::editDebianFile()
{
    const QString debianFilePath = m_step->debBasedMaemoTarget()->debianDirPath()
        + QLatin1Char('/') + m_ui->debianFilesComboBox->currentText();
    Core::EditorManager::instance()->openEditor(debianFilePath, QString(),
        Core::EditorManager::ModeSwitch);
}

} // namespace Internal
} // namespace Qt4ProjectManager
