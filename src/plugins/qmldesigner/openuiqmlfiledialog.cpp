// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openuiqmlfiledialog.h"

#include <qmldesignerplugin.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDir>
#include <QListWidget>
#include <QPushButton>

namespace QmlDesigner {

OpenUiQmlFileDialog::OpenUiQmlFileDialog(QWidget *parent) :
    QDialog(parent)
{
    resize(600, 300);
    setModal(true);
    setWindowTitle(tr("Open ui.qml file"));

    auto checkBox = new QCheckBox(tr("Do not show this dialog again"));

    auto openButton = new QPushButton(tr("Open ui.qml file"));
    auto cancelButton = new QPushButton(tr("Cancel"));

    cancelButton->setDefault(true);

    m_listWidget = new QListWidget;

    using namespace Layouting;

    Column {
        tr("You are opening a .qml file in the designer. Do you want to open a .ui.qml file instead?"),
        m_listWidget,
        checkBox,
        Row { st, openButton, cancelButton }  // FIXME: Use QDialogButtonBox to get order right?
    }.attachTo(this);

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::close);
    connect(openButton, &QPushButton::clicked, [this] {
        QListWidgetItem *item = m_listWidget->currentItem();
        if (item) {
            m_uiFileOpened = true;
            m_uiQmlFile = item->data(Qt::UserRole).toString();
        }
        close();
    });
    connect(m_listWidget, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        if (item) {
            m_uiFileOpened = true;
            m_uiQmlFile = item->data(Qt::UserRole).toString();
        }
        close();
    });
    connect(checkBox, &QCheckBox::toggled, this, [](bool b){
        QmlDesignerPlugin::settings().insert(
            DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES, !b);
    });
}

OpenUiQmlFileDialog::~OpenUiQmlFileDialog() = default;

bool OpenUiQmlFileDialog::uiFileOpened() const
{
    return m_uiFileOpened;
}

void OpenUiQmlFileDialog::setUiQmlFiles(const QString &projectPath, const QStringList &stringList)
{
    QDir projectDir(projectPath);

    for (const QString &fileName : stringList) {
        QListWidgetItem *item = new QListWidgetItem(projectDir.relativeFilePath(fileName), m_listWidget);
        item->setData(Qt::UserRole, fileName);
        m_listWidget->addItem(item);
    }
    m_listWidget->setCurrentItem(m_listWidget->item(0));
}

QString OpenUiQmlFileDialog::uiQmlFile() const
{
    return m_uiQmlFile;
}

} // namespace QmlDesigner
