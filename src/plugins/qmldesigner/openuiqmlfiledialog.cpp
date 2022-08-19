// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "openuiqmlfiledialog.h"
#include "ui_openuiqmlfiledialog.h"

#include <qmldesignerplugin.h>

#include <QDir>

namespace QmlDesigner {

OpenUiQmlFileDialog::OpenUiQmlFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenUiQmlFileDialog)
{
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

    for (const QString &fileName : stringList) {
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
