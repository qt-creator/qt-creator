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

#include "topicchooser.h"

#include <QtCore/QMap>
#include <QtCore/QUrl>

#include <QtGui/QStandardItemModel>
#include <QtGui/QSortFilterProxyModel>

TopicChooser::TopicChooser(QWidget *parent, const QString &keyword,
        const QMap<QString, QUrl> &links)
    : QDialog(parent)
    , m_filterModel(new QSortFilterProxyModel(this))
{
    ui.setupUi(this);
    ui.label->setText(tr("Choose a topic for <b>%1</b>:").arg(keyword));

    QStandardItemModel *model = new QStandardItemModel(this);
    m_filterModel->setSourceModel(model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QMap<QString, QUrl>::const_iterator it = links.constBegin();
    for (; it != links.constEnd(); ++it) {
        m_links.append(it.value());
        model->appendRow(new QStandardItem(it.key()));
    }

    ui.listWidget->setModel(m_filterModel);
    ui.listWidget->setUniformItemSizes(true);
    ui.listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (m_filterModel->rowCount() != 0)
        ui.listWidget->setCurrentIndex(m_filterModel->index(0, 0));
    ui.listWidget->setFocus();

    connect(ui.buttonDisplay, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui.listWidget, SIGNAL(activated(QModelIndex)), this,
        SLOT(activated(QModelIndex)));
    connect(ui.lineEdit, SIGNAL(filterChanged(QString)), m_filterModel,
        SLOT(setFilterFixedString(QString)));
}

QUrl TopicChooser::link() const
{
    if (m_activedIndex.isValid())
        return m_links.at(m_filterModel->mapToSource(m_activedIndex).row());
    return QUrl();
}

void TopicChooser::activated(const QModelIndex &index)
{
    m_activedIndex = index;
    accept();
}
