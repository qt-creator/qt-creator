// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractTableModel>
#include <QFrame>

QT_BEGIN_NAMESPACE
class QLabel;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils { class TreeView; }

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
    QLabel *m_fileNameLabel;
    QLabel *m_levels;
    QLabel *m_timeLabel;
    Utils::TreeView *m_statisticsView;
};

} // namespace Common
} // namespace ScxmlEditor
