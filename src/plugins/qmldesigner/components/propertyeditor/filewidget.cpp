/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "filewidget.h"
#include <QHBoxLayout>
#include <QFont>
#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>
#include <model.h>


QT_BEGIN_NAMESPACE

static QString s_lastBrowserPath;

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
    QString modelPath;
    if (m_itemNode.isValid())
        modelPath = QFileInfo(m_itemNode.modelNode().model()->fileUrl().toLocalFile()).absoluteDir().absolutePath();

    m_lastModelPath = modelPath;

    bool documentChanged = m_lastModelPath == modelPath;

    //First we try the last path this browser widget was opened with
    //if the document was not changed
    QString path = documentChanged ? QString() : m_currentPath;


    //If that one is not valid we try the path for the current file
    if (path.isEmpty() && !m_fileName.isEmpty())
        path = QFileInfo(modelPath + QLatin1String("/") + m_fileName.toString()).absoluteDir().absolutePath();


    //Next we try to fall back to the path any file browser was opened with
    if (!QFileInfo(path).exists()) {
        path = s_lastBrowserPath;

        //The last fallback is to try the path of the document
        if (!QFileInfo(path).exists()) {
            if (m_itemNode.isValid())
                path = modelPath;
        }
    }

    QString newFile = QFileDialog::getOpenFileName(0, tr("Open File"), path, m_filter);

    if (!newFile.isEmpty()) {
        setFileNameStr(newFile);

        m_currentPath = QFileInfo(newFile).absolutePath();
        s_lastBrowserPath = m_currentPath;
    }
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

