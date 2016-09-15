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

#pragma once

#include "ui_statistics.h"

#include <QAbstractTableModel>
#include <QFrame>

QT_FORWARD_DECLARE_CLASS(QSortFilterProxyModel)

namespace ScxmlEditor {

namespace PluginInterface {
class ScxmlDocument;
class ScxmlTag;
} // namespace PluginInterface

namespace Common {

class StatisticsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    StatisticsModel(QObject *parent = nullptr);
    void setDocument(PluginInterface::ScxmlDocument *document);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int levels() const;

private:
    void calculateStats(PluginInterface::ScxmlTag *tag);

    QStringList m_names;
    QVector<int> m_counts;
    int m_levels = 0;
};

class Statistics : public QFrame
{
    Q_OBJECT

public:
    explicit Statistics(QWidget *parent = nullptr);

    void setDocument(PluginInterface::ScxmlDocument *doc);

private:
    StatisticsModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    Ui::Statistics m_ui;
};

} // namespace Common
} // namespace ScxmlEditor
