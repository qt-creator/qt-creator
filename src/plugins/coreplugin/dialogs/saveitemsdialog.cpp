/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "saveitemsdialog.h"

#include <coreplugin/diffservice.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/idocument.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <extensionsystem/pluginmanager.h>

#include <QDir>
#include <QFileInfo>
#include <QPushButton>
#include <QDebug>

Q_DECLARE_METATYPE(Core::IDocument*)

using namespace Core;
using namespace Core::Internal;

SaveItemsDialog::SaveItemsDialog(QWidget *parent,
                                 QList<IDocument *> items)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // QDialogButtonBox's behavior for "destructive" is wrong, the "do not save" should be left-aligned
    const QDialogButtonBox::ButtonRole discardButtonRole = Utils::HostOsInfo::isMacHost()
            ? QDialogButtonBox::ResetRole : QDialogButtonBox::DestructiveRole;

    if (ExtensionSystem::PluginManager::getObject<Core::DiffService>()) {
        m_diffButton = m_ui.buttonBox->addButton(tr("&Diff"), discardButtonRole);
        connect(m_diffButton, &QAbstractButton::clicked, this, &SaveItemsDialog::collectFilesToDiff);
    }

    QPushButton *discardButton = m_ui.buttonBox->addButton(tr("Do &not Save"), discardButtonRole);
    m_ui.buttonBox->button(QDialogButtonBox::Save)->setDefault(true);
    m_ui.treeWidget->setFocus();

    m_ui.saveBeforeBuildCheckBox->setVisible(false);

    foreach (IDocument *document, items) {
        QString visibleName;
        QString directory;
        QString fileName = document->filePath().toString();
        if (fileName.isEmpty()) {
            visibleName = document->fallbackSaveAsFileName();
        } else {
            QFileInfo info = QFileInfo(fileName);
            directory = info.absolutePath();
            visibleName = info.fileName();
        }
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui.treeWidget, QStringList()
                                                    << visibleName << QDir::toNativeSeparators(directory));
        if (!fileName.isEmpty())
            item->setIcon(0, FileIconProvider::icon(fileName));
        item->setData(0, Qt::UserRole, qVariantFromValue(document));
    }

    m_ui.treeWidget->resizeColumnToContents(0);
    m_ui.treeWidget->selectAll();
    if (Utils::HostOsInfo::isMacHost())
        m_ui.treeWidget->setAlternatingRowColors(true);
    adjustButtonWidths();
    updateButtons();

    connect(m_ui.buttonBox->button(QDialogButtonBox::Save), &QAbstractButton::clicked,
            this, &SaveItemsDialog::collectItemsToSave);
    connect(discardButton, &QAbstractButton::clicked, this, &SaveItemsDialog::discardAll);
    connect(m_ui.treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &SaveItemsDialog::updateButtons);
}

void SaveItemsDialog::setMessage(const QString &msg)
{
    m_ui.msgLabel->setText(msg);
}

void SaveItemsDialog::updateButtons()
{
    int count = m_ui.treeWidget->selectedItems().count();
    QPushButton *saveButton = m_ui.buttonBox->button(QDialogButtonBox::Save);
    bool buttonsEnabled = true;
    QString saveText = tr("&Save");
    QString diffText = tr("&Diff && Cancel");
    if (count == m_ui.treeWidget->topLevelItemCount()) {
        saveText = tr("&Save All");
        diffText = tr("&Diff All && Cancel");
    } else if (count == 0) {
        buttonsEnabled = false;
    } else {
        saveText = tr("&Save Selected");
        diffText = tr("&Diff Selected && Cancel");
    }
    saveButton->setEnabled(buttonsEnabled);
    saveButton->setText(saveText);
    if (m_diffButton) {
        m_diffButton->setEnabled(buttonsEnabled);
        m_diffButton->setText(diffText);
    }
}

void SaveItemsDialog::adjustButtonWidths()
{
    // give save button a size that all texts fit in, so it doesn't get resized
    // Mac: make cancel + save button same size (work around dialog button box issue)
    QStringList possibleTexts;
    possibleTexts << tr("Save") << tr("Save All");
    if (m_ui.treeWidget->topLevelItemCount() > 1)
        possibleTexts << tr("Save Selected");
    int maxTextWidth = 0;
    QPushButton *saveButton = m_ui.buttonBox->button(QDialogButtonBox::Save);
    foreach (const QString &text, possibleTexts) {
        saveButton->setText(text);
        int hint = saveButton->sizeHint().width();
        if (hint > maxTextWidth)
            maxTextWidth = hint;
    }
    if (Utils::HostOsInfo::isMacHost()) {
        QPushButton *cancelButton = m_ui.buttonBox->button(QDialogButtonBox::Cancel);
        int cancelButtonWidth = cancelButton->sizeHint().width();
        if (cancelButtonWidth > maxTextWidth)
            maxTextWidth = cancelButtonWidth;
        cancelButton->setMinimumWidth(maxTextWidth);
    }
    saveButton->setMinimumWidth(maxTextWidth);
}

void SaveItemsDialog::collectItemsToSave()
{
    m_itemsToSave.clear();
    foreach (QTreeWidgetItem *item, m_ui.treeWidget->selectedItems()) {
        m_itemsToSave.append(item->data(0, Qt::UserRole).value<IDocument*>());
    }
    accept();
}

void SaveItemsDialog::collectFilesToDiff()
{
    m_filesToDiff.clear();
    foreach (QTreeWidgetItem *item, m_ui.treeWidget->selectedItems()) {
        if (IDocument *doc = item->data(0, Qt::UserRole).value<IDocument*>())
            m_filesToDiff.append(doc->filePath().toString());
    }
    reject();
}

void SaveItemsDialog::discardAll()
{
    m_ui.treeWidget->clearSelection();
    collectItemsToSave();
}

QList<IDocument*> SaveItemsDialog::itemsToSave() const
{
    return m_itemsToSave;
}

QStringList SaveItemsDialog::filesToDiff() const
{
    return m_filesToDiff;
}

void SaveItemsDialog::setAlwaysSaveMessage(const QString &msg)
{
    m_ui.saveBeforeBuildCheckBox->setText(msg);
    m_ui.saveBeforeBuildCheckBox->setVisible(true);
}

bool SaveItemsDialog::alwaysSaveChecked()
{
    return m_ui.saveBeforeBuildCheckBox->isChecked();
}
