// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QUrl>
#include <QMap>
#include <QModelIndex>
#include <QString>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListView;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

class TopicChooser : public QDialog
{
    Q_OBJECT

public:
    TopicChooser(QWidget *parent, const QString &keyword,
        const QMultiMap<QString, QUrl> &links);
    ~TopicChooser() override;

    QUrl link() const;

private:
    void acceptDialog();
    void setFilter(const QString &pattern);
    void activated(const QModelIndex &index);
    bool eventFilter(QObject *object, QEvent *event) override;

    QList<QUrl> m_links;

    QModelIndex m_activedIndex;
    QSortFilterProxyModel *m_filterModel;

    Utils::FancyLineEdit *m_lineEdit;
    QListView *m_listWidget;
};
