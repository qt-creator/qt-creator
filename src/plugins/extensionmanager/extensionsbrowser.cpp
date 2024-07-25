// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionsbrowser.h"

#include "extensionmanagertr.h"
#include "extensionsmodel.h"
#include "extensionmanagersettings.h"

#ifdef WITH_TESTS
#include "extensionmanager_test.h"
#endif // WITH_TESTS

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/plugininstallwizard.h>
#include <coreplugin/welcomepagehelper.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginview.h>
#include <extensionsystem/pluginmanager.h>

#include <solutions/spinner/spinner.h>
#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/stylehelper.h>

#include <QApplication>
#include <QItemDelegate>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>

using namespace Core;
using namespace ExtensionSystem;
using namespace Utils;
using namespace StyleHelper;
using namespace SpacingTokens;
using namespace WelcomePageHelpers;

namespace ExtensionManager::Internal {

Q_LOGGING_CATEGORY(browserLog, "qtc.extensionmanager.browser", QtWarningMsg)

constexpr int gapSize = HGapL;
constexpr int itemWidth = 330;
constexpr int cellWidth = itemWidth + gapSize;

class OptionChooser : public QComboBox
{
public:
    OptionChooser(const FilePath &iconMask, const QString &textTemplate, QWidget *parent = nullptr)
        : QComboBox(parent)
        , m_iconDefault(Icon({{iconMask, m_colorDefault}}, Icon::Tint).icon())
        , m_iconActive(Icon({{iconMask, m_colorActive}}, Icon::Tint).icon())
        , m_textTemplate(textTemplate)
    {
        setMouseTracking(true);
        connect(this, &QComboBox::currentIndexChanged, this, &QWidget::updateGeometry);
    }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent *event) override
    {
        // +------------+------+---------+---------------+------------+
        // |            |      |         |  (VPaddingXs) |            |
        // |            |      |         +---------------+            |
        // |(HPaddingXs)|(icon)|(HGapXxs)|<template%item>|(HPaddingXs)|
        // |            |      |         +---------------+            |
        // |            |      |         |  (VPaddingXs) |            |
        // +------------+------+---------+---------------+------------+

        const bool active = currentIndex() > 0;
        const bool hover = underMouse();
        const TextFormat &tF = (active || hover) ? m_itemActiveTf : m_itemDefaultTf;

        const QRect iconRect(HPaddingXs, 0, m_iconSize.width(), height());
        const int textX = iconRect.right() + 1 + HGapXxs;
        const QRect textRect(textX, VPaddingXs,
                             width() - HPaddingXs - textX, tF.lineHeight());

        QPainter p(this);
        (active ? m_iconActive : m_iconDefault).paint(&p, iconRect);
        p.setPen(tF.color());
        p.setFont(tF.font());
        const QString elidedText = p.fontMetrics().elidedText(currentFormattedText(),
                                                              Qt::ElideRight,
                                                              textRect.width() + HPaddingXs);
        p.drawText(textRect, tF.drawTextFlags, elidedText);
    }

    void enterEvent(QEnterEvent *event) override
    {
        QComboBox::enterEvent(event);
        update();
    }

    void leaveEvent(QEvent *event) override
    {
        QComboBox::leaveEvent(event);
        update();
    }

private:
    QSize sizeHint() const override
    {
        const QFontMetrics fm(m_itemDefaultTf.font());
        const int textWidth = fm.horizontalAdvance(currentFormattedText());
        const int width =
            HPaddingXs
            + m_iconSize.width()
            + HGapXxs
            + textWidth
            + HPaddingXs;
        const int height =
            VPaddingXs
            + m_itemDefaultTf.lineHeight()
            + VPaddingXs;
        return {width, height};
    }

    QString currentFormattedText() const
    {
        return m_textTemplate.arg(currentText());
    }

    constexpr static Theme::Color m_colorDefault = Theme::Token_Text_Muted;
    constexpr static Theme::Color m_colorActive = Theme::Token_Text_Default;
    constexpr static QSize m_iconSize{16, 16};
    constexpr static TextFormat m_itemDefaultTf
        {m_colorDefault, UiElement::UiElementLabelMedium};
    constexpr static TextFormat m_itemActiveTf
        {m_colorActive, m_itemDefaultTf.uiElement};
    const QIcon m_iconDefault;
    const QIcon m_iconActive;
    const QString m_textTemplate;
};

static QString extensionStateDisplayString(ExtensionState state)
{
    switch (state) {
    case InstalledEnabled:
        return Tr::tr("Loaded");
    case InstalledDisabled:
        return Tr::tr("Installed");
    default:
        return {};
    }
    return {};
}

class ExtensionItemDelegate : public QItemDelegate
{
public:
    constexpr static QSize dividerS{1, 16};
    constexpr static TextFormat itemNameTF
        {Theme::Token_Text_Default, UiElement::UiElementH6};
    constexpr static TextFormat countTF
        {Theme::Token_Text_Default, UiElement::UiElementLabelSmall,
         Qt::AlignCenter | Qt::TextDontClip};
    constexpr static TextFormat vendorTF
        {Theme::Token_Text_Muted, UiElement::UiElementLabelSmall,
         Qt::AlignVCenter | Qt::TextDontClip};
    constexpr static TextFormat stateTF
        {vendorTF.themeColor, UiElement::UiElementCaption, vendorTF.drawTextFlags};
    constexpr static TextFormat tagsTF
        {Theme::Token_Text_Default, UiElement::UiElementCaption};

    explicit ExtensionItemDelegate(QObject *parent = nullptr)
        : QItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        // +---------------+-------+---------------+----------------------------------------------------------------------+---------------+---------+
        // |               |       |               |                            (ExPaddingGapL)                           |               |         |
        // |               |       |               +-----------------------------+---------+--------+---------+-----------+               |         |
        // |               |       |               |          <itemName>         |(HGapXxs)|<status>|(HGapXxs)|<checkmark>|               |         |
        // |               |       |               +-----------------------------+---------+--------+---------+-----------+               |         |
        // |               |       |               |                               (VGapXxs)                              |               |         |
        // |               |       |               +--------+--------+--------------+--------+--------+---------+---------+               |         |
        // |(ExPaddingGapL)|<icon> |(ExPaddingGapL)|<vendor>|(HGapXs)|<divider>(h16)|(HGapXs)|<dlIcon>|(HGapXxs)|<dlCount>|(ExPaddingGapL)|(gapSize)|
        // |               |(50x50)|               +--------+--------+--------------+--------+--------+---------+---------+               |         |
        // |               |       |               |                               (VGapXxs)                              |               |         |
        // |               |       |               +----------------------------------------------------------------------+               |         |
        // |               |       |               |                                <tags>                                |               |         |
        // |               |       |               +----------------------------------------------------------------------+               |         |
        // |               |       |               |                            (ExPaddingGapL)                           |               |         |
        // +---------------+-------+---------------+----------------------------------------------------------------------+---------------+---------+
        // |                                                                (gapSize)                                                               |
        // +----------------------------------------------------------------------------------------------------------------------------------------+

        const QRect bgRGlobal = option.rect.adjusted(0, 0, -gapSize, -gapSize);
        const QRect bgR = bgRGlobal.translated(-option.rect.topLeft());

        const int middleColumnW = bgR.width() - ExPaddingGapL - iconBgSizeSmall.width()
                - ExPaddingGapL - ExPaddingGapL;

        int x = bgR.x();
        int y = bgR.y();
        x += ExPaddingGapL;
        const QRect iconBgR(x, y + (bgR.height() - iconBgSizeSmall.height()) / 2,
                            iconBgSizeSmall.width(), iconBgSizeSmall.height());
        x += iconBgSizeSmall.width() + ExPaddingGapL;
        y += ExPaddingGapL;
        const QRect itemNameR(x, y, middleColumnW, itemNameTF.lineHeight());
        const QString itemName = index.data().toString();

        const QSize checkmarkS(12, 12);
        const QRect checkmarkR(x + middleColumnW - checkmarkS.width(), y,
                               checkmarkS.width(), checkmarkS.height());
        const ExtensionState state = index.data(RoleExtensionState).value<ExtensionState>();
        const QString stateString = extensionStateDisplayString(state);
        const bool showState = (state == InstalledEnabled || state == InstalledDisabled)
                && !stateString.isEmpty();
        const QFont stateFont = stateTF.font();
        const QFontMetrics stateFM(stateFont);
        const int stateStringWidth = stateFM.horizontalAdvance(stateString);
        const QRect stateR(checkmarkR.x() - HGapXxs - stateStringWidth, y,
                           stateStringWidth, stateTF.lineHeight());

        y += itemNameR.height() + VGapXxs;
        const QRect vendorRowR(x, y, middleColumnW, vendorRowHeight());
        QRect vendorR = vendorRowR;

        y += vendorRowR.height() + VGapXxs;
        const QRect tagsR(x, y, middleColumnW, tagsTF.lineHeight());

        QTC_CHECK(option.rect.height() - 1 == tagsR.bottom() + ExPaddingGapL + gapSize);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(bgRGlobal.topLeft());

        const bool isPack = index.data(RoleItemType) == ItemTypePack;
        {
            const bool selected = option.state & QStyle::State_Selected;
            const bool hovered = option.state & QStyle::State_MouseOver;
            const QColor fillColor =
                creatorColor(hovered ? WelcomePageHelpers::cardHoverBackground
                                              : WelcomePageHelpers::cardDefaultBackground);
            const QColor strokeColor =
                creatorColor(selected ? Theme::Token_Stroke_Strong
                                      : hovered ? WelcomePageHelpers::cardHoverStroke
                                                : WelcomePageHelpers::cardDefaultStroke);
            WelcomePageHelpers::drawCardBackground(painter, bgR, fillColor, strokeColor);
        }
        {
            const QPixmap icon = itemIcon(index, SizeSmall);
            painter->drawPixmap(iconBgR.topLeft(), icon);
        }
        if (isPack) {
            constexpr int circleSize = 18;
            constexpr int circleOverlap = 3; // Protrusion from lower right corner of iconRect
            const QRect smallCircle(iconBgR.right() + 1 + circleOverlap - circleSize,
                                    iconBgR.bottom() + 1 + circleOverlap - circleSize,
                                    circleSize, circleSize);
            const QColor fillColor = creatorColor(Theme::Token_Foreground_Muted);
            const QColor strokeColor = creatorColor(Theme::Token_Stroke_Subtle);
            drawCardBackground(painter, smallCircle, fillColor, strokeColor, circleSize / 2);

            painter->setFont(countTF.font());
            painter->setPen(countTF.color());
            const PluginsData plugins = index.data(RolePlugins).value<PluginsData>();
            painter->drawText(smallCircle, countTF.drawTextFlags, QString::number(plugins.count()));
        }
        {
            QRect effectiveR = itemNameR;
            if (showState)
                effectiveR.setRight(stateR.left() - HGapXxs - 1);
            painter->setPen(itemNameTF.color());
            painter->setFont(itemNameTF.font());
            const QString titleElided
                = painter->fontMetrics().elidedText(itemName, Qt::ElideRight, effectiveR.width());
            painter->drawText(effectiveR, itemNameTF.drawTextFlags, titleElided);
        }
        if (showState) {
            static const QIcon checkmark = Icon({{":/extensionmanager/images/checkmark.png",
                                                  stateTF.themeColor}}, Icon::Tint).icon();
            checkmark.paint(painter, checkmarkR);
            painter->setPen(stateTF.color());
            painter->setFont(stateTF.font());
            painter->drawText(stateR, stateTF.drawTextFlags, stateString);
        }
        {
            const QString vendor = index.data(RoleVendor).toString();
            const QFontMetrics fm(vendorTF.font());
            painter->setPen(vendorTF.color());
            painter->setFont(vendorTF.font());

            if (const int dlCount = index.data(RoleDownloadCount).toInt(); dlCount > 0) {
                constexpr QSize dlIconS(16, 16);
                const QString dlCountString = QString::number(dlCount);
                const int dlCountW = fm.horizontalAdvance(dlCountString);
                const int dlItemsW = HGapXs + dividerS.width() + HGapXs + dlIconS.width()
                                     + HGapXxs + dlCountW;
                const int vendorW = fm.horizontalAdvance(vendor);
                vendorR.setWidth(qMin(middleColumnW - dlItemsW, vendorW));

                QRect dividerR = vendorRowR;
                dividerR.setLeft(vendorR.right() + HGapXs);
                dividerR.setWidth(dividerS.width());
                painter->fillRect(dividerR, vendorTF.color());

                QRect dlIconR = vendorRowR;
                dlIconR.setLeft(dividerR.right() + HGapXs);
                dlIconR.setWidth(dlIconS.width());
                static const QIcon dlIcon = Icon({{":/extensionmanager/images/download.png",
                                                   vendorTF.themeColor}}, Icon::Tint).icon();
                dlIcon.paint(painter, dlIconR);

                QRect dlCountR = vendorRowR;
                dlCountR.setLeft(dlIconR.right() + HGapXxs);
                painter->drawText(dlCountR, vendorTF.drawTextFlags, dlCountString);
            }

            const QString vendorElided = fm.elidedText(vendor, Qt::ElideRight, vendorR.width());
            painter->drawText(vendorR, vendorTF.drawTextFlags, vendorElided);
        }
        {
            const QStringList tagList = index.data(RoleTags).toStringList();
            const QString tags = tagList.join(", ");
            painter->setPen(tagsTF.color());
            painter->setFont(tagsTF.font());
            const QString tagsElided
                = painter->fontMetrics().elidedText(tags, Qt::ElideRight, tagsR.width());
            painter->drawText(tagsR, tagsTF.drawTextFlags, tagsElided);
        }

        painter->restore();
    }

    static int vendorRowHeight()
    {
        return qMax(vendorTF.lineHeight(), dividerS.height());
    }

    QSize sizeHint([[maybe_unused]] const QStyleOptionViewItem &option,
                   [[maybe_unused]] const QModelIndex &index) const override
    {
        const int middleColumnH =
            itemNameTF.lineHeight()
            + VGapXxs
            + vendorRowHeight()
            + VGapXxs
            + tagsTF.lineHeight();
        const int height =
            ExPaddingGapL
            + qMax(iconBgSizeSmall.height(), middleColumnH)
            + ExPaddingGapL;
        return {cellWidth, height + gapSize};
    }
};

class SortFilterProxyModel : public QSortFilterProxyModel
{
public:
    struct SortOption {
        const QString displayName;
        const Role role;
        const Qt::SortOrder order = Qt::AscendingOrder;
    };

    struct FilterOption {
        const QString displayName;
        const std::function<bool(const QModelIndex &)> indexAcceptedFunc;
    };

    SortFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setSortCaseSensitivity(Qt::CaseInsensitive);
    }

    static const QList<SortOption> &sortOptions()
    {
        static const QList<SortOption> options = {
            {Tr::tr("Name"), RoleName},
            {Tr::tr("Vendor"), RoleVendor},
            {Tr::tr("Popularity"), RoleDownloadCount, Qt::DescendingOrder},
        };
        return options;
    }

    void setSortOption(int index)
    {
        QTC_ASSERT(index < sortOptions().count(), index = 0);
        m_sortOptionIndex = index;
        const SortOption &option = sortOptions().at(index);

        // Ensure some order for cases with insufficient data, e.g. RoleDownloadCount
        setSortRole(RoleName);
        sort(0);
        if (option.role == RoleName)
            return; // Already sorted.

        setSortRole(option.role);
        sort(0, option.order);
    }

    static const QList<FilterOption> &filterOptions()
    {
        static const QList<FilterOption> options = {
            {
                Tr::tr("All"),
                []([[maybe_unused]] const QModelIndex &index) {
                    return true;
                },
            },
            {
                Tr::tr("Extension packs"),
                [](const QModelIndex &index) {
                    return index.data(RoleItemType).value<ItemType>() == ItemTypePack;
                },
            },
            {
                Tr::tr("Individual extensions"),
                [](const QModelIndex &index) {
                    return index.data(RoleItemType).value<ItemType>() == ItemTypeExtension;
                },
            },
        };
        return options;
    }

    void setFilterOption(int index)
    {
        QTC_ASSERT(index < filterOptions().count(), index = 0);
        beginResetModel();
        m_filterOptionIndex = index;
        endResetModel();
    }

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        const SortOption &option = sortOptions().at(m_sortOptionIndex);
        const ItemType leftType = left.data(RoleItemType).value<ItemType>();
        const ItemType rightType = right.data(RoleItemType).value<ItemType>();
        if (leftType != rightType)
            return option.order == Qt::AscendingOrder ? leftType < rightType
                                                      : leftType > rightType;

        return QSortFilterProxyModel::lessThan(left, right);
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
        return filterOptions().at(m_filterOptionIndex).indexAcceptedFunc(index);
    }

    int m_filterOptionIndex = 0;
    int m_sortOptionIndex = 0;
};

class ExtensionsBrowserPrivate
{
public:
    bool dataFetched = false;
    ExtensionsModel *model;
    QLineEdit *searchBox;
    OptionChooser *filterChooser;
    OptionChooser *sortChooser;
    QListView *extensionsView;
    QItemSelectionModel *selectionModel = nullptr;
    QSortFilterProxyModel *searchProxyModel;
    SortFilterProxyModel *sortFilterProxyModel;
    int columnsCount = 2;
    Tasking::TaskTreeRunner taskTreeRunner;
    SpinnerSolution::Spinner *m_spinner;
};

ExtensionsBrowser::ExtensionsBrowser(QWidget *parent)
    : QWidget(parent)
    , d(new ExtensionsBrowserPrivate)
{
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    static const TextFormat titleTF
        {Theme::Token_Text_Default, UiElementH2};
    QLabel *titleLabel = tfLabel(titleTF);
    titleLabel->setText(Tr::tr("Manage Extensions"));

    d->searchBox = new SearchBox;
    d->searchBox->setPlaceholderText(Tr::tr("Search"));

    d->model = new ExtensionsModel(this);

    d->searchProxyModel = new QSortFilterProxyModel(this);
    d->searchProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->searchProxyModel->setFilterRole(RoleSearchText);
    d->searchProxyModel->setSourceModel(d->model);

    d->sortFilterProxyModel = new SortFilterProxyModel(this);
    d->sortFilterProxyModel->setSourceModel(d->searchProxyModel);

    d->filterChooser = new OptionChooser(":/extensionmanager/images/filter.png",
                                         Tr::tr("Filter by: %1"));
    d->filterChooser->addItems(Utils::transform(SortFilterProxyModel::filterOptions(),
                                                &SortFilterProxyModel::FilterOption::displayName));

    d->sortChooser = new OptionChooser(":/extensionmanager/images/sort.png", Tr::tr("Sort by: %1"));
    d->sortChooser->addItems(Utils::transform(SortFilterProxyModel::sortOptions(),
                                              &SortFilterProxyModel::SortOption::displayName));

    d->extensionsView = new QListView;
    d->extensionsView->setFrameStyle(QFrame::NoFrame);
    d->extensionsView->setItemDelegate(new ExtensionItemDelegate(this));
    d->extensionsView->setResizeMode(QListView::Adjust);
    d->extensionsView->setSelectionMode(QListView::SingleSelection);
    d->extensionsView->setUniformItemSizes(true);
    d->extensionsView->setViewMode(QListView::IconMode);
    d->extensionsView->setModel(d->sortFilterProxyModel);
    d->extensionsView->setMouseTracking(true);

    using namespace Layouting;
    Column {
        Column {
            titleLabel,
            customMargins(0, VPaddingM, 0, VPaddingM),
        },
        Row {
            d->searchBox,
            spacing(gapSize),
            customMargins(0, VPaddingM, extraListViewWidth() + gapSize, VPaddingM),
        },
        Row {
            d->filterChooser,
            Space(HGapS),
            d->sortChooser,
            st,
            customMargins(0, 0, extraListViewWidth() + gapSize, 0),
        },
        d->extensionsView,
        noMargin, spacing(0),
    }.attachTo(this);

    WelcomePageHelpers::setBackgroundColor(this, Theme::Token_Background_Default);
    WelcomePageHelpers::setBackgroundColor(d->extensionsView, Theme::Token_Background_Default);
    WelcomePageHelpers::setBackgroundColor(d->extensionsView->viewport(),
                                           Theme::Token_Background_Default);

    d->m_spinner = new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Large, this);
    d->m_spinner->hide();

    auto updateModel = [this] {
        d->sortFilterProxyModel->sort(0);

        if (d->selectionModel == nullptr) {
            d->selectionModel = new QItemSelectionModel(d->sortFilterProxyModel,
                                                          d->extensionsView);
            d->extensionsView->setSelectionModel(d->selectionModel);
            connect(d->extensionsView->selectionModel(), &QItemSelectionModel::currentChanged,
                    this, &ExtensionsBrowser::itemSelected);
        }
    };

    connect(PluginManager::instance(), &PluginManager::pluginsChanged, this, updateModel);
    connect(d->searchBox, &QLineEdit::textChanged,
            d->searchProxyModel, &QSortFilterProxyModel::setFilterWildcard);
    connect(d->sortChooser, &OptionChooser::currentIndexChanged,
            d->sortFilterProxyModel, &SortFilterProxyModel::setSortOption);
    connect(d->filterChooser, &OptionChooser::currentIndexChanged,
            d->sortFilterProxyModel, &SortFilterProxyModel::setFilterOption);
}

ExtensionsBrowser::~ExtensionsBrowser()
{
    delete d;
}

void ExtensionsBrowser::setFilter(const QString &filter)
{
    d->searchBox->setText(filter);
}

void ExtensionsBrowser::adjustToWidth(const int width)
{
    const int widthForItems = width - extraListViewWidth();
    d->columnsCount = qMax(1, qFloor(widthForItems / cellWidth));
    updateGeometry();
}

QSize ExtensionsBrowser::sizeHint() const
{
    const int columsWidth = d->columnsCount * cellWidth;
    return { columsWidth + extraListViewWidth(), 0};
}

int ExtensionsBrowser::extraListViewWidth() const
{
    // TODO: Investigate "transient" scrollbar, just for this list view.
    constexpr int extraPadding = qMax(0, ExVPaddingGapXl - gapSize);
    return d->extensionsView->style()->pixelMetric(QStyle::PM_ScrollBarExtent)
           + extraPadding
           + 1; // Needed
}

void ExtensionsBrowser::showEvent(QShowEvent *event)
{
    if (!d->dataFetched) {
        d->dataFetched = true;
        fetchExtensions();
    }
    QWidget::showEvent(event);
}

static QString customOsTypeToString(OsType osType)
{
    switch (osType) {
    case OsTypeWindows:
        return "Windows";
    case OsTypeLinux:
        return "Linux";
    case OsTypeMac:
        return "macOS";
    case OsTypeOtherUnix:
        return "Other Unix";
    case OsTypeOther:
    default:
        return "Other";
    }
}

void ExtensionsBrowser::fetchExtensions()
{
#ifdef WITH_TESTS
    // Uncomment for testing with local json data.
    // Available: "augmentedplugindata", "defaultpacks", "varieddata", "thirdpartyplugins"
    // d->model->setExtensionsJson(testData("defaultpacks")); return;
#endif // WITH_TESTS

    if (!settings().useExternalRepo()) {
        d->model->setExtensionsJson({});
        return;
    }

    using namespace Tasking;

    const auto onQuerySetup = [this](NetworkQuery &query) {
        const QString url = "%1/api/v1/search?request=";
        const QString requestTemplate
            = R"({"qtc_version":"%1","host_os":"%2","host_os_version":"%3","host_architecture":"%4","page_size":200})";
        const QString request = url.arg(settings().externalRepoUrl()) + requestTemplate
                                                    .arg(QCoreApplication::applicationVersion())
                                                    .arg(customOsTypeToString(HostOsInfo::hostOs()))
                                                    .arg(QSysInfo::productVersion())
                                                    .arg(QSysInfo::currentCpuArchitecture());
        query.setRequest(QNetworkRequest(QUrl::fromUserInput(request)));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        qCDebug(browserLog).noquote() << "Sending JSON request:" << request;
        d->m_spinner->show();
    };
    const auto onQueryDone = [this](const NetworkQuery &query, DoneWith result) {
        const QByteArray response = query.reply()->readAll();
        qCDebug(browserLog).noquote() << "Got JSON QNetworkReply:" << query.reply()->error();
        if (result == DoneWith::Success) {
            qCDebug(browserLog).noquote() << "JSON response size:"
                                          << QLocale::system().formattedDataSize(response.size());
            d->model->setExtensionsJson(response);
        } else {
            qCWarning(browserLog).noquote() << response;
            d->model->setExtensionsJson({});
        }
        d->m_spinner->hide();
    };

    Group group {
        NetworkQueryTask{onQuerySetup, onQueryDone},
    };

    d->taskTreeRunner.start(group);
}

QLabel *tfLabel(const TextFormat &tf, bool singleLine)
{
    QLabel *label = singleLine ? new Utils::ElidingLabel : new QLabel;
    if (singleLine)
        label->setFixedHeight(tf.lineHeight());
    label->setFont(tf.font());
    label->setAlignment(Qt::Alignment(tf.drawTextFlags));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, tf.color());
    label->setPalette(pal);

    return label;
}

QPixmap itemIcon(const QModelIndex &index, Size size)
{
    const QSize iconBgS = size == SizeSmall ? iconBgSizeSmall : iconBgSizeBig;
    const qreal dpr = qApp->devicePixelRatio();
    QPixmap pixmap(iconBgS * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);
    const QRect iconBgR(QPoint(), pixmap.deviceIndependentSize().toSize());

    const PluginSpec *ps = pluginSpecForName(index.data(RoleName).toString());
    const bool isEnabled = ps == nullptr || ps->isEffectivelyEnabled();
    const QGradientStops gradientStops = {
        {0, creatorColor(isEnabled ? Theme::Token_Gradient01_Start
                                   : Theme::Token_Gradient02_Start)},
        {1, creatorColor(isEnabled ? Theme::Token_Gradient01_End
                                   : Theme::Token_Gradient02_End)},
    };

    const Theme::Color color = Theme::Token_Basic_White;
    static const QIcon packS = Icon({{":/extensionmanager/images/packsmall.png", color}},
                                    Icon::Tint).icon();
    static const QIcon packB = Icon({{":/extensionmanager/images/packbig.png", color}},
                                    Icon::Tint).icon();
    static const QIcon extensionS = Icon({{":/extensionmanager/images/extensionsmall.png",
                                           color}}, Icon::Tint).icon();
    static const QIcon extensionB = Icon({{":/extensionmanager/images/extensionbig.png",
                                           color}}, Icon::Tint).icon();
    const ItemType itemType = index.data(RoleItemType).value<ItemType>();
    const QIcon &icon = (itemType == ItemTypePack) ? (size == SizeSmall ? packS : packB)
                                                   : (size == SizeSmall ? extensionS : extensionB);
    const int iconRectRounding = 4;
    const qreal iconOpacityDisabled = 0.6;

    QPainter p(&pixmap);
    QLinearGradient gradient(iconBgR.topRight(), iconBgR.bottomLeft());
    gradient.setStops(gradientStops);
    WelcomePageHelpers::drawCardBackground(&p, iconBgR, gradient, Qt::NoPen, iconRectRounding);
    if (!isEnabled)
        p.setOpacity(iconOpacityDisabled);
    icon.paint(&p, iconBgR);

    return pixmap;
}

} // ExtensionManager::Internal
