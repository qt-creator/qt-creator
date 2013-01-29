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

#include "bardescriptoreditorwidget.h"
#include "ui_bardescriptoreditorwidget.h"

#include "qnxconstants.h"
#include "bardescriptoreditor.h"
#include "bardescriptorpermissionsmodel.h"

#include <qtsupport/qtversionmanager.h>
#include <texteditor/plaintexteditor.h>
#include <utils/qtcassert.h>

#include <QFileDialog>
#include <QItemSelection>
#include <QStandardItemModel>
#include <QStringListModel>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
void setTextBlocked(QLineEdit *lineEdit, const QString &value)
{
    bool blocked = lineEdit->blockSignals(true);
    lineEdit->setText(value);
    lineEdit->blockSignals(blocked);
}

void setComboBoxDataBlocked(QComboBox *comboBox, const QString &data)
{
    int index = comboBox->findData(data);
    QTC_CHECK(index > -1);
    bool blocked = comboBox->blockSignals(true);
    comboBox->setCurrentIndex(index);
    comboBox->blockSignals(blocked);
}

void setPathBlocked(Utils::PathChooser *pathChooser, const QString &path)
{
    bool blocked = pathChooser->blockSignals(true);
    pathChooser->setPath(path);
    pathChooser->blockSignals(blocked);
}

void setCheckBoxBlocked(QCheckBox *checkBox, bool check)
{
    bool blocked = checkBox->blockSignals(true);
    checkBox->setChecked(check);
    checkBox->blockSignals(blocked);
}
}

BarDescriptorEditorWidget::BarDescriptorEditorWidget(QWidget *parent)
    : QStackedWidget(parent)
    , m_editor(0)
    , m_dirty(false)
    , m_ui(new Ui::BarDescriptorEditorWidget)
{
    m_ui->setupUi(this);

    setCurrentIndex(0);

    initGeneralPage();
    initApplicationPage();
    initAssetsPage();
    initSourcePage();
}

BarDescriptorEditorWidget::~BarDescriptorEditorWidget()
{
    delete m_ui;
}

void BarDescriptorEditorWidget::initGeneralPage()
{
    QRegExp versionNumberRegExp(QLatin1String("(\\d{1,3}\\.)?(\\d{1,3}\\.)?(\\d{1,3})"));
    QRegExpValidator *versionNumberValidator = new QRegExpValidator(versionNumberRegExp, this);
    m_ui->packageVersion->setValidator(versionNumberValidator);

    connect(m_ui->packageId, SIGNAL(textChanged(QString)), this, SLOT(setDirty()));
    connect(m_ui->packageVersion, SIGNAL(textChanged(QString)), this, SLOT(setDirty()));
    connect(m_ui->packageBuildId, SIGNAL(textChanged(QString)), this, SLOT(setDirty()));

    connect(m_ui->author, SIGNAL(textChanged(QString)), this, SLOT(setDirty()));
    connect(m_ui->authorId, SIGNAL(textChanged(QString)), this, SLOT(setDirty()));
}

void BarDescriptorEditorWidget::clearGeneralPage()
{
    setTextBlocked(m_ui->packageId, QString());
    setTextBlocked(m_ui->packageVersion, QString());
    setTextBlocked(m_ui->packageBuildId, QString());

    setTextBlocked(m_ui->author, QString());
    setTextBlocked(m_ui->authorId, QString());
}

void BarDescriptorEditorWidget::initApplicationPage()
{
    // General
    m_ui->orientation->addItem(tr("Default"), QLatin1String(""));
    m_ui->orientation->addItem(tr("Auto-orient"), QLatin1String("auto-orient"));
    m_ui->orientation->addItem(tr("Landscape"), QLatin1String("landscape"));
    m_ui->orientation->addItem(tr("Portrait"), QLatin1String("portrait"));

    m_ui->chrome->addItem(tr("Standard"), QLatin1String("standard"));
    m_ui->chrome->addItem(tr("None"), QLatin1String("none"));

    connect(m_ui->orientation, SIGNAL(currentIndexChanged(int)), this, SLOT(setDirty()));
    connect(m_ui->chrome, SIGNAL(currentIndexChanged(int)), this, SLOT(setDirty()));
    connect(m_ui->transparentMainWindow, SIGNAL(toggled(bool)), this, SLOT(setDirty()));
    connect(m_ui->applicationArguments, SIGNAL(textChanged(QString)), this, SLOT(setDirty()));

    //Permissions
    m_permissionsModel = new BarDescriptorPermissionsModel(this);
    m_ui->permissionsView->setModel(m_permissionsModel);

    connect(m_ui->selectAllPermissions, SIGNAL(clicked()), m_permissionsModel, SLOT(checkAll()));
    connect(m_ui->deselectAllPermissions, SIGNAL(clicked()), m_permissionsModel, SLOT(uncheckAll()));
    connect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));

    // Environment
    m_ui->environmentWidget->setBaseEnvironmentText(tr("Device Environment"));

    connect(m_ui->environmentWidget, SIGNAL(userChangesChanged()), this, SLOT(setDirty()));

    // Entry-Point Text and Images
    m_ui->iconFilePath->setExpectedKind(Utils::PathChooser::File);
    m_ui->iconFilePath->setPromptDialogFilter(tr("Images (*.jpg *.png)"));

    connect(m_ui->applicationName, SIGNAL(textChanged(QString)), this, SLOT(setDirty()));
    connect(m_ui->applicationDescription, SIGNAL(textChanged()), this, SLOT(setDirty()));

    connect(m_ui->iconFilePath, SIGNAL(changed(QString)), this, SLOT(setDirty()));
    connect(m_ui->iconFilePath, SIGNAL(changed(QString)), this, SLOT(addImageAsAsset(QString)));
    connect(m_ui->iconFilePath, SIGNAL(changed(QString)), this, SLOT(setApplicationIconPreview(QString)));
    connect(m_ui->iconClearButton, SIGNAL(clicked()), m_ui->iconFilePath->lineEdit(), SLOT(clear()));

    m_splashScreenModel = new QStringListModel(this);
    m_ui->splashScreensView->setModel(m_splashScreenModel);
    connect(m_ui->addSplashScreen, SIGNAL(clicked()), this, SLOT(browseForSplashScreen()));
    connect(m_ui->removeSplashScreen, SIGNAL(clicked()), this, SLOT(removeSelectedSplashScreen()));
    connect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
    connect(m_ui->splashScreensView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(handleSplashScreenSelectionChanged(QItemSelection, QItemSelection)));
}

void BarDescriptorEditorWidget::clearApplicationPage()
{
    // General
    setComboBoxDataBlocked(m_ui->orientation, QLatin1String(""));
    setComboBoxDataBlocked(m_ui->chrome, QLatin1String("none"));
    setCheckBoxBlocked(m_ui->transparentMainWindow, false);
    setTextBlocked(m_ui->applicationArguments, QString());

    // Permissions
    disconnect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
    m_permissionsModel->uncheckAll();
    connect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));

    // Environment
    disconnect(m_ui->environmentWidget, SIGNAL(userChangesChanged()), this, SLOT(setDirty()));
    m_ui->environmentWidget->setUserChanges(QList<Utils::EnvironmentItem>());
    connect(m_ui->environmentWidget, SIGNAL(userChangesChanged()), this, SLOT(setDirty()));

    // Entry-Point Text and Images
    setPathBlocked(m_ui->iconFilePath, QString());
    setApplicationIconPreview(QString());

    disconnect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
    m_splashScreenModel->setStringList(QStringList());
    connect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
    setImagePreview(m_ui->splashScreenPreviewLabel, QString());

}

void BarDescriptorEditorWidget::initAssetsPage()
{
    QStringList headerLabels;
    headerLabels << tr("Path") << tr("Destination") << tr("Entry-Point");
    m_assetsModel = new QStandardItemModel(this);
    m_assetsModel->setHorizontalHeaderLabels(headerLabels);
    m_ui->assets->setModel(m_assetsModel);

    connect(m_ui->addAsset, SIGNAL(clicked()), this, SLOT(addNewAsset()));
    connect(m_ui->removeAsset, SIGNAL(clicked()), this, SLOT(removeSelectedAsset()));
    connect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));
    connect(m_assetsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
}

void BarDescriptorEditorWidget::clearAssetsPage()
{
    disconnect(m_assetsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
    m_assetsModel->removeRows(0, m_assetsModel->rowCount());
    connect(m_assetsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
}

void BarDescriptorEditorWidget::initSourcePage()
{
    m_ui->xmlSourceView->configure(QLatin1String(Constants::QNX_BAR_DESCRIPTOR_MIME_TYPE));
    connect(m_ui->xmlSourceView, SIGNAL(textChanged()), this, SLOT(setDirty()));
}

void BarDescriptorEditorWidget::clearSourcePage()
{
    disconnect(m_ui->xmlSourceView, SIGNAL(textChanged()), this, SLOT(setDirty()));
    m_ui->xmlSourceView->clear();
    connect(m_ui->xmlSourceView, SIGNAL(textChanged()), this, SLOT(setDirty()));
}

Core::IEditor *BarDescriptorEditorWidget::editor() const
{
    if (!m_editor) {
        m_editor = const_cast<BarDescriptorEditorWidget *>(this)->createEditor();
        connect(this, SIGNAL(changed()), m_editor, SIGNAL(changed()));
    }

    return m_editor;
}

QString BarDescriptorEditorWidget::packageId() const
{
    return m_ui->packageId->text();
}

void BarDescriptorEditorWidget::setPackageId(const QString &packageId)
{
    setTextBlocked(m_ui->packageId, packageId);
}

QString BarDescriptorEditorWidget::packageVersion() const
{
    QString version = m_ui->packageVersion->text();
    int pos = 0;
    if (m_ui->packageVersion->validator()->validate(version, pos) == QValidator::Intermediate) {
        if (version.endsWith(QLatin1Char('.')))
            version = version.left(version.size() - 1);
    }
    return version;
}

void BarDescriptorEditorWidget::setPackageVersion(const QString &packageVersion)
{
    setTextBlocked(m_ui->packageVersion, packageVersion);
}

QString BarDescriptorEditorWidget::packageBuildId() const
{
    return m_ui->packageBuildId->text();
}

void BarDescriptorEditorWidget::setPackageBuildId(const QString &packageBuildId)
{
    setTextBlocked(m_ui->packageBuildId, packageBuildId);
}

QString BarDescriptorEditorWidget::author() const
{
    return m_ui->author->text();
}

void BarDescriptorEditorWidget::setAuthor(const QString &author)
{
    setTextBlocked(m_ui->author, author);
}

QString BarDescriptorEditorWidget::authorId() const
{
    return m_ui->authorId->text();
}

void BarDescriptorEditorWidget::setAuthorId(const QString &authorId)
{
    setTextBlocked(m_ui->authorId, authorId);
}

QString BarDescriptorEditorWidget::orientation() const
{
    return m_ui->orientation->itemData(m_ui->orientation->currentIndex()).toString();
}

void BarDescriptorEditorWidget::setOrientation(const QString &orientation)
{
    setComboBoxDataBlocked(m_ui->orientation, orientation);
}

QString BarDescriptorEditorWidget::chrome() const
{
    return m_ui->chrome->itemData(m_ui->chrome->currentIndex()).toString();
}

void BarDescriptorEditorWidget::setChrome(const QString &chrome)
{
    setComboBoxDataBlocked(m_ui->chrome, chrome);
}

bool BarDescriptorEditorWidget::transparent() const
{
    return m_ui->transparentMainWindow->isChecked();
}

void BarDescriptorEditorWidget::setTransparent(bool transparent)
{
    setCheckBoxBlocked(m_ui->transparentMainWindow, transparent);
}

void BarDescriptorEditorWidget::appendApplicationArgument(const QString &argument)
{
    QString completeArguments = m_ui->applicationArguments->text();
    if (!completeArguments.isEmpty())
        completeArguments.append(QLatin1Char(' '));
    completeArguments.append(argument);

    setTextBlocked(m_ui->applicationArguments, completeArguments);
}

QStringList BarDescriptorEditorWidget::applicationArguments() const
{
    // TODO: Should probably handle "argument with spaces within quotes"
    return m_ui->applicationArguments->text().split(QLatin1Char(' '));
}

QStringList BarDescriptorEditorWidget::checkedPermissions() const
{
    return m_permissionsModel->checkedIdentifiers();
}

void BarDescriptorEditorWidget::checkPermission(const QString &identifier)
{
    disconnect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
    m_permissionsModel->checkPermission(identifier);
    connect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
}

QList<Utils::EnvironmentItem> BarDescriptorEditorWidget::environment() const
{
    return m_ui->environmentWidget->userChanges();
}

void BarDescriptorEditorWidget::appendEnvironmentItem(const Utils::EnvironmentItem &envItem)
{
    disconnect(m_ui->environmentWidget, SIGNAL(userChangesChanged()), this, SLOT(setDirty()));
    QList<Utils::EnvironmentItem> items = m_ui->environmentWidget->userChanges();
    items.append(envItem);
    m_ui->environmentWidget->setUserChanges(items);
    connect(m_ui->environmentWidget, SIGNAL(userChangesChanged()), this, SLOT(setDirty()));
}

QString BarDescriptorEditorWidget::applicationName() const
{
    return m_ui->applicationName->text();
}

void BarDescriptorEditorWidget::setApplicationName(const QString &applicationName)
{
    setTextBlocked(m_ui->applicationName, applicationName);
}

QString BarDescriptorEditorWidget::applicationDescription() const
{
    return m_ui->applicationDescription->toPlainText();
}

void BarDescriptorEditorWidget::setApplicationDescription(const QString &applicationDescription)
{
    bool blocked = m_ui->applicationDescription->blockSignals(true);
    m_ui->applicationDescription->setPlainText(applicationDescription);
    m_ui->applicationDescription->blockSignals(blocked);
}

QString BarDescriptorEditorWidget::applicationIconFileName() const
{
    return QFileInfo(m_ui->iconFilePath->path()).fileName();
}

void BarDescriptorEditorWidget::setApplicationIcon(const QString &iconPath)
{
    // During file loading, the assets might not have been read yet
    QMetaObject::invokeMethod(this, "setApplicationIconDelayed", Qt::QueuedConnection, Q_ARG(QString, iconPath));
}

QStringList BarDescriptorEditorWidget::splashScreens() const
{
    QStringList result;

    foreach (const QString &splashScreen, m_splashScreenModel->stringList())
        result << QFileInfo(splashScreen).fileName();

    return result;
}

void BarDescriptorEditorWidget::appendSplashScreen(const QString &splashScreenPath)
{
    // During file loading, the assets might not have been read yet
    QMetaObject::invokeMethod(this, "appendSplashScreenDelayed", Qt::QueuedConnection, Q_ARG(QString, splashScreenPath));
}

void BarDescriptorEditorWidget::setApplicationIconDelayed(const QString &iconPath)
{
    const QString fullIconPath = localAssetPathFromDestination(iconPath);
    setPathBlocked(m_ui->iconFilePath, fullIconPath);
    setApplicationIconPreview(fullIconPath);
}

void BarDescriptorEditorWidget::setImagePreview(QLabel *previewLabel, const QString &path)
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

void BarDescriptorEditorWidget::setApplicationIconPreview(const QString &path)
{
    setImagePreview(m_ui->iconPreviewLabel, path);
}

void BarDescriptorEditorWidget::appendSplashScreenDelayed(const QString &splashScreenPath)
{
    const QString fullSplashScreenPath = localAssetPathFromDestination(splashScreenPath);

    disconnect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
    int rowCount = m_splashScreenModel->rowCount();
    m_splashScreenModel->insertRow(rowCount);
    m_splashScreenModel->setData(m_splashScreenModel->index(rowCount), fullSplashScreenPath);
    connect(m_splashScreenModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setDirty()));
}

void BarDescriptorEditorWidget::browseForSplashScreen()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select Splash Screen"), QString(), tr("Images (*.jpg *.png)"));
    if (fileName.isEmpty())
        return;

    if (m_splashScreenModel->stringList().contains(fileName))
        return;

    int rowCount = m_splashScreenModel->rowCount();
    m_splashScreenModel->insertRow(rowCount);
    m_splashScreenModel->setData(m_splashScreenModel->index(rowCount), fileName);
    addImageAsAsset(fileName);
}

void BarDescriptorEditorWidget::removeSelectedSplashScreen()
{
    QModelIndexList selectedIndexes = m_ui->splashScreensView->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return;

    foreach (const QModelIndex &index, selectedIndexes) {
        QString path = m_splashScreenModel->data(index, Qt::DisplayRole).toString();

        QList<QStandardItem*> assetItems = m_assetsModel->findItems(path);
        foreach (QStandardItem *assetItem, assetItems) {
            QList<QStandardItem*> assetRow = m_assetsModel->takeRow(assetItem->row());
            while (!assetRow.isEmpty())
                delete assetRow.takeLast();
        }

        m_splashScreenModel->removeRow(index.row());
    }
}

void BarDescriptorEditorWidget::handleSplashScreenSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    const bool emptySelection = selected.indexes().isEmpty();
    m_ui->removeSplashScreen->setEnabled(!emptySelection);

    if (!emptySelection) {
        QString path = m_splashScreenModel->data(selected.indexes().at(0), Qt::DisplayRole).toString();
        setImagePreview(m_ui->splashScreenPreviewLabel, path);
    } else {
        setImagePreview(m_ui->splashScreenPreviewLabel, QString());
    }
}

void BarDescriptorEditorWidget::addAsset(const BarDescriptorAsset &asset)
{
    const QString path = asset.source;
    const QString dest = asset.destination;
    QTC_ASSERT(!path.isEmpty(), return);
    QTC_ASSERT(!dest.isEmpty(), return);

    if (hasAsset(asset))
        return;

    disconnect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));

    QList<QStandardItem *> items;
    items << new QStandardItem(path);
    items << new QStandardItem(dest);

    QStandardItem *entryItem = new QStandardItem();
    entryItem->setCheckable(true);
    entryItem->setCheckState(asset.entry ? Qt::Checked : Qt::Unchecked);
    items << entryItem;
    m_assetsModel->appendRow(items);

    connect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));
}

bool BarDescriptorEditorWidget::hasAsset(const BarDescriptorAsset &asset)
{
    // TODO: Move this to a specific BarDescriptorAssetModel
    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        QStandardItem *sourceItem = m_assetsModel->item(i, 0);
        QStandardItem *destItem = m_assetsModel->item(i, 1);
        if (sourceItem->text() == asset.source && destItem->text() == asset.destination)
            return true;
    }

    return false;
}

QString BarDescriptorEditorWidget::localAssetPathFromDestination(const QString &destination)
{
    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        QStandardItem *destItem = m_assetsModel->item(i, 1);
        if (destItem->text() == destination)
            return m_assetsModel->item(i, 0)->text();
    }

    return QString();
}

QList<BarDescriptorAsset> BarDescriptorEditorWidget::assets() const
{
    QList<BarDescriptorAsset> result;

    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        BarDescriptorAsset asset;
        asset.source = m_assetsModel->item(i, 0)->text();
        asset.destination = m_assetsModel->item(i, 1)->text();
        asset.entry = m_assetsModel->item(i, 2)->checkState() == Qt::Checked;
        result << asset;
    }

    return result;
}

QString BarDescriptorEditorWidget::xmlSource() const
{
    return m_ui->xmlSourceView->toPlainText();
}

void BarDescriptorEditorWidget::setXmlSource(const QString &xmlSource)
{
    disconnect(m_ui->xmlSourceView, SIGNAL(textChanged()), this, SLOT(setDirty()));
    m_ui->xmlSourceView->setPlainText(xmlSource);
    connect(m_ui->xmlSourceView, SIGNAL(textChanged()), this, SLOT(setDirty()));
}

bool BarDescriptorEditorWidget::isDirty() const
{
    return m_dirty;
}

void BarDescriptorEditorWidget::clear()
{
    clearGeneralPage();
    clearApplicationPage();
    clearAssetsPage();
    clearSourcePage();
}

void BarDescriptorEditorWidget::setDirty(bool dirty)
{
    m_dirty = dirty;
    emit changed();
}

BarDescriptorEditor *BarDescriptorEditorWidget::createEditor()
{
    return new BarDescriptorEditor(this);
}

void BarDescriptorEditorWidget::addNewAsset()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select File to Add"));
    if (fileName.isEmpty())
        return;

    QFileInfo fi(fileName);
    BarDescriptorAsset asset;
    asset.source = fileName;
    asset.destination = fi.fileName();
    asset.entry = false; // TODO
    addAsset(asset);
}

void BarDescriptorEditorWidget::removeSelectedAsset()
{
    QModelIndexList selectedIndexes = m_ui->assets->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return;

    foreach (const QModelIndex &index, selectedIndexes)
        m_assetsModel->removeRow(index.row());
}

void BarDescriptorEditorWidget::updateEntryCheckState(QStandardItem *item)
{
    if (item->column() != 2 || item->checkState() == Qt::Unchecked)
        return;

    disconnect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));
    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        QStandardItem *other = m_assetsModel->item(i, 2);
        if (other == item)
            continue;

        // Only one asset can be the entry point
        other->setCheckState(Qt::Unchecked);
    }
    connect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));
}

void BarDescriptorEditorWidget::addImageAsAsset(const QString &path)
{
    if (path.isEmpty())
        return;

    BarDescriptorAsset asset;
    asset.source = path;
    asset.destination = QFileInfo(path).fileName();
    asset.entry = false;
    addAsset(asset);
}
