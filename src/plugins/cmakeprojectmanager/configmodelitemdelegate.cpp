/****************************************************************************
**
** Copyright (C) 2016 Alexander Drozdov.
** Contact: adrozdoff@gmail.com
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

#include "configmodelitemdelegate.h"

#include "configmodel.h"

#include <utils/pathchooser.h>

#include <QCheckBox>

namespace CMakeProjectManager {

ConfigModelItemDelegate::ConfigModelItemDelegate(const Utils::FilePath &base, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_base(base)
{ }

QWidget *ConfigModelItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const

{
    if (index.column() == 1) {
        ConfigModel::DataItem data = ConfigModel::dataItemFromIndex(index);
        if (data.type == ConfigModel::DataItem::FILE || data.type == ConfigModel::DataItem::DIRECTORY) {
            auto edit = new Utils::PathChooser(parent);
            edit->setFocusPolicy(Qt::StrongFocus);
            edit->setBaseDirectory(m_base);
            edit->setAutoFillBackground(true);
            if (data.type == ConfigModel::DataItem::FILE) {
                edit->setExpectedKind(Utils::PathChooser::File);
                edit->setPromptDialogTitle(tr("Select a file for %1").arg(data.key));
            } else {
                edit->setExpectedKind(Utils::PathChooser::Directory);
                edit->setPromptDialogTitle(tr("Select a directory for %1").arg(data.key));
            }
            return edit;
        } else if (!data.values.isEmpty()) {
            auto edit = new QComboBox(parent);
            edit->setFocusPolicy(Qt::StrongFocus);
            for (const QString &s : qAsConst(data.values))
                edit->addItem(s);
            return edit;
        } else if (data.type == ConfigModel::DataItem::BOOLEAN) {
            auto edit = new QCheckBox(parent);
            edit->setFocusPolicy(Qt::StrongFocus);
            return edit;
        } else if (data.type == ConfigModel::DataItem::STRING) {
            auto edit = new QLineEdit(parent);
            edit->setFocusPolicy(Qt::StrongFocus);
            return edit;
        }
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void ConfigModelItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == 1) {
        ConfigModel::DataItem data = ConfigModel::dataItemFromIndex(index);
        if (data.type == ConfigModel::DataItem::FILE || data.type == ConfigModel::DataItem::DIRECTORY) {
            auto edit = static_cast<Utils::PathChooser *>(editor);
            edit->setFilePath(Utils::FilePath::fromUserInput(data.value));
            return;
        } else if (!data.values.isEmpty()) {
            auto edit = static_cast<QComboBox *>(editor);
            edit->setCurrentText(data.value);
            return;
        } else if (data.type == ConfigModel::DataItem::BOOLEAN) {
            auto edit = static_cast<QCheckBox *>(editor);
            edit->setChecked(index.data(Qt::CheckStateRole).toBool());
            edit->setText(data.value);
            return;
        } else if (data.type == ConfigModel::DataItem::STRING) {
            auto edit = static_cast<QLineEdit *>(editor);
            edit->setText(data.value);
            return;
        }
    }
    QStyledItemDelegate::setEditorData(editor, index);
}

void ConfigModelItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                           const QModelIndex &index) const
{
    if (index.column() == 1) {
        ConfigModel::DataItem data = ConfigModel::dataItemFromIndex(index);
        if (data.type == ConfigModel::DataItem::FILE || data.type == ConfigModel::DataItem::DIRECTORY) {
            auto edit = static_cast<Utils::PathChooser *>(editor);
            if (edit->rawPath() != data.value)
                model->setData(index, edit->filePath().toString(), Qt::EditRole);
            return;
        } else if (!data.values.isEmpty()) {
            auto edit = static_cast<QComboBox *>(editor);
            model->setData(index, edit->currentText(), Qt::EditRole);
            return;
        } else if (data.type == ConfigModel::DataItem::BOOLEAN) {
            auto edit = static_cast<QCheckBox *>(editor);
            model->setData(index, edit->text(), Qt::EditRole);
        } else if (data.type == ConfigModel::DataItem::STRING) {
            auto edit = static_cast<QLineEdit *>(editor);
            model->setData(index, edit->text(), Qt::EditRole);
        }
    }
    QStyledItemDelegate::setModelData(editor, model, index);
}

QSize CMakeProjectManager::ConfigModelItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                                             const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(100, m_measurement.sizeHint().height());
}

} // namespace CMakeProjectManager

