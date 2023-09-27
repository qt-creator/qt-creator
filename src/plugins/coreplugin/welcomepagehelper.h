// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"
#include "iwelcomepage.h"

#include <QElapsedTimer>
#include <QListView>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTimer>

#include <functional>
#include <optional>

namespace Utils { class FancyLineEdit; }

namespace Core {

namespace WelcomePageHelpers {

constexpr int HSpacing = 20;
constexpr int ItemGap = 4;

constexpr int GridItemGap = 3 * ItemGap;
constexpr int GridItemWidth = 240 + GridItemGap;               // Extra GridItemGap as "spacing"
constexpr int GridItemHeight = GridItemWidth;
constexpr QSize GridItemImageSize(GridItemWidth - GridItemGap
                                      - 2 * (GridItemGap + 1), // Horizontal margins + 1 pixel
                                  GridItemHeight - GridItemGap
                                      - GridItemGap - 1        // Upper margin + 1 pixel
                                      - 67);                   // Bottom margin (for title + tags)

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
    explicit GridView(QWidget *parent = nullptr);

protected:
    void leaveEvent(QEvent *) final;
};

class CORE_EXPORT SectionGridView : public GridView
{
    Q_OBJECT

public:
    explicit SectionGridView(QWidget *parent);

    void setMaxRows(std::optional<int> max);
    std::optional<int> maxRows() const;

    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;

    void wheelEvent(QWheelEvent *e) override;
    bool event(QEvent *e) override;

signals:
    void itemsFitChanged(bool fit);

private:
    std::optional<int> m_maxRows;
};

using OptModelIndex = std::optional<QModelIndex>;

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
    enum ListDataRole { ItemRole = Qt::UserRole, ItemImageRole, ItemTagsRole };

    using PixmapFunction = std::function<QPixmap(QString)>;

    explicit ListModel(QObject *parent = nullptr);
    ~ListModel() override;

    void appendItems(const QList<ListItem *> &items);
    const QList<ListItem *> items() const;
    void clear();

    int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setPixmapFunction(const PixmapFunction &fetchPixmapAndUpdatePixmapCache);

    void setOwnsItems(bool owns);

private:
    QList<ListItem *> m_items;
    PixmapFunction m_fetchPixmapAndUpdatePixmapCache;
    bool m_ownsItems = true;
};

class CORE_EXPORT ListModelFilter : public QSortFilterProxyModel
{
public:
    ListModelFilter(ListModel *sourceModel, QObject *parent);

    void setSearchString(const QString &arg);

    ListModel *sourceListModel() const;

protected:
    virtual bool leaveFilterAcceptsRowBeforeFiltering(const ListItem *item,
                                                      bool *earlyExitResult) const;

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const final;

    QString m_searchString;
    QStringList m_filterTags;
    QStringList m_filterStrings;
};

class CORE_EXPORT ListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ListItemDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

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
    const QColor foregroundSecondaryColor;
    const QColor hoverColor;
    const QColor textColor;

private:
    mutable QPersistentModelIndex m_previousIndex;
    mutable QElapsedTimer m_startTime;
    mutable QPointer<QAbstractItemView> m_currentWidget;
    mutable QVector<QPair<QString, QRect>> m_currentTagRects;
    mutable QPixmap m_blurredThumbnail;
};

class CORE_EXPORT Section
{
public:
    Section(const QString &name, int priority);
    Section(const QString &name, int priority, std::optional<int> maxRows);

    friend bool operator<(const Section &lhs, const Section &rhs)
    {
        if (lhs.priority < rhs.priority)
            return true;
        return lhs.priority > rhs.priority ? false : lhs.name < rhs.name;
    }

    friend bool operator==(const Section &lhs, const Section &rhs)
    {
        return lhs.priority == rhs.priority && lhs.name == rhs.name;
    }

    QString name;
    int priority;
    std::optional<int> maxRows;
};

class CORE_EXPORT SectionedGridView : public QStackedWidget
{
public:
    explicit SectionedGridView(QWidget *parent = nullptr);
    ~SectionedGridView();

    void setItemDelegate(QAbstractItemDelegate *delegate);
    void setPixmapFunction(const Core::ListModel::PixmapFunction &pixmapFunction);
    void setSearchStringDelayed(const QString &searchString);
    void setSearchString(const QString &searchString);

    Core::ListModel *addSection(const Section &section, const QList<Core::ListItem *> &items);

    void clear();

private:
    void zoomInSection(const Section &section);

    QMap<Section, Core::ListModel *> m_sectionModels;
    QList<QWidget *> m_sectionLabels;
    QMap<Section, Core::GridView *> m_gridViews;
    std::unique_ptr<Core::ListModel> m_allItemsModel;
    std::unique_ptr<Core::GridView> m_allItemsView;
    QPointer<QWidget> m_zoomedInWidget;
    Core::ListModel::PixmapFunction m_pixmapFunction;
    QAbstractItemDelegate *m_itemDelegate = nullptr;
    QTimer m_searchTimer;
    QString m_delayedSearchString;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::ListItem *)
