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

#include "filewidget.h"
#include <QHBoxLayout>
#include <QFont>
#include <QFileDialog>
#include <QDebug>


QT_BEGIN_NAMESPACE

FileWidget::FileWidget(QWidget *parent) : QWidget(parent)
{
    m_pushButton = new QPushButton(this);
    m_label = new QLabel(this);
    m_lineEdit = new QLineEdit(this);
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);
    layout->addWidget(m_label);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_pushButton);
    m_pushButton->setText("...");
    QFont f = m_label->font();
    f.setBold(true);
    m_label->setFont(f);
    connect(m_lineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditChanged()));
    connect(m_pushButton, SIGNAL(pressed()), this, SLOT(buttonPressed()));
}

FileWidget::~FileWidget()
{
}


void FileWidget::lineEditChanged()
{
    setFileNameStr(m_lineEdit->text());
}

void FileWidget::buttonPressed()
{
    QString newFile = QFileDialog::getOpenFileName();
    if (!newFile.isEmpty())
        setFileNameStr(newFile);
}

void FileWidget::setFileNameStr(const QString &fileName)
{
    setFileName(QUrl(fileName));
}
void FileWidget::setFileName(const QUrl &fileName)
{
    if (fileName == m_fileName)
        return;

    m_fileName = fileName;
    m_lineEdit->setText(fileName.toString());
    emit fileNameChanged(fileName);
}

QT_END_NAMESPACE

