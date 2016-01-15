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

#include "topicchooser.h"

#include <QMap>
#include <QUrl>

#include <QKeyEvent>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

TopicChooser::TopicChooser(QWidget *parent, const QString &keyword,
        const QMap<QString, QUrl> &links)
    : QDialog(parent)
    , m_filterModel(new QSortFilterProxyModel(this))
{
    ui.setupUi(this);

    setFocusProxy(ui.lineEdit);
    ui.lineEdit->setFiltering(true);
    ui.lineEdit->installEventFilter(this);
    ui.lineEdit->setPlaceholderText(tr("Filter"));
    ui.label->setText(tr("Choose a topic for <b>%1</b>:").arg(keyword));

    QStandardItemModel *model = new QStandardItemModel(this);
    m_filterModel->setSourceModel(model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QMap<QString, QUrl>::const_iterator it = links.constBegin();
    for (; it != links.constEnd(); ++it) {
        m_links.append(it.value());
        QStandardItem *item = new QStandardItem(it.key());
        item->setToolTip(it.value().toString());
        model->appendRow(item);
    }

    ui.listWidget->setModel(m_filterModel);
    ui.listWidget->setUniformItemSizes(true);
    ui.listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (m_filterModel->rowCount() != 0)
        ui.listWidget->setCurrentIndex(m_filterModel->index(0, 0));

    connect(ui.buttonDisplay, &QPushButton::clicked,
            this, &TopicChooser::acceptDialog);
    connect(ui.buttonCancel, &QPushButton::clicked,
            this, &TopicChooser::reject);
    connect(ui.listWidget, &QListView::activated,
            this, &TopicChooser::activated);
    connect(ui.lineEdit, &Utils::FancyLineEdit::filterChanged,
            this, &TopicChooser::setFilter);
}

QUrl TopicChooser::link() const
{
    if (m_activedIndex.isValid())
        return m_links.at(m_filterModel->mapToSource(m_activedIndex).row());
    return QUrl();
}

void TopicChooser::acceptDialog()
{
    m_activedIndex = ui.listWidget->currentIndex();
    accept();
}

void TopicChooser::setFilter(const QString &pattern)
{
    m_filterModel->setFilterFixedString(pattern);
    if (m_filterModel->rowCount() != 0 && !ui.listWidget->currentIndex().isValid())
        ui.listWidget->setCurrentIndex(m_filterModel->index(0, 0));
}

void TopicChooser::activated(const QModelIndex &index)
{
    m_activedIndex = index;
    accept();
}

bool TopicChooser::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui.lineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        int dIndex = 0;
        switch (ke->key()) {
        case Qt::Key_Up:
            dIndex = -1;
            break;
        case Qt::Key_Down:
            dIndex = +1;
            break;
        case Qt::Key_PageUp:
            dIndex = -5;
            break;
        case Qt::Key_PageDown:
            dIndex = +5;
            break;
        default:
            break;
        }
        if (dIndex != 0) {
            QModelIndex idx = ui.listWidget->currentIndex();
            int newIndex = qMin(m_filterModel->rowCount(idx.parent()) - 1,
                                qMax(0, idx.row() + dIndex));
            idx = m_filterModel->index(newIndex, idx.column(), idx.parent());
            if (idx.isValid())
                ui.listWidget->setCurrentIndex(idx);
            return true;
        }
    } else if (ui.lineEdit && event->type() == QEvent::FocusIn
        && static_cast<QFocusEvent *>(event)->reason() != Qt::MouseFocusReason) {
        ui.lineEdit->selectAll();
        ui.lineEdit->setFocus();
    }
    return QDialog::eventFilter(object, event);
}
