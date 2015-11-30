/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "filewidget.h"

#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include <QComboBox>

#include <QHBoxLayout>
#include <QFont>
#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>

namespace QmlEditorWidgets {

FileWidget::FileWidget(QWidget *parent) :
    QWidget(parent),
    m_filter(QLatin1String("(*.*)")),
    m_showComboBox(false),
    m_lock(false)
{
    m_pushButton = new QToolButton(this);
    m_pushButton->setFixedWidth(32);
    m_lineEdit = new QLineEdit(this);
    m_comboBox = new QComboBox(this);
    m_comboBox->hide();
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_comboBox);
    m_comboBox->setEditable(true);
    layout->addWidget(m_pushButton);
    m_pushButton->setText(QLatin1String("..."));
    connect(m_lineEdit, &QLineEdit::editingFinished, this, &FileWidget::lineEditChanged);
    connect(m_pushButton, &QToolButton::released, this, &FileWidget::onButtonReleased);
    connect(m_comboBox, &QComboBox::editTextChanged, this, &FileWidget::comboBoxChanged);
}

FileWidget::~FileWidget()
{
}

void FileWidget::setShowComboBox(bool show)
{
    m_showComboBox = show;
    m_comboBox->setVisible(show);
    m_lineEdit->setVisible(!show);
}

void FileWidget::lineEditChanged()
{
    if (m_lock)
        return;
    setFileNameStr(m_lineEdit->text());
}

void FileWidget::comboBoxChanged()
{
    if (m_lock)
        return;
    setFileNameStr(m_comboBox->currentText());
}

void FileWidget::onButtonReleased()
{
    QString newFile = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                   m_path.toLocalFile(), m_filter);
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
    if (m_lineEdit->text() != fileName.toString()) {
        m_lineEdit->setText(fileName.toString());
        m_lineEdit->setToolTip(m_fileName.toString());
    }
    if (m_comboBox->currentText() != fileName.toString()) {
        m_comboBox->setEditText(m_fileName.toString());
        m_comboBox->setToolTip(m_fileName.toString());
    }
    emit fileNameChanged(fileName);
}

void FileWidget::setupComboBox()
{
    m_lock = true;
    m_comboBox->clear();

    QDir dir;


    if (m_path.isValid())
        dir = QDir(m_path.toLocalFile());

    QStringList filterList = m_filter.split(QLatin1Char(' '));

    QDirIterator it(dir.absolutePath(), filterList, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString absolutePath = it.next();
        m_comboBox->addItem(dir.relativeFilePath(absolutePath));
    }
    m_comboBox->setEditText(m_fileName.toString());

    m_lock = false;
}

} //QmlEditorWidgets


