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

#pragma once

#include <QComboBox>
#include <QStyledItemDelegate>

#include <utils/fileutils.h>

namespace CMakeProjectManager {

class ConfigModelItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ConfigModelItemDelegate(const Utils::FileName &base, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const final;
    void setEditorData(QWidget *editor, const QModelIndex &index) const final;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const final;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const final;

private:
    Utils::FileName m_base;
    QComboBox m_measurement;
};

} // namespace CMakeProjectManager
