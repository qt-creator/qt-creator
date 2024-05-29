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

using namespace ExtensionSystem;
using namespace Core;
using namespace Utils;

namespace ExtensionManager::Internal {

Q_LOGGING_CATEGORY(browserLog, "qtc.extensionmanager.browser", QtWarningMsg)

constexpr QSize itemSize = {330, 86};
constexpr int gapSize = StyleHelper::SpacingTokens::ExVPaddingGapXl;
constexpr QSize cellSize = {itemSize.width() + gapSize, itemSize.height() + gapSize};

static QColor colorForExtensionName(const QString &name)
{
    const size_t hash = qHash(name);
    return QColor::fromHsv(hash % 360, 180, 110);
}

class ExtensionItemDelegate : public QItemDelegate
{
public:
    explicit ExtensionItemDelegate(QObject *parent = nullptr)
        : QItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        const QString itemName = index.data().toString();
        const bool isPack = index.data(RoleItemType) == ItemTypePack;
        const QRectF itemRect(option.rect.topLeft(), itemSize);
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
            WelcomePageHelpers::drawCardBackground(painter, itemRect, fillColor, strokeColor);
        }
        {
            constexpr QRectF bigCircle(16, 16, 48, 48);
            constexpr double gradientMargin = 0.14645;
            const QRectF bigCircleLocal = bigCircle.translated(itemRect.topLeft());
            QPainterPath bigCirclePath;
            bigCirclePath.addEllipse(bigCircleLocal);
            QLinearGradient gradient(bigCircleLocal.topLeft(), bigCircleLocal.bottomRight());
            const QColor startColor = isPack ? qRgb(0x1e, 0x99, 0x6e)
                                             : colorForExtensionName(itemName);
            const QColor endColor = isPack ? qRgb(0x07, 0x6b, 0x6d) : startColor.lighter(150);
            gradient.setColorAt(gradientMargin, startColor);
            gradient.setColorAt(1 - gradientMargin, endColor);
            painter->fillPath(bigCirclePath, gradient);

            static const QIcon packIcon =
                Icon({{":/extensionmanager/images/packsmall.png",
                       Theme::Token_Text_Default}}, Icon::Tint).icon();
            static const QIcon extensionIcon =
                Icon({{":/extensionmanager/images/extensionsmall.png",
                       Theme::Token_Text_Default}}, Icon::Tint).icon();
            QRectF iconRect(0, 0, 32, 32);
            iconRect.moveCenter(bigCircleLocal.center());
            (isPack ? packIcon : extensionIcon).paint(painter, iconRect.toRect());
        }
        if (isPack) {
            constexpr QRectF smallCircle(47, 50, 18, 18);
            constexpr qreal strokeWidth = 1;
            constexpr qreal shrink = strokeWidth / 2;
            constexpr QRectF smallCircleAdjusted = smallCircle.adjusted(shrink, shrink,
                                                                        -shrink, -shrink);
            const QRectF smallCircleLocal = smallCircleAdjusted.translated(itemRect.topLeft());
            const QColor fillColor = creatorColor(Theme::Token_Foreground_Muted);
            const QColor strokeColor = creatorColor(Theme::Token_Stroke_Subtle);
            painter->setBrush(fillColor);
            painter->setPen(strokeColor);
            painter->drawEllipse(smallCircleLocal);

            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementCaptionStrong));
            const QColor textColor = creatorColor(Theme::Token_Text_Default);
            painter->setPen(textColor);
            const PluginsData plugins = index.data(RolePlugins).value<PluginsData>();
            painter->drawText(
                smallCircleLocal,
                QString::number(plugins.count()),
                QTextOption(Qt::AlignCenter));
        }
        {
            constexpr int textX = 80;
            constexpr int rightMargin = StyleHelper::SpacingTokens::ExVPaddingGapXl;
            constexpr int maxTextWidth = itemSize.width() - textX - rightMargin;
            constexpr Qt::TextElideMode elideMode = Qt::ElideRight;

            constexpr int titleY = 30;
            const QPointF titleOrigin(itemRect.topLeft() + QPointF(textX, titleY));
            painter->setPen(creatorColor(Theme::Token_Text_Default));
            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementH6));
            const QString titleElided
                = painter->fontMetrics().elidedText(itemName, elideMode, maxTextWidth);
            painter->drawText(titleOrigin, titleElided);

            constexpr int copyrightY = 52;
            const QPointF copyrightOrigin(itemRect.topLeft() + QPointF(textX, copyrightY));
            painter->setPen(creatorColor(Theme::Token_Text_Muted));
            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementCaptionStrong));
            const QString copyright = index.data(RoleCopyright).toString();
            const QString copyrightElided
                = painter->fontMetrics().elidedText(copyright, elideMode, maxTextWidth);
            painter->drawText(copyrightOrigin, copyrightElided);

            constexpr int tagsY = 70;
            const QPointF tagsOrigin(itemRect.topLeft() + QPointF(textX, tagsY));
            const QString tags = index.data(RoleTags).toStringList().join(", ");
            painter->setPen(creatorColor(Theme::Token_Text_Default));
            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementCaption));
            const QString tagsElided = painter->fontMetrics().elidedText(
                tags, elideMode, maxTextWidth);
            painter->drawText(tagsOrigin, tagsElided);
        }

        painter->restore();
    }

    QSize sizeHint([[maybe_unused]] const QStyleOptionViewItem &option,
                   [[maybe_unused]] const QModelIndex &index) const override
    {
        return cellSize;
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
    manageLabel->setFont(StyleHelper::uiFont(StyleHelper::UiElementH1));

    d->searchBox = new Core::SearchBox;
    d->searchBox->setFixedWidth(itemSize.width());

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
    connect(ExtensionSystem::PluginManager::instance(),
            &ExtensionSystem::PluginManager::pluginsChanged, this, updateModel);
    connect(ExtensionSystem::PluginManager::instance(),
            &ExtensionSystem::PluginManager::initializationDone,
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
    d->columnsCount = qMax(1, qFloor(widthForItems / cellSize.width()));
    d->updateButton->setVisible(d->columnsCount > 1);
    updateGeometry();
}

QSize ExtensionsBrowser::sizeHint() const
{
    const int columsWidth = d->columnsCount * cellSize.width();
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
