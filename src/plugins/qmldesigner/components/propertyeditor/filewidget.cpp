/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "filewidget.h"
#include <QHBoxLayout>
#include <QFont>
#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>
#include <model.h>


QT_BEGIN_NAMESPACE

FileWidget::FileWidget(QWidget *parent) : QWidget(parent), m_filter("(*.*)"), m_showComboBox(false), m_lock(false)
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
    m_pushButton->setText("...");
    connect(m_lineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditChanged()));
    connect(m_pushButton, SIGNAL(pressed()), this, SLOT(buttonPressed()));
    connect(m_comboBox, SIGNAL(editTextChanged(QString)), this, SLOT(comboBoxChanged()));
    m_currentPath = QDir::currentPath();
}

FileWidget::~FileWidget()
{
}

void FileWidget::setItemNode(const QVariant &itemNode)
{

    if (!itemNode.value<QmlDesigner::ModelNode>().isValid() || !QmlDesigner::QmlItemNode(itemNode.value<QmlDesigner::ModelNode>()).hasNodeParent())
        return;
    m_itemNode = itemNode.value<QmlDesigner::ModelNode>();
    setupComboBox();
    emit itemNodeChanged();
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

void FileWidget::buttonPressed()
{
    QString path = m_currentPath;
    if (m_itemNode.isValid()) {
        path = QFileInfo(m_itemNode.modelNode().model()->fileUrl().toLocalFile()).absoluteDir().absolutePath();
    }
    QString newFile = QFileDialog::getOpenFileName(0, tr("Open File"), path, m_filter);
    if (!newFile.isEmpty())
        setFileNameStr(newFile);

    m_currentPath = QFileInfo(newFile).absolutePath();
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

    if (m_itemNode.isValid())
        dir = QFileInfo(m_itemNode.modelNode().model()->fileUrl().toLocalFile()).dir();
    else if (m_path.isValid())
        dir = QDir(m_path.toLocalFile());

    QStringList filterList = m_filter.split(' ');

    QDirIterator it(dir.absolutePath(), filterList, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString absolutePath = it.next();
        m_comboBox->addItem(dir.relativeFilePath(absolutePath));
    }
    m_comboBox->setEditText(m_fileName.toString());

    m_lock = false;
}

QT_END_NAMESPACE

