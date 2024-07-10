// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"
#include "iwelcomepage.h"

#include <utils/fancylineedit.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QComboBox>
#include <QElapsedTimer>
#include <QLabel>
#include <QListView>
#include <QPen>
#include <QPointer>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTimer>

#include <functional>
#include <optional>

namespace Core {

namespace WelcomePageHelpers {

constexpr QSize WelcomeThumbnailSize(214, 160);

class CORE_EXPORT TextFormat
{
public:
    QColor color() const
    {
        return Utils::creatorColor(themeColor);
    }

    QFont font(bool underlined = false) const
    {
        QFont result = Utils::StyleHelper::uiFont(uiElement);
        result.setUnderline(underlined);
        return result;
    }

    int lineHeight() const
    {
        return Utils::StyleHelper::uiFontLineHeight(uiElement);
    }

    const Utils::Theme::Color themeColor;
    const Utils::StyleHelper::UiElement uiElement;
    const int drawTextFlags = Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip
                              | Qt::TextShowMnemonic;
};

CORE_EXPORT void setBackgroundColor(QWidget *widget, Utils::Theme::Color colorRole);
constexpr qreal defaultCardBackgroundRounding = 3.75;
constexpr Utils::Theme::Color cardDefaultBackground = Utils::Theme::Token_Background_Muted;
constexpr Utils::Theme::Color cardDefaultStroke = Utils::Theme::Token_Stroke_Subtle;
constexpr Utils::Theme::Color cardHoverBackground = Utils::Theme::Token_Background_Subtle;
constexpr Utils::Theme::Color cardHoverStroke = cardDefaultStroke;
CORE_EXPORT void drawCardBackground(QPainter *painter, const QRectF &rect,
                                    const QBrush &fill, const QPen &pen = QPen(Qt::NoPen),
                                    qreal rounding = defaultCardBackgroundRounding);
CORE_EXPORT QWidget *createRule(Qt::Orientation orientation, QWidget *parent = nullptr);

} // namespace WelcomePageHelpers

class CORE_EXPORT Button : public QAbstractButton
{
public:
    enum Role {
        MediumPrimary,
        MediumSecondary,
        SmallPrimary,
        SmallSecondary,
        SmallList,
        SmallLink,
        Tag,
    };

    explicit Button(const QString &text, Role role, QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

    void setPixmap(const QPixmap &newPixmap);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateMargins();

    const Role m_role = MediumPrimary;
    QPixmap m_pixmap;
};

class CORE_EXPORT Label : public QLabel
{
public:
    enum Role {
        Primary,
        Secondary,
    };

    explicit Label(const QString &text, Role role, QWidget *parent = nullptr);

private:
    const Role m_role = Primary;
};

class CORE_EXPORT SearchBox : public Utils::FancyLineEdit
{
public:
    explicit SearchBox(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

class CORE_EXPORT ComboBox : public QComboBox
{
public:
    explicit ComboBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
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
    ListItemDelegate() = default;

    static QSize itemSize();
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

class CORE_EXPORT ResizeSignallingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResizeSignallingWidget(QWidget *parent = nullptr);
    void resizeEvent(QResizeEvent *event) override;

signals:
    void resized(const QSize &size, const QSize &oldSize);
};

} // namespace Core

Q_DECLARE_METATYPE(Core::ListItem *)
