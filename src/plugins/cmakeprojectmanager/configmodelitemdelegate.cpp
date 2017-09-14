/****************************************************************************
**
** Copyright (C) 2016 Alexander Drozdov.
** Contact: adrozdoff@gmail.com
**
** This file is part of CMakeProjectManager2 plugin.
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
****************************************************************************/

#include "configmodelitemdelegate.h"
#include "configmodel.h"

#include <utils/asconst.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPainter>

namespace CMakeProjectManager {

ConfigModelItemDelegate::ConfigModelItemDelegate(const Utils::FileName &base, QObject* parent)
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
            edit->setBaseFileName(m_base);
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
            for (const QString &s : Utils::asConst(data.values))
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
            edit->setFileName(Utils::FileName::fromUserInput(data.value));
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
                model->setData(index, edit->fileName().toUserOutput(), Qt::EditRole);
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
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(100, m_measurement.sizeHint().height());
}

} // namespace CMakeProjectManager

