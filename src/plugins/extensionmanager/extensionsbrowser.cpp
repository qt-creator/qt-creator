// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionsbrowser.h"

#include "extensionmanagertr.h"
#include "extensionsmodel.h"
#include "utils/hostosinfo.h"

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

#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/fancylineedit.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/stylehelper.h>

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

constexpr int gapSize = ExVPaddingGapXl;
constexpr int itemWidth = 330;
constexpr int cellWidth = itemWidth + HPaddingL;

class ExtensionItemDelegate : public QItemDelegate
{
public:
    constexpr static QSize dividerS{1, 16};
    constexpr static QSize iconBgS{50, 50};
    constexpr static TextFormat itemNameTF
        {Theme::Token_Text_Default, UiElement::UiElementH6};
    constexpr static TextFormat countTF
        {Theme::Token_Text_Default, UiElement::UiElementLabelSmall,
         Qt::AlignCenter | Qt::TextDontClip};
    constexpr static TextFormat vendorTF
        {Theme::Token_Text_Muted, UiElement::UiElementLabelSmall,
         Qt::AlignVCenter | Qt::TextDontClip};
    constexpr static TextFormat tagsTF
        {Theme::Token_Text_Default, UiElement::UiElementCaption};

    explicit ExtensionItemDelegate(QObject *parent = nullptr)
        : QItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        // +---------------+-------+---------------+----------------------------------------------------------------------+---------------+-----------+
        // |               |       |               |                            (ExPaddingGapL)                           |               |           |
        // |               |       |               +-------------------------------------------------------------+--------+               |           |
        // |               |       |               |                          <itemName>                         |<status>|               |           |
        // |               |       |               +-------------------------------------------------------------+--------+               |           |
        // |               |       |               |                               (VGapXxs)                              |               |           |
        // |               |       |               +--------+--------+--------------+--------+--------+---------+---------+               |           |
        // |(ExPaddingGapL)|<icon> |(ExPaddingGapL)|<vendor>|(HGapXs)|<divider>(h16)|(HGapXs)|<dlIcon>|(HGapXxs)|<dlCount>|(ExPaddingGapL)|(HPaddingL)|
        // |               |(50x50)|               +--------+--------+--------------+--------+--------+---------+---------+               |           |
        // |               |       |               |                               (VGapXxs)                              |               |           |
        // |               |       |               +----------------------------------------------------------------------+               |           |
        // |               |       |               |                                <tags>                                |               |           |
        // |               |       |               +----------------------------------------------------------------------+               |           |
        // |               |       |               |                            (ExPaddingGapL)                           |               |           |
        // +---------------+-------+---------------+----------------------------------------------------------------------+---------------+-----------+
        // |                                                             (ExVPaddingGapXl)                                                            |
        // +------------------------------------------------------------------------------------------------------------------------------------------+

        const QRect bgRGlobal = option.rect.adjusted(0, 0, -HPaddingL, -gapSize);
        const QRect bgR = bgRGlobal.translated(-option.rect.topLeft());

        const int middleColumnW = bgR.width() - ExPaddingGapL - iconBgS.width() - ExPaddingGapL
                                  - ExPaddingGapL;

        int x = bgR.x();
        int y = bgR.y();
        x += ExPaddingGapL;
        const QRect iconBgR(x, y + (bgR.height() - iconBgS.height()) / 2,
                            iconBgS.width(), iconBgS.height());
        x += iconBgS.width() + ExPaddingGapL;
        y += ExPaddingGapL;
        const QRect itemNameR(x, y, middleColumnW, itemNameTF.lineHeight());
        const QString itemName = index.data().toString();

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
            QLinearGradient gradient(iconBgR.topRight(), iconBgR.bottomLeft());
            const QColor startColor = creatorColor(Utils::Theme::Token_Gradient01_Start);
            const QColor endColor = creatorColor(Utils::Theme::Token_Gradient01_End);
            gradient.setColorAt(0, startColor);
            gradient.setColorAt(1, endColor);
            constexpr int iconRectRounding = 4;
            drawCardBackground(painter, iconBgR, gradient, Qt::NoPen, iconRectRounding);

            // Icon
            constexpr Theme::Color color = Theme::Token_Basic_White;
            static const QIcon pack = Icon({{":/extensionmanager/images/packsmall.png", color}},
                                           Icon::Tint).icon();
            static const QIcon extension = Icon({{":/extensionmanager/images/extensionsmall.png",
                                                  color}}, Icon::Tint).icon();
            (isPack ? pack : extension).paint(painter, iconBgR);
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
            painter->setPen(itemNameTF.color());
            painter->setFont(itemNameTF.font());
            const QString titleElided
                = painter->fontMetrics().elidedText(itemName, Qt::ElideRight, itemNameR.width());
            painter->drawText(itemNameR, itemNameTF.drawTextFlags, titleElided);
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
            + qMax(iconBgS.height(), middleColumnH)
            + ExPaddingGapL;
        return {cellWidth, height + gapSize};
    }
};

class ExtensionsBrowserPrivate
{
public:
    ExtensionsModel *model;
    QLineEdit *searchBox;
    QAbstractButton *updateButton;
    QListView *extensionsView;
    QItemSelectionModel *selectionModel = nullptr;
    QSortFilterProxyModel *filterProxyModel;
    int columnsCount = 2;
    Tasking::TaskTreeRunner taskTreeRunner;
};

ExtensionsBrowser::ExtensionsBrowser(QWidget *parent)
    : QWidget(parent)
    , d(new ExtensionsBrowserPrivate)
{
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    auto manageLabel = new QLabel(Tr::tr("Manage Extensions"));
    manageLabel->setFont(uiFont(UiElementH1));

    d->searchBox = new SearchBox;
    d->searchBox->setFixedWidth(itemWidth);
    d->updateButton = new Button(Tr::tr("Install..."), Button::MediumPrimary);

    d->model = new ExtensionsModel(this);

    d->filterProxyModel = new QSortFilterProxyModel(this);
    d->filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->filterProxyModel->setFilterRole(RoleSearchText);
    d->filterProxyModel->setSortRole(RoleItemType);
    d->filterProxyModel->setSourceModel(d->model);

    d->extensionsView = new QListView;
    d->extensionsView->setFrameStyle(QFrame::NoFrame);
    d->extensionsView->setItemDelegate(new ExtensionItemDelegate(this));
    d->extensionsView->setResizeMode(QListView::Adjust);
    d->extensionsView->setSelectionMode(QListView::SingleSelection);
    d->extensionsView->setUniformItemSizes(true);
    d->extensionsView->setViewMode(QListView::IconMode);
    d->extensionsView->setModel(d->filterProxyModel);
    d->extensionsView->setMouseTracking(true);

    using namespace Layouting;
    Column {
        Space(15),
        manageLabel,
        Space(15),
        Row { d->searchBox, st, d->updateButton, Space(extraListViewWidth() + gapSize) },
        Space(gapSize),
        d->extensionsView,
        noMargin, spacing(0),
    }.attachTo(this);

    WelcomePageHelpers::setBackgroundColor(this, Theme::Token_Background_Default);
    WelcomePageHelpers::setBackgroundColor(d->extensionsView, Theme::Token_Background_Default);
    WelcomePageHelpers::setBackgroundColor(d->extensionsView->viewport(),
                                           Theme::Token_Background_Default);

    auto updateModel = [this] {
        d->filterProxyModel->sort(0);

        if (d->selectionModel == nullptr) {
            d->selectionModel = new QItemSelectionModel(d->filterProxyModel,
                                                          d->extensionsView);
            d->extensionsView->setSelectionModel(d->selectionModel);
            connect(d->extensionsView->selectionModel(), &QItemSelectionModel::currentChanged,
                    this, &ExtensionsBrowser::itemSelected);
        }
    };

    connect(d->updateButton, &QAbstractButton::pressed, this, []() {
        executePluginInstallWizard();
    });
    connect(PluginManager::instance(), &PluginManager::pluginsChanged, this, updateModel);
    connect(PluginManager::instance(), &PluginManager::initializationDone,
            this, &ExtensionsBrowser::fetchExtensions);
    connect(d->searchBox, &QLineEdit::textChanged,
            d->filterProxyModel, &QSortFilterProxyModel::setFilterWildcard);
}

ExtensionsBrowser::~ExtensionsBrowser()
{
    delete d;
}

void ExtensionsBrowser::adjustToWidth(const int width)
{
    const int widthForItems = width - extraListViewWidth();
    d->columnsCount = qMax(1, qFloor(widthForItems / cellWidth));
    d->updateButton->setVisible(d->columnsCount > 1);
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
    return d->extensionsView->style()->pixelMetric(QStyle::PM_ScrollBarExtent)
           + 1; // Needed
}

void ExtensionsBrowser::fetchExtensions()
{
    // d->model->setExtensionsJson(testData("thirdpartyplugins")); return;

    using namespace Tasking;

    const auto onQuerySetup = [](NetworkQuery &query) {
        const QString host = "https://qc-extensions.qt.io";
        const QString url = "%1/api/v1/search?request=";
        const QString requestTemplate
            = R"({"version":"%1","host_os":"%2","host_os_version":"%3","host_architecture":"%4","page_size":200})";
        const QString request = url.arg(host)
                                + requestTemplate
                                      .arg("2.2")    // .arg(QCoreApplication::applicationVersion())
                                      .arg("macOS")  // .arg(QSysInfo::productType())
                                      .arg("12")     // .arg(QSysInfo::productVersion())
                                      .arg("arm64"); // .arg(QSysInfo::currentCpuArchitecture());

        query.setRequest(QNetworkRequest(QUrl::fromUserInput(request)));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };
    const auto onQueryDone = [this](const NetworkQuery &query, DoneWith result) {
        if (result != DoneWith::Success) {
#ifdef WITH_TESTS
            d->model->setExtensionsJson(testData("defaultpacks"));
#endif // WITH_TESTS
            return;
        }
        const QByteArray response = query.reply()->readAll();
        d->model->setExtensionsJson(response);
    };

    Group group {
        NetworkQueryTask{onQuerySetup, onQueryDone},
    };

    d->taskTreeRunner.start(group);
}

} // ExtensionManager::Internal
