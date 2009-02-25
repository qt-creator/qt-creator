/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "saveitemsdialog.h"
#include "mainwindow.h"
#include "vcsmanager.h"

#include <coreplugin/ifile.h>

#include <QtCore/QFileInfo>
#include <QtGui/QPushButton>
#include <QtGui/QTreeWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>

using namespace Core;
using namespace Core::Internal;

FileItem::FileItem(QTreeWidget *tree, bool supportOpen, bool open, const QString &text)
    : QTreeWidgetItem(tree)
{
    m_saveCheckBox = createCheckBox(tree, 0);
    m_saveCheckBox->setChecked(true);

    QFileInfo fi(text);
    QString name = fi.fileName();
    if (open)
        name.append(tr(" [ReadOnly]"));

    if (supportOpen) {
        m_sccCheckBox = createCheckBox(tree, 1);
        m_sccCheckBox->setEnabled(open);
        m_sccCheckBox->setChecked(open);
        connect(m_saveCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(updateSCCCheckBox()));
        setText(2, name);
        setToolTip(2, text);
    } else {
        m_sccCheckBox = 0;
        setText(2, name);
        setToolTip(2, text);
    }
}

QCheckBox *FileItem::createCheckBox(QTreeWidget *tree, int column)
{
    QWidget *w = new QWidget();
    QHBoxLayout *l = new QHBoxLayout(w);
    l->setMargin(0);
    l->setSpacing(0);
    QCheckBox *box = new QCheckBox(w);
    l->addWidget(box);
    l->setAlignment(box, Qt::AlignCenter);
    w->setLayout(l);
    tree->setItemWidget(this, column, w);
    return box;
}

void FileItem::updateSCCCheckBox()
{
    if (!m_saveCheckBox->isChecked()) {
        m_sccCheckBox->setEnabled(false);
        m_sccCheckBox->setChecked(false);
    } else {
        m_sccCheckBox->setEnabled(true);
    }
}

bool FileItem::shouldBeSaved() const
{
    return m_saveCheckBox->isChecked();
}

void FileItem::setShouldBeSaved(bool s)
{
    m_saveCheckBox->setChecked(s);
}

bool FileItem::shouldBeOpened() const
{
    if (m_sccCheckBox)
        return m_sccCheckBox->isChecked();
    return false;
}



SaveItemsDialog::SaveItemsDialog(MainWindow *mainWindow,
                                 QMap<IFile*, QString> items)
    : QDialog(mainWindow)
{
    m_ui.setupUi(this);
    QPushButton *uncheckButton = m_ui.buttonBox->addButton(tr("Uncheck All"),
                                                           QDialogButtonBox::ActionRole);
    QPushButton *discardButton = m_ui.buttonBox->addButton(tr("Discard All"),
                                                           QDialogButtonBox::DestructiveRole);
    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setFocus(Qt::TabFocusReason);

    m_ui.treeWidget->header()->setMovable(false);
    m_ui.treeWidget->setRootIsDecorated(false);
    m_ui.treeWidget->setColumnCount(2);

    QStringList headers;
    headers << tr("Save") << tr("File Name");

    const bool hasVersionControl = true;
    if (hasVersionControl) {
        m_ui.treeWidget->setColumnCount(3);
        headers.insert(1, tr("Open with SCC"));
    }
    m_ui.treeWidget->setHeaderLabels(headers);

    FileItem *itm;
    QMap<IFile*, QString>::const_iterator it = items.constBegin();
    while (it != items.constEnd()) {
        QString directory = QFileInfo(it.key()->fileName()).absolutePath();
        bool fileHasVersionControl = mainWindow->vcsManager()->findVersionControlForDirectory(directory) != 0;
        itm = new FileItem(m_ui.treeWidget, fileHasVersionControl,
            it.key()->isReadOnly(), it.value());
        m_itemMap.insert(itm, it.key());
        ++it;
    }

    m_ui.treeWidget->resizeColumnToContents(0);
    if (hasVersionControl)
        m_ui.treeWidget->resizeColumnToContents(1);

    connect(m_ui.buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
            this, SLOT(collectItemsToSave()));
    connect(uncheckButton, SIGNAL(clicked()), this, SLOT(uncheckAll()));
    connect(discardButton, SIGNAL(clicked()), this, SLOT(discardAll()));
}

void SaveItemsDialog::setMessage(const QString &msg)
{
    m_ui.msgLabel->setText(msg);
}

void SaveItemsDialog::collectItemsToSave()
{
    m_itemsToSave.clear();
    m_itemsToOpen.clear();
    QMap<FileItem*, IFile*>::const_iterator it = m_itemMap.constBegin();
    while (it != m_itemMap.constEnd()) {
        if (it.key()->shouldBeSaved())
            m_itemsToSave << it.value();
        if (it.key()->shouldBeOpened())
            m_itemsToOpen.insert(it.value());
        ++it;
    }
    accept();
}

void SaveItemsDialog::discardAll()
{
    uncheckAll();
    collectItemsToSave();
}

QList<IFile*> SaveItemsDialog::itemsToSave() const
{
    return m_itemsToSave;
}

QSet<IFile*> SaveItemsDialog::itemsToOpen() const
{
    return m_itemsToOpen;
}

void SaveItemsDialog::uncheckAll()
{
    for (int i=0; i<m_ui.treeWidget->topLevelItemCount(); ++i) {
        FileItem *item = static_cast<FileItem*>(m_ui.treeWidget->topLevelItem(i));
        item->setShouldBeSaved(false);
    }
}
