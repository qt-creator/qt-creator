/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditorentrypointwidget.h"
#include "ui_bardescriptoreditorentrypointwidget.h"

#include <QFileDialog>
#include <QStringListModel>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
// Recommended maximum size for icons according to
// http://developer.blackberry.com/native/documentation/bb10/com.qnx.doc.native_sdk.devguide/com.qnx.doc.native_sdk.devguide/topic/r_barfile_dtd_ref_image.html
static int AppIconMaxWidth = 114;
static int AppIconMaxHeight = 114;

// Recommended maximum size for splashscreens according to
// http://developer.blackberry.com/native/documentation/bb10/com.qnx.doc.native_sdk.devguide/com.qnx.doc.native_sdk.devguide/topic/r_barfile_dtd_ref_splashscreens.html
static int SplashScreenMaxWidth = 1280;
static int SplashScreenMaxHeight = 1280;
}

BarDescriptorEditorEntryPointWidget::BarDescriptorEditorEntryPointWidget(QWidget *parent) :
    BarDescriptorEditorAbstractPanelWidget(parent),
    m_ui(new Ui::BarDescriptorEditorEntryPointWidget)
{
    m_ui->setupUi(this);

    m_ui->iconFilePath->setExpectedKind(Utils::PathChooser::File);
    m_ui->iconFilePath->setPromptDialogFilter(tr("Images (*.jpg *.png)"));

    m_ui->iconWarningLabel->setVisible(false);
    m_ui->iconWarningPixmap->setVisible(false);

    m_ui->splashScreenWarningLabel->setVisible(false);
    m_ui->splashScreenWarningPixmap->setVisible(false);

    connect(m_ui->applicationName, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
    connect(m_ui->applicationDescription, SIGNAL(textChanged()), this, SIGNAL(changed()));

    connect(m_ui->iconFilePath, SIGNAL(changed(QString)), this, SLOT(handleIconChanged(QString)));
    connect(m_ui->iconClearButton, SIGNAL(clicked()), this, SLOT(clearIcon()));

    m_splashScreenModel = new QStringListModel(this);
    m_ui->splashScreensView->setModel(m_splashScreenModel);
    connect(m_ui->addSplashScreen, SIGNAL(clicked()), this, SLOT(browseForSplashScreen()));
    connect(m_ui->removeSplashScreen, SIGNAL(clicked()), this, SLOT(removeSelectedSplashScreen()));
    connect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    connect(m_ui->splashScreensView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(handleSplashScreenSelectionChanged(QItemSelection,QItemSelection)));
}

BarDescriptorEditorEntryPointWidget::~BarDescriptorEditorEntryPointWidget()
{
    delete m_ui;
}

void BarDescriptorEditorEntryPointWidget::clear()
{
    setPathChooserBlocked(m_ui->iconFilePath, QString());
    setApplicationIconPreview(QString());

    disconnect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    m_splashScreenModel->setStringList(QStringList());
    connect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    setImagePreview(m_ui->splashScreenPreviewLabel, QString());
}

QString BarDescriptorEditorEntryPointWidget::applicationName() const
{
    return m_ui->applicationName->text();
}

void BarDescriptorEditorEntryPointWidget::setApplicationName(const QString &applicationName)
{
    setLineEditBlocked(m_ui->applicationName, applicationName);
}

QString BarDescriptorEditorEntryPointWidget::applicationDescription() const
{
    return m_ui->applicationDescription->toPlainText();
}

void BarDescriptorEditorEntryPointWidget::setApplicationDescription(const QString &applicationDescription)
{
    setTextEditBlocked(m_ui->applicationDescription, applicationDescription);
}

QString BarDescriptorEditorEntryPointWidget::applicationIconFileName() const
{
    return QFileInfo(m_ui->iconFilePath->path()).fileName();
}

void BarDescriptorEditorEntryPointWidget::setApplicationIcon(const QString &iconPath)
{
    // During file loading, the assets might not have been read yet
    QMetaObject::invokeMethod(this, "setApplicationIconDelayed", Qt::QueuedConnection, Q_ARG(QString, iconPath));
}

QStringList BarDescriptorEditorEntryPointWidget::splashScreens() const
{
    QStringList result;

    foreach (const QString &splashScreen, m_splashScreenModel->stringList())
        result << QFileInfo(splashScreen).fileName();

    return result;
}

void BarDescriptorEditorEntryPointWidget::appendSplashScreen(const QString &splashScreenPath)
{
    QMetaObject::invokeMethod(this, "appendSplashScreenDelayed", Qt::QueuedConnection, Q_ARG(QString, splashScreenPath));
}

void BarDescriptorEditorEntryPointWidget::setAssetsModel(QStandardItemModel *assetsModel)
{
    m_assetsModel = QWeakPointer<QStandardItemModel>(assetsModel);
}

void BarDescriptorEditorEntryPointWidget::setApplicationIconPreview(const QString &path)
{
    setImagePreview(m_ui->iconPreviewLabel, path);
}

void BarDescriptorEditorEntryPointWidget::validateIconSize(const QString &path)
{
    validateImage(path, m_ui->iconWarningLabel, m_ui->iconWarningPixmap, QSize(AppIconMaxWidth, AppIconMaxHeight));
}

void BarDescriptorEditorEntryPointWidget::handleIconChanged(const QString &path)
{
    if (path == m_prevIconPath)
        return;

    setApplicationIconPreview(path);
    validateIconSize(path);

    emit changed();
    emit imageRemoved(m_prevIconPath);

    m_prevIconPath = path;
    if (QFileInfo(path).exists())
        emit imageAdded(path);
}

void BarDescriptorEditorEntryPointWidget::clearIcon()
{
    m_ui->iconFilePath->setPath(QString());
}

void BarDescriptorEditorEntryPointWidget::browseForSplashScreen()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select Splash Screen"), QString(), tr("Images (*.jpg *.png)"));
    if (fileName.isEmpty())
        return;

    if (m_splashScreenModel->stringList().contains(fileName))
        return;

    int rowCount = m_splashScreenModel->rowCount();
    m_splashScreenModel->insertRow(rowCount);
    m_splashScreenModel->setData(m_splashScreenModel->index(rowCount), fileName);
    emit imageAdded(fileName);
}

void BarDescriptorEditorEntryPointWidget::removeSelectedSplashScreen()
{
    QModelIndexList selectedIndexes = m_ui->splashScreensView->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return;

    foreach (const QModelIndex &index, selectedIndexes) {
        QString path = m_splashScreenModel->data(index, Qt::DisplayRole).toString();
        emit imageRemoved(path);

        m_splashScreenModel->removeRow(index.row());
    }
}

void BarDescriptorEditorEntryPointWidget::handleSplashScreenSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    const bool emptySelection = selected.indexes().isEmpty();
    m_ui->removeSplashScreen->setEnabled(!emptySelection);

    if (!emptySelection) {
        QString path = m_splashScreenModel->data(selected.indexes().at(0), Qt::DisplayRole).toString();
        setImagePreview(m_ui->splashScreenPreviewLabel, path);
        validateSplashScreenSize(path);
    } else {
        setImagePreview(m_ui->splashScreenPreviewLabel, QString());
        m_ui->splashScreenWarningLabel->setVisible(false);
        m_ui->splashScreenWarningPixmap->setVisible(false);
    }
}

void BarDescriptorEditorEntryPointWidget::appendSplashScreenDelayed(const QString &splashScreenPath)
{
    const QString fullSplashScreenPath = localAssetPathFromDestination(splashScreenPath);
    if (fullSplashScreenPath.isEmpty())
        return;

    disconnect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    int rowCount = m_splashScreenModel->rowCount();
    m_splashScreenModel->insertRow(rowCount);
    m_splashScreenModel->setData(m_splashScreenModel->index(rowCount), fullSplashScreenPath);
    connect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
}

void BarDescriptorEditorEntryPointWidget::setImagePreview(QLabel *previewLabel, const QString &path)
{
    if (path.isEmpty()) {
        previewLabel->clear();
        return;
    }

    QPixmap originalPixmap(path);
    if (originalPixmap.isNull()) {
        previewLabel->clear();
        return;
    }

    QSize size = previewLabel->minimumSize();
    QPixmap scaledPixmap = originalPixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (scaledPixmap.isNull()) {
        previewLabel->clear();
        return;
    }

    previewLabel->setPixmap(scaledPixmap);
}

void BarDescriptorEditorEntryPointWidget::validateSplashScreenSize(const QString &path)
{
    validateImage(path, m_ui->splashScreenWarningLabel, m_ui->splashScreenWarningPixmap, QSize(SplashScreenMaxWidth, SplashScreenMaxHeight));
}

void BarDescriptorEditorEntryPointWidget::validateImage(const QString &path, QLabel *warningMessage, QLabel *warningPixmap, const QSize &maximumSize)
{
    ImageValidationResult result = Valid;

    QSize actualSize;
    if (!path.isEmpty()) {
        QImage img(path);
        if (img.isNull()) {
            result = CouldNotLoad;
        } else {
            actualSize = img.size();
            if (actualSize.width() > maximumSize.width() || actualSize.height() > maximumSize.height())
                result = IncorrectSize;
        }
    }

    switch (result) {
    case CouldNotLoad:
        warningMessage->setText(tr("<font color=\"red\">Could not open '%1' for reading.</font>").arg(path));
        warningMessage->setVisible(true);
        warningPixmap->setVisible(true);
        break;
    case IncorrectSize: {
        warningMessage->setText(tr("<font color=\"red\">The selected image is too big (%1x%2). The maximum size is %3x%4 pixels.</font>")
                                .arg(actualSize.width()).arg(actualSize.height())
                                .arg(maximumSize.width()).arg(maximumSize.height()));
        warningMessage->setVisible(true);
        warningPixmap->setVisible(true);
        break;
    }
    case Valid:
    default:
        warningMessage->setVisible(false);
        warningPixmap->setVisible(false);
        break;
    }
}

void BarDescriptorEditorEntryPointWidget::setApplicationIconDelayed(const QString &iconPath)
{
    const QString fullIconPath = localAssetPathFromDestination(iconPath);
    if (fullIconPath.isEmpty())
        return;

    setPathChooserBlocked(m_ui->iconFilePath, fullIconPath);
    setApplicationIconPreview(fullIconPath);
    validateIconSize(fullIconPath);
}

QString BarDescriptorEditorEntryPointWidget::localAssetPathFromDestination(const QString &destination)
{
    if (!m_assetsModel)
        return QString();

    for (int i = 0; i < m_assetsModel.data()->rowCount(); ++i) {
        QStandardItem *destItem = m_assetsModel.data()->item(i, 1);
        if (destItem->text() == destination)
            return m_assetsModel.data()->item(i, 0)->text();
    }

    return QString();
}
