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

#include "openuiqmlfiledialog.h"
#include "ui_openuiqmlfiledialog.h"

#include <qmldesignerplugin.h>

#include <QDir>

namespace QmlDesigner {

OpenUiQmlFileDialog::OpenUiQmlFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenUiQmlFileDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->openButton, &QPushButton::clicked, [this] {
        QListWidgetItem *item = ui->listWidget->currentItem();
        if (item) {
            m_uiFileOpened = true;
            m_uiQmlFile = item->data(Qt::UserRole).toString();
        }
        close();
    });
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        if (item) {
            m_uiFileOpened = true;
            m_uiQmlFile = item->data(Qt::UserRole).toString();
        }
        close();
    });
    connect(ui->checkBox, &QCheckBox::toggled, this, [](bool b){
        DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
        settings.insert(DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES, !b);
        QmlDesignerPlugin::instance()->setSettings(settings);
    });
}

OpenUiQmlFileDialog::~OpenUiQmlFileDialog()
{
    delete ui;
}

bool OpenUiQmlFileDialog::uiFileOpened() const
{
    return m_uiFileOpened;
}

void OpenUiQmlFileDialog::setUiQmlFiles(const QString &projectPath, const QStringList &stringList)
{
    QDir projectDir(projectPath);

    foreach (const QString &fileName, stringList) {
        QListWidgetItem *item = new QListWidgetItem(projectDir.relativeFilePath(fileName), ui->listWidget);
        item->setData(Qt::UserRole, fileName);
        ui->listWidget->addItem(item);
    }
    ui->listWidget->setCurrentItem(ui->listWidget->item(0));
}

QString OpenUiQmlFileDialog::uiQmlFile() const
{
    return m_uiQmlFile;
}

} // namespace QmlDesigner
