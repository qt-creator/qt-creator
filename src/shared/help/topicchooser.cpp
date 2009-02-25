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

#include <QtCore/QMap>
#include <QtCore/QUrl>

#include "topicchooser.h"

QT_BEGIN_NAMESPACE

TopicChooser::TopicChooser(QWidget *parent, const QString &keyword,
                         const QMap<QString, QUrl> &links)
    : QDialog(parent)
{
    ui.setupUi(this);
    ui.label->setText(tr("Choose a topic for <b>%1</b>:").arg(keyword));

    m_links = links;
    QMap<QString, QUrl>::const_iterator it = m_links.constBegin();
    for (; it != m_links.constEnd(); ++it)
        ui.listWidget->addItem(it.key());
    
    if (ui.listWidget->count() != 0)
        ui.listWidget->setCurrentRow(0);
    ui.listWidget->setFocus();

    connect(ui.buttonDisplay, SIGNAL(clicked()),
        this, SLOT(accept()));
    connect(ui.buttonCancel, SIGNAL(clicked()),
        this, SLOT(reject()));
    connect(ui.listWidget, SIGNAL(itemActivated(QListWidgetItem*)),
        this, SLOT(accept()));
}

QUrl TopicChooser::link() const
{
    QListWidgetItem *item = ui.listWidget->currentItem();
    if (!item)
        return QUrl();

    QString title = item->text();
    if (title.isEmpty() || !m_links.contains(title))
        return QUrl();

    return m_links.value(title);
}

QT_END_NAMESPACE
