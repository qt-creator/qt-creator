/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qtgradientview.h"
#include "qtgradientmanager.h"
#include "qtgradientdialog.h"
#include "qtgradientutils.h"
#include <QtGui/QPainter>
#include <QtGui/QMessageBox>
#include <QtGui/QClipboard>

QT_BEGIN_NAMESPACE

void QtGradientView::slotGradientAdded(const QString &id, const QGradient &gradient)
{
    QListWidgetItem *item = new QListWidgetItem(QtGradientUtils::gradientPixmap(gradient), id, m_ui.listWidget);
    item->setToolTip(id);
    item->setSizeHint(QSize(72, 84));
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    m_idToItem[id] = item;
    m_itemToId[item] = id;
}

void QtGradientView::slotGradientRenamed(const QString &id, const QString &newId)
{
    if (!m_idToItem.contains(id))
        return;

    QListWidgetItem *item = m_idToItem.value(id);
    item->setText(newId);
    item->setToolTip(newId);
    m_itemToId[item] = newId;
    m_idToItem.remove(id);
    m_idToItem[newId] = item;
}

void QtGradientView::slotGradientChanged(const QString &id, const QGradient &newGradient)
{
    if (!m_idToItem.contains(id))
        return;

    QListWidgetItem *item = m_idToItem.value(id);
    item->setIcon(QtGradientUtils::gradientPixmap(newGradient));
}

void QtGradientView::slotGradientRemoved(const QString &id)
{
    if (!m_idToItem.contains(id))
        return;

    QListWidgetItem *item = m_idToItem.value(id);
    delete item;
    m_itemToId.remove(item);
    m_idToItem.remove(id);
}

void QtGradientView::slotNewGradient()
{
    bool ok;
    QListWidgetItem *item = m_ui.listWidget->currentItem();
    QGradient grad = QLinearGradient();
    if (item)
        grad = m_manager->gradients().value(m_itemToId.value(item));
    QGradient gradient = QtGradientDialog::getGradient(&ok, grad, this);
    if (!ok)
        return;

    QString id = m_manager->addGradient(tr("Grad"), gradient);
    m_ui.listWidget->setCurrentItem(m_idToItem.value(id));
}

void QtGradientView::slotEditGradient()
{
    bool ok;
    QListWidgetItem *item = m_ui.listWidget->currentItem();
    if (!item)
        return;

    const QString id = m_itemToId.value(item);
    QGradient grad = m_manager->gradients().value(id);
    QGradient gradient = QtGradientDialog::getGradient(&ok, grad, this);
    if (!ok)
        return;

    m_manager->changeGradient(id, gradient);
}

void QtGradientView::slotRemoveGradient()
{
    QListWidgetItem *item = m_ui.listWidget->currentItem();
    if (!item)
        return;

    if (QMessageBox::question(this, tr("Remove Gradient"),
                tr("Are you sure you want to remove the selected gradient?"),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    const QString id = m_itemToId.value(item);
    m_manager->removeGradient(id);
}

void QtGradientView::slotRenameGradient()
{
    QListWidgetItem *item = m_ui.listWidget->currentItem();
    if (!item)
        return;

    m_ui.listWidget->editItem(item);
}

void QtGradientView::slotRenameGradient(QListWidgetItem *item)
{
    if (!item)
        return;

    const QString id = m_itemToId.value(item);
    m_manager->renameGradient(id, item->text());
}

void QtGradientView::slotCurrentItemChanged(QListWidgetItem *item)
{
    m_editAction->setEnabled(item);
    m_renameAction->setEnabled(item);
    m_removeAction->setEnabled(item);
    emit currentGradientChanged(m_itemToId.value(item));
}

void QtGradientView::slotGradientActivated(QListWidgetItem *item)
{
    const QString id = m_itemToId.value(item);
    if (!id.isEmpty())
        emit gradientActivated(id);
}

QtGradientView::QtGradientView(QWidget *parent)
    : QWidget(parent)
{
    m_manager = 0;

    m_ui.setupUi(this);

    m_ui.listWidget->setViewMode(QListView::IconMode);
    m_ui.listWidget->setMovement(QListView::Static);
    m_ui.listWidget->setTextElideMode(Qt::ElideRight);
    m_ui.listWidget->setResizeMode(QListWidget::Adjust);
    m_ui.listWidget->setIconSize(QSize(64, 64));
    m_ui.listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QPalette pal = m_ui.listWidget->viewport()->palette();
    int pixSize = 18;
    QPixmap pm(2 * pixSize, 2 * pixSize);

    QColor c1 = palette().color(QPalette::Midlight);
    QColor c2 = palette().color(QPalette::Dark);
    QPainter pmp(&pm);
    pmp.fillRect(0, 0, pixSize, pixSize, c1);
    pmp.fillRect(pixSize, pixSize, pixSize, pixSize, c1);
    pmp.fillRect(0, pixSize, pixSize, pixSize, c2);
    pmp.fillRect(pixSize, 0, pixSize, pixSize, c2);

    pal.setBrush(QPalette::Base, QBrush(pm));
    m_ui.listWidget->viewport()->setPalette(pal);

    connect(m_ui.listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotGradientActivated(QListWidgetItem*)));
    connect(m_ui.listWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(slotRenameGradient(QListWidgetItem*)));
    connect(m_ui.listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(slotCurrentItemChanged(QListWidgetItem*)));

    m_newAction = new QAction(QIcon(QLatin1String(":/trolltech/qtgradienteditor/images/plus.png")), tr("New..."), this);
    m_editAction = new QAction(QIcon(QLatin1String(":/trolltech/qtgradienteditor/images/edit.png")), tr("Edit..."), this);
    m_renameAction = new QAction(tr("Rename"), this);
    m_removeAction = new QAction(QIcon(QLatin1String(":/trolltech/qtgradienteditor/images/minus.png")), tr("Remove"), this);

    connect(m_newAction, SIGNAL(triggered()), this, SLOT(slotNewGradient()));
    connect(m_editAction, SIGNAL(triggered()), this, SLOT(slotEditGradient()));
    connect(m_removeAction, SIGNAL(triggered()), this, SLOT(slotRemoveGradient()));
    connect(m_renameAction, SIGNAL(triggered()), this, SLOT(slotRenameGradient()));

    m_ui.listWidget->addAction(m_newAction);
    m_ui.listWidget->addAction(m_editAction);
    m_ui.listWidget->addAction(m_renameAction);
    m_ui.listWidget->addAction(m_removeAction);

    m_ui.newButton->setDefaultAction(m_newAction);
    m_ui.editButton->setDefaultAction(m_editAction);
    m_ui.renameButton->setDefaultAction(m_renameAction);
    m_ui.removeButton->setDefaultAction(m_removeAction);

    m_ui.listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void QtGradientView::setGradientManager(QtGradientManager *manager)
{
    if (m_manager == manager)
        return;

    if (m_manager) {
        disconnect(m_manager, SIGNAL(gradientAdded(QString,QGradient)),
                    this, SLOT(slotGradientAdded(QString,QGradient)));
        disconnect(m_manager, SIGNAL(gradientRenamed(QString,QString)),
                    this, SLOT(slotGradientRenamed(QString,QString)));
        disconnect(m_manager, SIGNAL(gradientChanged(QString,QGradient)),
                    this, SLOT(slotGradientChanged(QString,QGradient)));
        disconnect(m_manager, SIGNAL(gradientRemoved(QString)),
                    this, SLOT(slotGradientRemoved(QString)));

        m_ui.listWidget->clear();
        m_idToItem.clear();
        m_itemToId.clear();
    }

    m_manager = manager;

    if (!m_manager)
        return;

    QMap<QString, QGradient> gradients = m_manager->gradients();
    QMapIterator<QString, QGradient> itGrad(gradients);
    while (itGrad.hasNext()) {
        itGrad.next();
        slotGradientAdded(itGrad.key(), itGrad.value());
    }

    connect(m_manager, SIGNAL(gradientAdded(QString,QGradient)),
            this, SLOT(slotGradientAdded(QString,QGradient)));
    connect(m_manager, SIGNAL(gradientRenamed(QString,QString)),
            this, SLOT(slotGradientRenamed(QString,QString)));
    connect(m_manager, SIGNAL(gradientChanged(QString,QGradient)),
            this, SLOT(slotGradientChanged(QString,QGradient)));
    connect(m_manager, SIGNAL(gradientRemoved(QString)),
            this, SLOT(slotGradientRemoved(QString)));
}

QtGradientManager *QtGradientView::gradientManager() const
{
    return m_manager;
}

void QtGradientView::setCurrentGradient(const QString &id)
{
    QListWidgetItem *item = m_idToItem.value(id);
    if (!item)
        return;

    m_ui.listWidget->setCurrentItem(item);
}

QString QtGradientView::currentGradient() const
{
    return m_itemToId.value(m_ui.listWidget->currentItem());
}

QT_END_NAMESPACE
