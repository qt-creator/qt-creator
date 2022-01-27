/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "core_global.h"
#include "iwelcomepage.h"

#include <utils/optional.h>

#include <QElapsedTimer>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QListView>

namespace Utils { class FancyLineEdit; }

namespace Core {

namespace WelcomePageHelpers {

constexpr int HSpacing = 20;
constexpr int ItemGap = 4;
CORE_EXPORT QFont brandFont();
CORE_EXPORT QWidget *panelBar(QWidget *parent = nullptr);

} // namespace WelcomePageHelpers

class CORE_EXPORT SearchBox : public WelcomePageFrame
{
public:
    explicit SearchBox(QWidget *parent);

    Utils::FancyLineEdit *m_lineEdit = nullptr;
};

class CORE_EXPORT GridView : public QListView
{
public:
    explicit GridView(QWidget *parent);
protected:
    void leaveEvent(QEvent *) final;
};

using OptModelIndex = Utils::optional<QModelIndex>;

class CORE_EXPORT ListItem
{
public:
    virtual ~ListItem() {}
    QString name;
    QString description;
    QString imageUrl;
    QStringList tags;
};

class CORE_EXPORT ListModel : public QAbstractListModel
{
public:
    enum ListDataRole {
        ItemRole = Qt::UserRole,
        ItemImageRole,
        ItemTagsRole
    };

    explicit ListModel(QObject *parent);
    ~ListModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QPixmap fetchPixmapAndUpdatePixmapCache(const QString &url) const = 0;

    static const QSize defaultImageSize;

protected:
    QList<ListItem *> m_items;
};

class CORE_EXPORT ListModelFilter : public QSortFilterProxyModel
{
public:
    ListModelFilter(ListModel *sourceModel, QObject *parent);

    void setSearchString(const QString &arg);

protected:
    virtual bool leaveFilterAcceptsRowBeforeFiltering(const ListItem *item,
                                                      bool *earlyExitResult) const;

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const final;
    void timerEvent(QTimerEvent *event) final;

    void delayedUpdateFilter();

    QString m_searchString;
    QStringList m_filterTags;
    QStringList m_filterStrings;
    int m_timerId = 0;
};

class CORE_EXPORT ListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ListItemDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    static constexpr int GridItemGap = 3 * WelcomePageHelpers::ItemGap;
    static constexpr int GridItemWidth = 240 + GridItemGap;
    static constexpr int GridItemHeight = GridItemWidth;
    static constexpr int TagsSeparatorY = GridItemHeight - GridItemGap - 52;

signals:
    void tagClicked(const QString &tag);

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    virtual void drawPixmapOverlay(const ListItem *item, QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QRect &currentPixmapRect) const;
    virtual void clickAction(const ListItem *item) const;

    void goon();

    const QColor backgroundPrimaryColor;
    const QColor backgroundSecondaryColor;
    const QColor foregroundPrimaryColor;
    const QColor hoverColor;
    const QColor textColor;

private:
    mutable QPersistentModelIndex m_previousIndex;
    mutable QElapsedTimer m_startTime;
    mutable QPointer<QAbstractItemView> m_currentWidget;
    mutable QVector<QPair<QString, QRect>> m_currentTagRects;
    mutable QPixmap m_blurredThumbnail;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::ListItem *)
