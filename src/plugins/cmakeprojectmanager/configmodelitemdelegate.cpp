// Copyright (C) 2016 Alexander Drozdov.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configmodelitemdelegate.h"

#include "configmodel.h"
#include "cmakeprojectmanagertr.h"

#include <utils/pathchooser.h>

#include <QCheckBox>

using namespace Utils;

namespace CMakeProjectManager::Internal {

ConfigModelItemDelegate::ConfigModelItemDelegate(const FilePath &base, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_base(base)
{ }

QWidget *ConfigModelItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const

{
    if (index.column() == 1) {
        ConfigModel::DataItem data = ConfigModel::dataItemFromIndex(index);
        if (data.type == ConfigModel::DataItem::FILE || data.type == ConfigModel::DataItem::DIRECTORY) {
            auto edit = new PathChooser(parent);
            edit->setAttribute(Qt::WA_MacSmallSize);
            edit->setFocusPolicy(Qt::StrongFocus);
            edit->setBaseDirectory(m_base);
            edit->setAutoFillBackground(true);
            if (data.type == ConfigModel::DataItem::FILE) {
                edit->setExpectedKind(PathChooser::File);
                edit->setPromptDialogTitle(Tr::tr("Select a file for %1").arg(data.key));
            } else {
                edit->setExpectedKind(PathChooser::Directory);
                edit->setPromptDialogTitle(Tr::tr("Select a directory for %1").arg(data.key));
            }
            return edit;
        } else if (!data.values.isEmpty()) {
            auto edit = new QComboBox(parent);
            edit->setAttribute(Qt::WA_MacSmallSize);
            edit->setFocusPolicy(Qt::StrongFocus);
            edit->setAutoFillBackground(true);
            for (const QString &s : std::as_const(data.values))
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
            auto edit = static_cast<PathChooser *>(editor);
            edit->setFilePath(FilePath::fromUserInput(data.value));
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
            auto edit = static_cast<PathChooser *>(editor);
            if (edit->rawFilePath().toString() != data.value)
                model->setData(index, edit->rawFilePath().toString(), Qt::EditRole);
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

QSize ConfigModelItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    static int height = -1;
    if (height < 0) {
        const auto setMaxSize = [](const QWidget &w) {
            if (w.sizeHint().height() > height)
                height = w.sizeHint().height();
        };
        QComboBox box;
        box.setAttribute(Qt::WA_MacSmallSize);
        QCheckBox check;
        setMaxSize(box);
        setMaxSize(check);
        // Do not take the path chooser into consideration, because that would make the height
        // larger on Windows, leading to less items displayed, and the size of PathChooser looks
        // "fine enough" as is.
    }
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(100, height);
}

} // CMakeProjectManager::Internal
