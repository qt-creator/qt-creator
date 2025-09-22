// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "overviewwelcomepage.h"

#include "learningtr.h"
#include "learningsettings.h"
#include "onboardingwizard.h"

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/overlaywidget.h>
#include <utils/qtcwidgets.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/welcomepagehelper.h>

#include <extensionmanager/extensionmanagerlegalnotice.h>

#include <projectexplorer/projectexplorer.h>

#include <qtsupport/qtversionmanager.h>

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QLoggingCategory>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QRadioButton>
#include <QVariantAnimation>
#include <QXmlStreamReader>

#ifdef WITH_TESTS
#include <QTest>
#endif // WITH_TESTS

using namespace Core;
using namespace Utils;
using namespace Utils::StyleHelper;
using namespace QtSupport;

Q_LOGGING_CATEGORY(qtWelcomeOverviewLog, "qtc.welcomeoverview", QtWarningMsg)

namespace Learning::Internal {

const int radiusS = StyleHelper::SpacingTokens::PrimitiveXs;
const int radiusL = StyleHelper::SpacingTokens::PrimitiveS;
constexpr QSize blogThumbSize(450, 192);

using OverviewItems = QList<class OverviewItem*>;
class OverviewItem : public ListItem
{
public:
    enum ItemType {
        Example,
        Tutorial,
        Course,
        Blogpost,
    };

    static QString displayName(OverviewItem::ItemType itemType)
    {
        switch (itemType) {
        case OverviewItem::Example: return Tr::tr("Example");
        case OverviewItem::Tutorial: return Tr::tr("Tutorial");
        case OverviewItem::Course: return Tr::tr("Course");
        case OverviewItem::Blogpost: return Tr::tr("Blog post");
        }
        Q_UNREACHABLE_RETURN({});
    }

    static ItemType itemType(const QString &string)
    {
        if (string == "example")
            return OverviewItem::Example;
        if (string == "tutorial")
            return OverviewItem::Tutorial;
        if (string == "course")
            return OverviewItem::Course;
        if (string == "blogpost")
            return OverviewItem::Blogpost;
        qCDebug(qtWelcomeOverviewLog) << "Invalid item type: " << string;
        return OverviewItem::Example;
    };

    static FilePath jsonFile()
    {
        const QString path = qtcEnvironmentVariable("QTC_LEARNING_RECOMMENDATIONSDIR",
                                                    ":/learning/overview/");
        return FilePath::fromUserInput(path) / "recommendations.json";
    }

    static QList<ListItem *> items(const QSet<ItemType> &types)
    {
        const Result<QByteArray> json = OverviewItem::jsonFile().fileContents();
        if (!json) {
            qCWarning(qtWelcomeOverviewLog).noquote() << json.error() << OverviewItem::jsonFile();
            return {};
        }
        qCDebug(qtWelcomeOverviewLog).noquote()
            << "Reading" << types << "from" << OverviewItem::jsonFile();
        return itemsFromJson(json->data(), types);
    }

    static void openExample(const OverviewItem *item)
    {
        QtVersionManager::openExampleProject(item->id, item->name);
    }

    static void handleClicked(const OverviewItem *item)
    {
        switch (item->type) {
        case Blogpost:
        case Course: // TODO: switch to courses page and show built-in course details widget
            QDesktopServices::openUrl(item->id);
            break;
        case Example:
            openExample(item);
            break;
        case Tutorial:
            HelpManager::showHelpUrl(QUrl::fromUserInput(item->id),
                                     HelpManager::ExternalHelpAlways);
            break;
        }
    }

    static bool validByFlags(const QStringList &userFlags, const QStringList &itemFlags)
    {
        if (itemFlags.isEmpty() || userFlags.isEmpty())
            return true;
        const FlagMap userFlagMap = flagListToMap(userFlags);
        FlagMap itemFlagMap = flagListToMap(itemFlags);
        for (const QString &flag : itemFlagMap.keys()) {
            if (!userFlagMap.contains(flag))
                return false;
            const QStringList &userSubFlags = userFlagMap.value(flag);
            const QStringList itemSubFlags = itemFlagMap.value(flag);
            for (const QString &itemSubFlag : itemSubFlags) {
                if (userSubFlags.contains(itemSubFlag)) {
                    itemFlagMap.remove(flag);
                    break;
                }
            }
        }
        return itemFlagMap.isEmpty();
    }

    ItemType type = Example;
    QString id; // Could be some kind of ID, or an Url
    QStringList flags;

private:
    struct ExampleData
    {
        FilePath project;
        FilePaths toOpen;
        FilePath mainFile;
        FilePaths dependencies;
        QUrl docUrl;
    };
    static QList<ListItem *> itemsFromJson(const QByteArray &json, const QSet<ItemType> &types)
    {
        QJsonParseError error;
        const QJsonObject jsonObj = QJsonDocument::fromJson(json, &error).object();
        if (error.error != QJsonParseError::NoError)
            qCDebug(qtWelcomeOverviewLog) << "QJsonParseError:" << error.errorString();
        const QJsonArray overviewItems = jsonObj.value("items").toArray();

        QList<ListItem *> items;
        for (const auto overviewItem : overviewItems) {
            const QJsonObject overviewItemObj = overviewItem.toObject();
            const QString itemTypeString = overviewItemObj.value("type").toString();
            const OverviewItem::ItemType type = OverviewItem::itemType(itemTypeString);
            if (!types.contains(type))
                continue;
            const bool idIsUrl = type == Course || type == Blogpost;
            const QString itemId = overviewItemObj.value(QLatin1String(idIsUrl ? "id_url"
                                                                               : "id")).toString();
            const QString itemName = overviewItemObj.value("name").toString();
            QString description = overviewItemObj.value("description").toString();
            if (type == OverviewItem::Example) {
                const std::optional<QString> exampleDescription
                    = QtVersionManager::getExampleDescription(itemId, itemName);
                if (!exampleDescription) {
                    qCDebug(qtWelcomeOverviewLog) << "Excluding" << itemTypeString << itemName
                                                  << "because it is not installed.";
                    continue;
                }
                description = *exampleDescription;
            }
            const QStringList itemFlags = overviewItemObj.value("flags").toVariant().toStringList();
            if (!validByFlags(itemFlags)) {
                qCDebug(qtWelcomeOverviewLog) << "Excluding" << itemTypeString << itemName
                                              << "due to flags:" << itemFlags;
                continue;
            }

            auto item = new OverviewItem;
            item->id = itemId;
            item->name = itemName;
            item->type = type;
            const FilePath imageUrl = FilePath::fromSettings(overviewItemObj.value("thumbnail"));
            const FilePath resolvedImageUrl =
                imageUrl.isAbsolutePath() ? imageUrl : jsonFile().parentDir().resolvePath(imageUrl);
            item->imageUrl = StyleHelper::dpiSpecificImageFile(resolvedImageUrl.toFSPathString());
            item->description = description;
            item->flags = itemFlags;
            items.append(item);
        }
        return items;
    }

    using FlagMap = QMap<QString, QStringList>;
    // "Flags" consist of "key_flag"
    static FlagMap flagListToMap(const QStringList &itemFlags)
    {
        FlagMap result;
        for (const QString &flagString : itemFlags) {
            const QStringList keyAndFlag = flagString.split("_");
            if (keyAndFlag.count() != 2)
                continue;
            const QString key = keyAndFlag.first();
            QStringList flags = result.value(key);
            flags.append(keyAndFlag.at(1));
            result.insert(key, flags);
        }
        return result;
    }

    static bool validByFlags(const QStringList &itemFlags)
    {
        return validByFlags(settings().userFlags(), itemFlags);
    }

};

class BlogButton : public QAbstractButton
{
public:
    BlogButton(const FilePath &mask, QWidget *parent = nullptr)
        : QAbstractButton(parent)
    {
        m_icon = Icon({{mask, Theme::Token_Text_Muted}}, Icon::Tint).pixmap();
        setAttribute(Qt::WA_Hover);
        setCursor(Qt::ArrowCursor);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);
        QPainter p(this);

        const QColor bgFill = creatorColor(Theme::Token_Background_Muted);
        p.setOpacity(underMouse() ? 1 : 0.6);
        drawCardBg(&p, rect(), bgFill, Qt::NoPen, radiusS);
        p.setOpacity(1);

        const QSizeF iconSize = m_icon.deviceIndependentSize();
        const QPoint iconPos((width() - iconSize.width()) / 2,
                             (height() - iconSize.height()) / 2);
        p.drawPixmap(iconPos, m_icon);
    };

private:
    QPixmap m_icon;
};

class BlogCarousel : public QWidget
{
    Q_OBJECT

public:
    BlogCarousel(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_Hover);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(blogThumbSize);

        m_animation.setStartValue(0.0);
        m_animation.setEndValue(1.0);

        const int btnS = 32;
        const int btnPad = SpacingTokens::PaddingHM;
        m_prevBtn = new BlogButton(FilePath::fromUserInput(":/utils/images/prev.png"), this);
        m_prevBtn->setGeometry(btnPad, (blogThumbSize.height() - btnS) / 2, btnS, btnS);
        m_prevBtn->hide();
        m_prevBtn->setToolTip(Tr::tr("Previous blog post"));
        m_nextBtn = new BlogButton(FilePath::fromUserInput(":/utils/images/next.png"), this);
        m_nextBtn->setGeometry(blogThumbSize.width() - btnPad - btnS, m_prevBtn->y(), btnS, btnS);
        m_nextBtn->hide();
        m_nextBtn->setToolTip(Tr::tr("Next blog post"));

        connect(m_prevBtn, &QAbstractButton::pressed, this, &BlogCarousel::prevPressed);
        connect(m_nextBtn, &QAbstractButton::pressed, this, &BlogCarousel::nextPressed);
    }

    void setThumbnail(const FilePath &path)
    {
        m_previousPixmap = m_currentPixmap;
        m_currentPixmap = QPixmap(path.toFSPathString());
        if (!m_currentPixmap.isNull()) {
            m_currentPixmap = m_currentPixmap.scaled(blogThumbSize * devicePixelRatio(),
                                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_currentPixmap.setDevicePixelRatio(devicePixelRatio());
        }
        if (!m_previousPixmap.isNull()) {
            m_animation.stop();
            m_animation.start();
        }
        update();
    }

signals:
    void prevPressed();
    void nextPressed();
    void thumbnailPressed();

protected:
    void enterEvent(QEnterEvent *event) override
    {
        QWidget::enterEvent(event);
        if (m_currentPixmap.isNull())
            return;
        m_nextBtn->show();
        m_prevBtn->show();
    }

    void leaveEvent(QEvent *event) override
    {
        QWidget::leaveEvent(event);
        m_nextBtn->hide();
        m_prevBtn->hide();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        emit thumbnailPressed();
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);

        QPainter p(this);
        QPainterPath clipPath;
        clipPath.addRoundedRect(rect(), radiusS, radiusS);
        p.setClipPath(clipPath);
        p.setRenderHint(QPainter::Antialiasing);
        if (!m_previousPixmap.isNull() && m_animation.state() == QAbstractAnimation::Running) {
            p.drawPixmap(0, 0, m_previousPixmap);
            p.setOpacity(m_animation.currentValue().toReal());
            update();
        }
        p.drawPixmap(0, 0, m_currentPixmap);
    }

private:
    QPixmap m_currentPixmap;
    QPixmap m_previousPixmap;
    QVariantAnimation m_animation;
    BlogButton *m_prevBtn;
    BlogButton *m_nextBtn;
};

class BlogWidget : public QWidget
{
public:
    BlogWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_carousel = new BlogCarousel;
        m_title = new Utils::ElidingLabel;
        applyTf(m_title, {.themeColor = Theme::Token_Text_Default,
                          .uiElement = StyleHelper::UiElementH4});

        m_pageIndicator = new QtcPageIndicator;

        using namespace Layouting;
        Column {
            m_carousel,
            m_title,
            Row {
                Space(67), // HACK: compensate Button, to have pageIndicator centered
                st,
                Column { st, m_pageIndicator, st },
                st,
                QtcWidgets::Button {
                    text(tr("Show All")),
                    role(QtcButton::LargeTertiary),
                    onClicked(this, [] {
                        QDesktopServices::openUrl(QUrl::fromUserInput("https://www.qt.io/blog"));
                    }),
                },
            },
            noMargin, spacing(SpacingTokens::GapVM),
        }.attachTo(this);

        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        connect(m_carousel, &BlogCarousel::prevPressed, this,
                [this]{ setCurrentIndex(m_currentIndex - 1); });
        connect(m_carousel, &BlogCarousel::nextPressed, this,
                [this]{ setCurrentIndex(m_currentIndex + 1); });
        connect(m_carousel, &BlogCarousel::thumbnailPressed, this,
                [this]{
                    if (m_items.isEmpty())
                        return;
                    const auto overviewItem =
                        dynamic_cast<OverviewItem*>(m_items.at(m_currentIndex));
                    QTC_ASSERT(overviewItem, return);
                    OverviewItem::handleClicked(overviewItem);
                });

        updateItems();
    }

    ~BlogWidget()
    {
        clearItems();
    }

private:
    void updateItems()
    {
        clearItems();
        m_items = OverviewItem::items({OverviewItem::Blogpost});
        m_pageIndicator->setPagesCount(m_items.count());

        setCurrentIndex(0);
    }

    void setCurrentIndex(int current)
    {
        if (m_items.isEmpty())
            return;
        m_currentIndex = (m_items.count() + current) % m_items.count();
        m_pageIndicator->setCurrentPage(m_currentIndex);
        const auto item = dynamic_cast<OverviewItem*>(m_items.at(m_currentIndex));
        QTC_ASSERT(item, return);
        m_carousel->setThumbnail(FilePath::fromUserInput(item->imageUrl));
        m_carousel->setToolTip(item->id);
        m_title->setText(item->name);
    }

    void clearItems()
    {
        qDeleteAll(m_items);
        m_items.clear();
    }

    QList<ListItem *> m_items;
    int m_currentIndex = -1;
    BlogCarousel *m_carousel;
    QLabel *m_title;
    QtcPageIndicator *m_pageIndicator;
};

class OverviewItemDelegate : public ListItemDelegate
{
protected:
    void clickAction(const ListItem *item) const override
    {
        const auto overviewItem = dynamic_cast<const OverviewItem*>(item);
        QTC_ASSERT(overviewItem, return);
        OverviewItem::handleClicked(overviewItem);
    }

    void drawPixmapOverlay(const ListItem *item, QPainter *painter,
                           [[maybe_unused]] const QStyleOptionViewItem &option,
                           [[maybe_unused]] const QRect &currentPixmapRect) const override
    {
        const auto overviewItem = dynamic_cast<const OverviewItem*>(item);
        QTC_ASSERT(overviewItem, return);
        const QString badgeText = OverviewItem::displayName(overviewItem->type);
        constexpr TextFormat badgeTF
            {Theme::Token_Basic_White, UiElement::UiElementLabelSmall};
        const QFont font = badgeTF.font();
        const int textWidth = QFontMetrics(font).horizontalAdvance(badgeText);
        const QRectF badgeR(1, 1, SpacingTokens::PaddingHS + textWidth + SpacingTokens::PaddingHS,
                            SpacingTokens::PaddingVXs + badgeTF.lineHeight()
                                + SpacingTokens::PaddingVXs);
        drawCardBg(painter, badgeR, creatorColor(Theme::Token_Notification_Success_Muted),
                   Qt::NoPen, radiusL);
        painter->setFont(font);
        painter->setPen(badgeTF.color());
        painter->drawText(badgeR, Qt::AlignCenter, badgeText);
    }
};

class RecommendationsWidget final : public QWidget
{
public:
    RecommendationsWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_view = new GridView;
        m_view->setModel(&m_model);
        m_view->setItemDelegate(&m_delegate);
        m_model.setPixmapFunction(pixmapFromFile);

        using namespace Layouting;
        Column {
            m_view,
            noMargin,
        }.attachTo(this);

        updateModel();
        connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
                this, &RecommendationsWidget::updateModel);
        connect(&settings(), &BaseAspect::changed,
                this, &RecommendationsWidget::updateModel);
    }

private:
    static QPixmap pixmapFromFile(const QString &url)
    {
        const QString path = FilePath::fromUserInput(url).toFSPathString();
        const qreal dpr = qApp->devicePixelRatio();
        const QString key = QLatin1String(Q_FUNC_INFO) % path % QString::number(dpr);
        QPixmap pixmap;
        if (QPixmapCache::find(key, &pixmap))
            return pixmap;
        pixmap = QPixmap(path);
        if (!pixmap.isNull()) {
            pixmap = pixmap.scaled(WelcomePageHelpers::WelcomeThumbnailSize * dpr,
                                   Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pixmap.setDevicePixelRatio(dpr);
        }
        QPixmapCache::insert(key, pixmap);
        return pixmap;
    }

    void updateModel()
    {
        m_model.clear();
        const QList<ListItem *> items = OverviewItem::items(
            {OverviewItem::Course, OverviewItem::Example, OverviewItem::Tutorial});
        m_model.appendItems(items);
        qCDebug(qtWelcomeOverviewLog) << "Loaded" << m_model.rowCount() << "items. User flags:"
                                      << settings().userFlags();
    }

    ListModel m_model;
    GridView *m_view;
    OverviewItemDelegate m_delegate;
};

class OverviewWelcomePageWidget final : public QWidget
{
public:
    OverviewWelcomePageWidget() = default;

    void showEvent(QShowEvent *event) override
    {
        if (!m_uiInitialized) {
            initializeUi();
            m_uiInitialized = true;
        }
        ExtensionManager::setLegalNoticeVisible(true);
        QWidget::showEvent(event);
    }

    void hideEvent(QHideEvent *event) override
    {
        ExtensionManager::setLegalNoticeVisible(false);
        QWidget::hideEvent(event);
    }

private:
    static QWidget *recentProjectsPanel()
    {
        QWidget *projects = ProjectExplorer::ProjectExplorerPlugin::createRecentProjectsView();
        projects->setMinimumWidth(100);
        projects->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        auto newButton = new QtcButton(Tr::tr("Create Project..."), QtcButton::LargePrimary);
        QStackedWidget *stackView;

        using namespace Layouting;
        QWidget *panel = QtcWidgets::Rectangle {
            radius(radiusL), fillBrush(rectFill()), strokePen(rectStroke()),
            customMargins(SpacingTokens::PaddingHXxl, SpacingTokens::PaddingVXl,
            rectStroke().width(), SpacingTokens::PaddingVXl),
            Column {
                tfLabel(Tr::tr("Recent Projects"), titleTf),
                Stack {
                    bindTo(&stackView),
                    Row { projects, noMargin },
                    Grid {
                        GridCell({ Align(Qt::AlignCenter, newButton) }),
                    },
                },
                noMargin, spacing(SpacingTokens::GapVM),
            },
        }.emerge();

        auto setStackIndex = [stackView] {
            const bool hasProjects =
                !ProjectExplorer::ProjectExplorerPlugin::recentProjects().empty();
            stackView->setCurrentIndex(hasProjects ? 0 : 1);
        };
        setStackIndex();
        connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
                &ProjectExplorer::ProjectExplorerPlugin::recentProjectsChanged,
                stackView, setStackIndex);
        connect(newButton, &QtcButton::clicked, newButton, [] {
            QAction *openAction = ActionManager::command(Core::Constants::NEW)->action();
            openAction->trigger();
        });

        return panel;
    }

    static QWidget *blogPostsPanel()
    {
        using namespace Layouting;
        return QtcWidgets::Rectangle {
            radius(radiusL), fillBrush(rectFill()), strokePen(rectStroke()),
            customMargins(SpacingTokens::PaddingHXxl, SpacingTokens::PaddingVXl,
                          SpacingTokens::PaddingHXxl, SpacingTokens::PaddingVXl),
            Column {
                tfLabel(Tr::tr("Highlights"), titleTf),
                new BlogWidget,
                noMargin, spacing(SpacingTokens::GapVM),
            },
        }.emerge();
    }

    static QWidget *recommendationsPanel(QWidget *parent)
    {
        auto settingsToolButton = new QPushButton;
        settingsToolButton->setIcon(Icons::SETTINGS.icon());
        settingsToolButton->setFlat(true);
        settingsToolButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        QWidget *optionsOverlay = createOnboardingWizard(parent);
        optionsOverlay->setVisible(settings().showWizardOnStart());
        connect(settingsToolButton, &QAbstractButton::clicked, optionsOverlay, &QWidget::show);

        using namespace Layouting;
        return Column {
            Row {
                tfLabel(Tr::tr("Recommended for you"), titleTf),
                settingsToolButton,
                st,
            },
            new RecommendationsWidget,
            spacing(SpacingTokens::GapVM),
            noMargin,
        }.emerge();
    }

    void initializeUi()
    {
        using namespace Layouting;
        auto projectsAndBlogPosts = Widget {
            Row {
                recentProjectsPanel(),
                blogPostsPanel(),
                spacing(SpacingTokens::PaddingHXxl),
                customMargins(0, 0, SpacingTokens::PaddingHXxl, 0),
            },
        }.emerge();
        projectsAndBlogPosts->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

        Column {
            Widget {
                Column {
                    projectsAndBlogPosts,
                    recommendationsPanel(parentWidget()),
                    noMargin,
                    spacing(SpacingTokens::PaddingVXxl),
                },
                customMargins(SpacingTokens::PaddingVXxl, 0, 0, 0),
            },
            customMargins(0, SpacingTokens::PaddingHXxl, 0, 0),
        }.attachTo(this);
    }

    static QBrush rectFill()
    {
        return Qt::transparent;
    }

    static QPen rectStroke()
    {
        return creatorColor(Core::WelcomePageHelpers::cardDefaultStroke);
    }

    static constexpr TextFormat titleTf {
        .themeColor = Theme::Token_Text_Muted,
        .uiElement = StyleHelper::UiElementH5,
    };
    bool m_uiInitialized = false;
};

class OverviewWelcomePage final : public IWelcomePage
{
public:
    OverviewWelcomePage() = default;

    QString title() const final { return Tr::tr("Overview"); }
    int priority() const final { return 1; }
    Id id() const final { return "Overview"; }
    QWidget *createWidget() const final { return new OverviewWelcomePageWidget; }
};

void setupOverviewWelcomePage(QObject *guard)
{
    auto page = new OverviewWelcomePage;
    page->setParent(guard);
}

#ifdef WITH_TESTS
void LearningTest::testFlagsMatching()
{
    QFETCH(QStringList, userFlags);
    QFETCH(QStringList, itemFlags);
    QFETCH(bool, isMatch);

    const bool actualMatch = OverviewItem::validByFlags(userFlags, itemFlags);
    QCOMPARE(actualMatch, isMatch);
}

void LearningTest::testFlagsMatching_data()
{
    QTest::addColumn<QStringList>("userFlags");
    QTest::addColumn<QStringList>("itemFlags");
    QTest::addColumn<bool>("isMatch");

    const QString targetDesktop = QLatin1String(TARGET_PREFIX) + TARGET_DESKTOP;
    const QString targetiOS = QLatin1String(TARGET_PREFIX) + TARGET_IOS;
    const QString targetAndroid = QLatin1String(TARGET_PREFIX) + TARGET_ANDROID;
    const QString expBasic = QLatin1String(EXPERIENCE_PREFIX) + EXPERIENCE_BASIC;
    const QString expAdvanced = QLatin1String(EXPERIENCE_PREFIX) + EXPERIENCE_ADVANCED;

    QTest::newRow("no_user_flags") << QStringList()
                                   << QStringList({targetDesktop, expBasic})
                                   << true;
    QTest::newRow("no_item_flags") << QStringList({targetDesktop, expBasic})
                                   << QStringList()
                                   << true;
    QTest::newRow("identical_flags") << QStringList({targetiOS, expBasic})
                                     << QStringList({targetiOS, expBasic})
                                     << true;
    QTest::newRow("no_user_or_item_flags") << QStringList()
                                           << QStringList()
                                           << true;
    QTest::newRow("user_basic_item_advanced") << QStringList({expBasic})
                                              << QStringList({expAdvanced})
                                              << false;
    QTest::newRow("user_basic_item_both") << QStringList({expBasic})
                                          << QStringList({expBasic, expAdvanced})
                                          << true;
    QTest::newRow("user_basic_item_undefiend") << QStringList({expBasic})
                                               << QStringList()
                                               << true;
    QTest::newRow("user_undefined_item_ios") << QStringList({expBasic})
                                             << QStringList({targetiOS})
                                             << false;
}
#endif // WITH_TESTS

} // namespace Learning::Internal

#include "overviewwelcomepage.moc"
