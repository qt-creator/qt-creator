// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QStandardItemModel;
QT_END_NAMESPACE

namespace Utils { class QtcSearchBox; }

namespace AcpClient::Internal {

struct ConfigSelectEntry
{
    QString value;
    QString name;
    QString description;
    QString group;
};

class ConfigSelectListView;

// Filterable list of the values of a select configuration option, shown as a
// popup below its button. Values the user marked as favorites are repeated in a
// section at the top.
class ConfigSelectPopup final : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigSelectPopup(QWidget *parent = nullptr);

    void setEntries(const QList<ConfigSelectEntry> &entries, const QString &currentValue);
    void setFavorites(const QStringList &favoriteValues);

    void setFilter(const QString &filter);
    // The values passing the current filter, in display order, favorites first.
    QStringList visibleValues() const;

    void showRelativeTo(QWidget *anchor);

signals:
    void valueSelected(const QString &value);
    void favoritesChanged(const QStringList &favoriteValues);

private:
    void rebuild();
    void updateListHeight();
    int preferredHeight() const;
    void moveSelection(int delta);
    void activate(const QModelIndex &index);
    void toggleFavorite(const QModelIndex &index);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    QList<ConfigSelectEntry> m_entries;
    QStringList m_favorites;
    QString m_currentValue;
    QString m_filter;
    bool m_openedUpwards = false;

    Utils::QtcSearchBox *m_filterEdit = nullptr;
    ConfigSelectListView *m_view = nullptr;
    QStandardItemModel *m_model = nullptr;
};

} // namespace AcpClient::Internal
