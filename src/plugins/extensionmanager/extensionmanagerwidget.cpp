// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagerwidget.h"

#include "extensionmanagersettings.h"
#include "extensionmanagertr.h"
#include "extensionsbrowser.h"
#include "extensionsmodel.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwelcomepage.h>
#include <coreplugin/plugininstallwizard.h>
#include <coreplugin/welcomepagehelper.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginview.h>

#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/icon.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/temporarydirectory.h>
#include <utils/utilsicons.h>

#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QCache>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QMovie>
#include <QPainter>
#include <QProgressDialog>
#include <QScrollArea>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocument>

using namespace Core;
using namespace Utils;
using namespace StyleHelper;
using namespace WelcomePageHelpers;

namespace ExtensionManager::Internal {

Q_LOGGING_CATEGORY(widgetLog, "qtc.extensionmanager.widget", QtWarningMsg)

constexpr TextFormat contentTF
    {Theme::Token_Text_Default, UiElement::UiElementBody2};
constexpr TextFormat h5TF
    {contentTF.themeColor, UiElement::UiElementH5};
constexpr TextFormat h6TF
    {contentTF.themeColor, UiElement::UiElementH6};
constexpr TextFormat h6CapitalTF
    {Theme::Token_Text_Muted, UiElement::UiElementH6Capital};

static QLabel *sectionTitle(const TextFormat &tf, const QString &title)
{
    QLabel *label = tfLabel(tf, true);
    label->setText(title);
    return label;
};

static QWidget *toScrollableColumn(QWidget *widget)
{
    widget->setContentsMargins(SpacingTokens::ExVPaddingGapXl, SpacingTokens::ExVPaddingGapXl,
                               SpacingTokens::ExVPaddingGapXl, SpacingTokens::ExVPaddingGapXl);
    widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);

    auto scrollArea = new QScrollArea;
    scrollArea->setWidget(widget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    return scrollArea;
};

class CollapsingWidget : public QWidget
{
public:
    explicit CollapsingWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    }

    void setWidth(int width)
    {
        m_width = width;
        setVisible(width > 0);
        updateGeometry();
    }

    QSize sizeHint() const override
    {
        return {m_width, 0};
    }

private:
    int m_width = 100;
};

class HeadingWidget : public QWidget
{
    static constexpr int dividerH = 16;

    Q_OBJECT

public:
    explicit HeadingWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_icon = new QLabel;
        m_icon->setFixedSize(iconBgSizeBig);

        static const TextFormat titleTF
            {Theme::Token_Text_Default, UiElementH4};
        static const TextFormat vendorTF
            {Theme::Token_Text_Accent, UiElementLabelMedium};
        static const TextFormat dlTF
            {Theme::Token_Text_Muted, vendorTF.uiElement};
        static const TextFormat detailsTF
            {Theme::Token_Text_Default, UiElementBody2};

        m_title = tfLabel(titleTF);
        m_vendor = new Button({}, Button::SmallLink);
        m_vendor->setContentsMargins({});
        m_divider = new QLabel;
        m_divider->setFixedSize(1, dividerH);
        WelcomePageHelpers::setBackgroundColor(m_divider, dlTF.themeColor);
        m_dlIcon = new QLabel;
        const QPixmap dlIcon = Icon({{":/extensionmanager/images/download.png", dlTF.themeColor}},
                                    Icon::Tint).pixmap();
        m_dlIcon->setPixmap(dlIcon);
        m_dlCount = tfLabel(dlTF);
        m_dlCount->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        m_details = tfLabel(detailsTF);
        installButton = new Button(Tr::tr("Install..."), Button::MediumPrimary);
        installButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        installButton->hide();

        using namespace Layouting;
        Row {
            m_icon,
            Column {
                m_title,
                st,
                Row {
                    m_vendor,
                    Widget {
                        bindTo(&m_dlCountItems),
                        Row {
                            Space(SpacingTokens::HGapXs),
                            m_divider,
                            Space(SpacingTokens::HGapXs),
                            m_dlIcon,
                            Space(SpacingTokens::HGapXxs),
                            m_dlCount,
                            noMargin, spacing(0),
                        },
                    },
                },
                st,
                m_details,
                spacing(0),
            },
            Column {
                installButton,
                st,
            },
            noMargin, spacing(SpacingTokens::ExPaddingGapL),
        }.attachTo(this);

        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
        m_dlCountItems->setVisible(false);

        connect(installButton, &QAbstractButton::pressed,
                this, &HeadingWidget::pluginInstallationRequested);
        connect(m_vendor, &QAbstractButton::pressed, this, [this]() {
            emit vendorClicked(m_currentVendor);
        });

        update({});
    }

    void update(const QModelIndex &current)
    {
        if (!current.isValid())
            return;

        m_icon->setPixmap(itemIcon(current, SizeBig));

        const QString name = current.data(RoleName).toString();
        m_title->setText(name);

        m_currentVendor = current.data(RoleVendor).toString();
        m_vendor->setText(m_currentVendor);

        const int dlCount = current.data(RoleDownloadCount).toInt();
        const bool showDlCount = dlCount > 0;
        if (showDlCount)
            m_dlCount->setText(QString::number(dlCount));
        m_dlCountItems->setVisible(showDlCount);

        const QStringList plugins = current.data(RolePlugins).toStringList();
        if (current.data(RoleItemType).toInt() == ItemTypePack) {
            const int pluginsCount = plugins.count();
            const QString details = Tr::tr("Pack contains %n plugins.", nullptr, pluginsCount);
            m_details->setText(details);
        } else {
            m_details->setText({});
        }

        const ItemType itemType = current.data(RoleItemType).value<ItemType>();
        const bool isPack = itemType == ItemTypePack;
        const bool isRemotePlugin = !(isPack || pluginSpecForId(current.data(RoleId).toString()));
        const QString downloadUrl = current.data(RoleDownloadUrl).toString();
        installButton->setVisible(isRemotePlugin && !downloadUrl.isEmpty());
        if (installButton->isVisible())
            installButton->setToolTip(downloadUrl);
    }

signals:
    void pluginInstallationRequested();
    void vendorClicked(const QString &vendor);

private:
    QLabel *m_icon;
    QLabel *m_title;
    Button *m_vendor;
    QLabel *m_divider;
    QLabel *m_dlIcon;
    QLabel *m_dlCount;
    QWidget *m_dlCountItems;
    QLabel *m_details;
    QAbstractButton *installButton;
    QString m_currentVendor;
};

class PluginStatusWidget : public QWidget
{
public:
    explicit PluginStatusWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_label = new InfoLabel;
        m_switch = new Switch(Tr::tr("Load on start"));
        m_restartButton = new Button(Tr::tr("Restart Now"), Button::MediumPrimary);
        m_restartButton->setVisible(false);
        m_pluginView.hide();
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        using namespace Layouting;
        Column {
            m_label,
            m_switch,
            m_restartButton,
        }.attachTo(this);

        connect(m_switch, &QCheckBox::clicked, this, [this](bool checked) {
            ExtensionSystem::PluginSpec *spec = pluginSpecForId(m_pluginId);
            if (spec == nullptr)
                return;
            const bool doIt = m_pluginView.data().setPluginsEnabled({spec}, checked);
            if (doIt) {
                m_restartButton->show();
                ExtensionSystem::PluginManager::writeSettings();
            } else {
                m_switch->setChecked(!checked);
            }
        });

        connect(ExtensionSystem::PluginManager::instance(),
                &ExtensionSystem::PluginManager::pluginsChanged, this, &PluginStatusWidget::update);
        connect(m_restartButton, &QAbstractButton::clicked,
                ICore::instance(), &ICore::restart, Qt::QueuedConnection);

        update();
    }

    void setPluginId(const QString &id)
    {
        m_pluginId = id;
        update();
    }

private:
    void update()
    {
        const ExtensionSystem::PluginSpec *spec = pluginSpecForId(m_pluginId);
        setVisible(spec != nullptr);
        if (spec == nullptr)
            return;

        if (spec->hasError()) {
            m_label->setType(InfoLabel::Error);
            m_label->setText(Tr::tr("Error"));
        } else if (spec->state() == ExtensionSystem::PluginSpec::Running) {
            m_label->setType(InfoLabel::Ok);
            m_label->setText(Tr::tr("Loaded"));
        } else {
            m_label->setType(InfoLabel::NotOk);
            m_label->setText(Tr::tr("Not loaded"));
        }

        m_switch->setChecked(spec->isRequired() || spec->isEnabledBySettings());
        m_switch->setEnabled(!spec->isRequired());
    }

    InfoLabel *m_label;
    Switch *m_switch;
    QAbstractButton *m_restartButton;
    QString m_pluginId;
    ExtensionSystem::PluginView m_pluginView{this};
};

class TagList : public QWidget
{
    Q_OBJECT

public:
    explicit TagList(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        setLayout(layout);
        layout->setContentsMargins({});
    }

    void setTags(const QStringList &tags)
    {
        if (m_container) {
            delete m_container;
            m_container = nullptr;
        }

        if (!tags.empty()) {
            m_container = new QWidget(this);
            layout()->addWidget(m_container);

            using namespace Layouting;
            Flow flow {};
            flow.setNoMargins();
            flow.setSpacing(SpacingTokens::HGapXs);

            for (const QString &tag : tags) {
                QAbstractButton *tagButton = new Button(tag, Button::Tag);
                connect(tagButton, &QAbstractButton::clicked,
                        this, [this, tag] { emit tagSelected(tag); });
                flow.addItem(tagButton);
            }

            flow.attachTo(m_container);
        }

        updateGeometry();
    }

signals:
    void tagSelected(const QString &tag);

private:
    QWidget *m_container = nullptr;
};

class AnimatedImageHandler : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

    class Entry
    {
    public:
        Entry(const QByteArray &data)
        {
            if (data.isEmpty())
                return;

            buffer.setData(data);
            movie.setDevice(&buffer);
        }

        QBuffer buffer;
        QMovie movie;
    };

public:
    AnimatedImageHandler(
        QObject *parent,
        std::function<void()> redraw,
        std::function<void(const QString &name)> scheduleLoad)
        : QObject(parent)
        , m_redraw(redraw)
        , m_scheduleLoad(scheduleLoad)
        , m_entries(1024 * 1024 * 10) // 10 MB max image cache size
    {}

    virtual QSizeF intrinsicSize(
        QTextDocument *doc, int posInDocument, const QTextFormat &format) override
    {
        Q_UNUSED(doc);
        Q_UNUSED(posInDocument);
        QString name = format.toImageFormat().name();

        Entry *entry = m_entries.object(name);

        if (entry && entry->movie.isValid()) {
            if (!entry->movie.frameRect().isValid())
                entry->movie.jumpToFrame(0);
            return entry->movie.frameRect().size();
        } else if (!entry) {
            m_scheduleLoad(name);
        }

        return Utils::Icons::UNKNOWN_FILE.icon().actualSize(QSize(16, 16));
    }

    void drawObject(
        QPainter *painter,
        const QRectF &rect,
        QTextDocument *document,
        int posInDocument,
        const QTextFormat &format) override
    {
        Q_UNUSED(document);
        Q_UNUSED(posInDocument);

        Entry *entry = m_entries.object(format.toImageFormat().name());

        if (entry) {
            if (entry->movie.isValid())
                painter->drawImage(rect, entry->movie.currentImage());
            else
                painter->drawPixmap(rect.toRect(), m_brokenImage.pixmap(rect.size().toSize()));
            return;
        }

        painter->drawPixmap(
            rect.toRect(), Utils::Icons::UNKNOWN_FILE.icon().pixmap(rect.size().toSize()));
    }

    void set(const QString &name, QByteArray data)
    {
        if (data.size() > m_entries.maxCost())
            data.clear();

        std::unique_ptr<Entry> entry = std::make_unique<Entry>(data);

        if (entry->movie.frameCount() > 1) {
            connect(&entry->movie, &QMovie::frameChanged, this, [this]() { m_redraw(); });
            entry->movie.start();
        }
        if (m_entries.insert(name, entry.release(), qMax(1, data.size())))
            m_redraw();
    }

private:
    std::function<void()> m_redraw;
    std::function<void(const QString &)> m_scheduleLoad;
    QCache<QString, Entry> m_entries;

    const Icon ErrorCloseIcon = Utils::Icon({{":/utils/images/close.png", Theme::IconsErrorColor}});

    const QIcon m_brokenImage = Icon::combinedIcon(
        {Utils::Icons::UNKNOWN_FILE.icon(), ErrorCloseIcon.icon()});
};

class AnimatedDocument : public QTextDocument
{
public:
    AnimatedDocument(QObject *parent = nullptr)
        : QTextDocument(parent)
        , m_imageHandler(
              this,
              [this]() { documentLayout()->update(); },
              [this](const QString &name) { scheduleLoad(QUrl(name)); })
    {
        connect(this, &QTextDocument::documentLayoutChanged, this, [this]() {
            documentLayout()->registerHandler(QTextFormat::ImageObject, &m_imageHandler);
        });

        connect(this, &QTextDocument::contentsChanged, this, [this]() {
            if (m_imageLoaderTree.isRunning())
                m_imageLoaderTree.cancel();

            if (urlsToLoad.isEmpty())
                return;

            using namespace Tasking;

            const LoopList iterator(urlsToLoad);

            auto onQuerySetup = [iterator](NetworkQuery &query) {
                query.setRequest(QNetworkRequest(*iterator));
                query.setNetworkAccessManager(NetworkAccessManager::instance());
            };

            auto onQueryDone = [this](const NetworkQuery &query, DoneWith result) {
                if (result == DoneWith::Success)
                    m_imageHandler.set(query.reply()->url().toString(), query.reply()->readAll());
                else {
                    m_imageHandler.set(query.reply()->url().toString(), {});
                }
            };

            // clang-format off
            Group group {
                For(iterator) >> Do {
                    continueOnError,
                    NetworkQueryTask{onQuerySetup, onQueryDone},
                },
                onGroupDone([this]() {
                    urlsToLoad.clear();
                    markContentsDirty(0, this->characterCount());
                })
            };
            // clang-format on

            m_imageLoaderTree.start(group);
        });
    }

    void scheduleLoad(const QUrl &url) { urlsToLoad.append(url); }

private:
    AnimatedImageHandler m_imageHandler;
    QList<QUrl> urlsToLoad;
    Tasking::TaskTreeRunner m_imageLoaderTree;
};

class ExtensionManagerWidget final : public Core::ResizeSignallingWidget
{
public:
    ExtensionManagerWidget();

private:
    void updateView(const QModelIndex &current);
    void fetchAndInstallPlugin(const QUrl &url, const QString &id);

    QString m_currentItemName;
    ExtensionsModel *m_extensionModel;
    ExtensionsBrowser *m_extensionBrowser;
    CollapsingWidget *m_secondaryDescriptionWidget;
    HeadingWidget *m_headingWidget;
    QWidget *m_primaryContent;
    QWidget *m_secondaryContent;
    QTextBrowser *m_description;
    QLabel *m_dateUpdatedTitle;
    QLabel *m_dateUpdated;
    QLabel *m_tagsTitle;
    TagList *m_tags;
    QLabel *m_platformsTitle;
    QLabel *m_platforms;
    QLabel *m_dependenciesTitle;
    QLabel *m_dependencies;
    QLabel *m_packExtensionsTitle;
    QLabel *m_packExtensions;
    PluginStatusWidget *m_pluginStatus;
    QString m_currentDownloadUrl;
    QString m_currentId;
    Tasking::TaskTreeRunner m_dlTaskTreeRunner;
};

ExtensionManagerWidget::ExtensionManagerWidget()
{
    m_extensionModel = new ExtensionsModel(this);
    m_extensionBrowser = new ExtensionsBrowser(m_extensionModel);
    auto descriptionColumns = new QWidget;
    m_secondaryDescriptionWidget = new CollapsingWidget;

    m_headingWidget = new HeadingWidget;
    m_description = new QTextBrowser;
    m_description->setDocument(new AnimatedDocument(m_description));
    m_description->setFrameStyle(QFrame::NoFrame);
    m_description->setOpenExternalLinks(true);
    QPalette browserPal = m_description->palette();
    browserPal.setColor(QPalette::Base, creatorColor(Theme::Token_Background_Default));
    m_description->setPalette(browserPal);

    using namespace Layouting;
    auto primary = new QWidget;
    const auto spL = spacing(SpacingTokens::VPaddingL);
    // clang-format off
    Column {
        m_description,
        noMargin, spacing(SpacingTokens::ExVPaddingGapXl),
    }.attachTo(primary);
    // clang-format on
    m_primaryContent = toScrollableColumn(primary);

    m_dateUpdatedTitle = sectionTitle(h6TF, Tr::tr("Last Update"));
    m_dateUpdated = tfLabel(contentTF, false);
    m_tagsTitle = sectionTitle(h6TF, Tr::tr("Tags"));
    m_tags = new TagList;
    m_platformsTitle = sectionTitle(h6TF, Tr::tr("Platforms"));
    m_platforms = tfLabel(contentTF, false);
    m_dependenciesTitle = sectionTitle(h6TF, Tr::tr("Dependencies"));
    m_dependencies = tfLabel(contentTF, false);
    m_packExtensionsTitle = sectionTitle(h6TF, Tr::tr("Extensions in pack"));
    m_packExtensions = tfLabel(contentTF, false);
    m_pluginStatus = new PluginStatusWidget;

    auto secondary = new QWidget;
    const auto spXxs = spacing(SpacingTokens::VPaddingXxs);
    Column {
        sectionTitle(h6CapitalTF, Tr::tr("Extension details")),
        Column {
            Column { m_dateUpdatedTitle, m_dateUpdated, spXxs },
            Column { m_tagsTitle, m_tags, spXxs },
            Column { m_platformsTitle, m_platforms, spXxs },
            Column { m_dependenciesTitle, m_dependencies, spXxs },
            Column { m_packExtensionsTitle, m_packExtensions, spXxs },
            spacing(SpacingTokens::VPaddingL),
        },
        st,
        noMargin, spacing(SpacingTokens::ExVPaddingGapXl),
    }.attachTo(secondary);
    m_secondaryContent = toScrollableColumn(secondary);

    Row {
        WelcomePageHelpers::createRule(Qt::Vertical),
        Column {
            m_secondaryContent,
            m_pluginStatus,
        },
        noMargin, spacing(0),
    }.attachTo(m_secondaryDescriptionWidget);

    Row {
        WelcomePageHelpers::createRule(Qt::Vertical),
        Row {
            Column {
                Column {
                    m_headingWidget,
                    customMargins(SpacingTokens::ExVPaddingGapXl, SpacingTokens::ExVPaddingGapXl,
                                  SpacingTokens::ExVPaddingGapXl, SpacingTokens::ExVPaddingGapXl),
                },
                m_primaryContent,
            },
        },
        m_secondaryDescriptionWidget,
        noMargin, spacing(0),
    }.attachTo(descriptionColumns);

    Column {
        new StyledBar,
        Row {
            Space(SpacingTokens::ExVPaddingGapXl),
            m_extensionBrowser,
            descriptionColumns,
            noMargin, spacing(0),
        },
        noMargin, spacing(0),
    }.attachTo(this);

    WelcomePageHelpers::setBackgroundColor(this, Theme::Token_Background_Default);

    connect(m_extensionBrowser, &ExtensionsBrowser::itemSelected,
            this, &ExtensionManagerWidget::updateView);
    connect(this, &ResizeSignallingWidget::resized, this, [this](const QSize &size) {
        const int intendedBrowserColumnWidth = size.width() - 580;
        m_extensionBrowser->adjustToWidth(intendedBrowserColumnWidth);
        const bool secondaryDescriptionVisible = size.width() > 970;
        const int secondaryDescriptionWidth = secondaryDescriptionVisible ? 264 : 0;
        m_secondaryDescriptionWidget->setWidth(secondaryDescriptionWidth);
    });
    connect(m_headingWidget, &HeadingWidget::pluginInstallationRequested, this, [this]() {
        fetchAndInstallPlugin(QUrl::fromUserInput(m_currentDownloadUrl), m_currentId);
    });
    connect(m_tags, &TagList::tagSelected, m_extensionBrowser, &ExtensionsBrowser::setFilter);
    connect(m_headingWidget, &HeadingWidget::vendorClicked,
            m_extensionBrowser, &ExtensionsBrowser::setFilter);

    updateView({});
}

static QString markdownToHtml(const QString &markdown)
{
    QTextDocument doc;
    doc.setMarkdown(markdown);
    doc.setDefaultFont(contentTF.font());

    for (QTextBlock block = doc.begin(); block != doc.end(); block = block.next()) {
        QTextBlockFormat blockFormat = block.blockFormat();
        // Leave images as they are.
        if (block.text().contains(QChar::ObjectReplacementCharacter))
            continue;

        if (blockFormat.hasProperty(QTextFormat::HeadingLevel))
            blockFormat.setTopMargin(SpacingTokens::ExVPaddingGapXl);
        else
            blockFormat.setLineHeight(contentTF.lineHeight(), QTextBlockFormat::FixedHeight);
        blockFormat.setBottomMargin(SpacingTokens::VGapL);
        QTextCursor cursor(block);
        cursor.mergeBlockFormat(blockFormat);
        const TextFormat headingTf =
                blockFormat.headingLevel() == 1 ? h5TF
                                                : blockFormat.headingLevel() == 2 ? h6TF
                                                                                  : h6CapitalTF;
        const QFont headingFont = headingTf.font();
        for (auto it = block.begin(); !(it.atEnd()); ++it) {
            QTextFragment fragment = it.fragment();
            if (fragment.isValid()) {
                QTextCharFormat charFormat = fragment.charFormat();
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                if (blockFormat.hasProperty(QTextFormat::HeadingLevel)) {
                    charFormat.setFontCapitalization(headingFont.capitalization());
                    charFormat.setFontFamilies(headingFont.families());
                    charFormat.setFontPointSize(headingFont.pointSizeF());
                    charFormat.setFontWeight(headingFont.weight());
                    charFormat.setForeground(headingTf.color());
                } else if (charFormat.isAnchor()) {
                    charFormat.setForeground(creatorColor(Theme::Token_Text_Accent));
                } else {
                    charFormat.setForeground(contentTF.color());
                }
                cursor.setCharFormat(charFormat);
            }
        }
    }

    return doc.toHtml();
}

void ExtensionManagerWidget::updateView(const QModelIndex &current)
{
    m_headingWidget->update(current);

    const bool showContent = current.isValid();
    m_primaryContent->setVisible(showContent);
    m_secondaryContent->setVisible(showContent);
    m_headingWidget->setVisible(showContent);
    m_pluginStatus->setVisible(showContent);
    if (!showContent)
        return;

    m_currentItemName = current.data(RoleName).toString();
    const bool isPack = current.data(RoleItemType) == ItemTypePack;
    m_pluginStatus->setPluginId(isPack ? QString() : current.data(RoleId).toString());
    m_currentDownloadUrl = current.data(RoleDownloadUrl).toString();

    m_currentId = current.data(RoleVendorId).toString() + "." + current.data(RoleId).toString();

    {
        const QStringList description = {
            "# " + m_currentItemName,
            current.data(RoleDescriptionShort).toString(),
            "",
            current.data(RoleDescriptionLong).toString()
        };
        const QString descriptionMarkdown = description.join("\n");
        m_description->setText(markdownToHtml(descriptionMarkdown));
    }

    {
        auto idToDisplayName = [this](const QString &id) {
            const QModelIndex dependencyIndex = m_extensionModel->indexOfId(id);
            return dependencyIndex.data(RoleName).toString();
        };

        auto toContentParagraph = [](const QStringList &text) {
            const QString lines = text.join("<br/>");
            const QString pHtml = QString::fromLatin1("<p style=\"margin-top:0;margin-bottom:0;"
                                                      "line-height:%1px\">%2</p>")
                                      .arg(contentTF.lineHeight()).arg(lines);
            return pHtml;
        };

        const QDate dateUpdated = current.data(RoleDateUpdated).toDate();
        const bool hasDateUpdated = dateUpdated.isValid();
        if (hasDateUpdated)
            m_dateUpdated->setText(dateUpdated.toString());
        m_dateUpdatedTitle->setVisible(hasDateUpdated);
        m_dateUpdated->setVisible(hasDateUpdated);

        const QStringList tags = current.data(RoleTags).toStringList();
        m_tags->setTags(tags);
        const bool hasTags = !tags.isEmpty();
        m_tagsTitle->setVisible(hasTags);
        m_tags->setVisible(hasTags);

        const QStringList platforms = current.data(RolePlatforms).toStringList();
        const bool hasPlatforms = !platforms.isEmpty();
        if (hasPlatforms)
            m_platforms->setText(toContentParagraph(platforms));
        m_platformsTitle->setVisible(hasPlatforms);
        m_platforms->setVisible(hasPlatforms);

        const QStringList dependencies = current.data(RoleDependencies).toStringList();
        const bool hasDependencies = !dependencies.isEmpty();
        if (hasDependencies) {
            const QStringList displayNames = transform(dependencies, idToDisplayName);
            m_dependencies->setText(toContentParagraph(displayNames));
        }
        m_dependenciesTitle->setVisible(hasDependencies);
        m_dependencies->setVisible(hasDependencies);

        const QStringList plugins = current.data(RolePlugins).toStringList();
        const bool hasExtensions = isPack && !plugins.isEmpty();
        if (hasExtensions) {
            const QStringList displayNames = transform(plugins, idToDisplayName);
            m_packExtensions->setText(toContentParagraph(displayNames));
        }
        m_packExtensionsTitle->setVisible(hasExtensions);
        m_packExtensions->setVisible(hasExtensions);
    }
}

void ExtensionManagerWidget::fetchAndInstallPlugin(const QUrl &url, const QString &id)
{
    using namespace Tasking;

    struct StorageStruct
    {
        StorageStruct() {
            progressDialog.reset(new QProgressDialog(
                Tr::tr("Downloading..."), Tr::tr("Cancel"), 0, 0, ICore::dialogParent()));
            progressDialog->setWindowTitle(Tr::tr("Download Extension"));
            progressDialog->setWindowModality(Qt::ApplicationModal);
            progressDialog->setFixedSize(progressDialog->sizeHint());
            progressDialog->setAutoClose(false);
            progressDialog->show(); // TODO: Should not be needed. Investigate possible QT_BUG
        }
        std::unique_ptr<QProgressDialog> progressDialog;
        QByteArray packageData;
        QUrl url;
    };
    Storage<StorageStruct> storage;

    const auto onQuerySetup = [url, storage](NetworkQuery &query) {
        storage->url = url;
        query.setRequest(QNetworkRequest(url));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };
    const auto onQueryDone = [storage](const NetworkQuery &query, DoneWith result) {
        storage->progressDialog->close();
        if (result == DoneWith::Success) {
            storage->packageData = query.reply()->readAll();
        } else {
            QMessageBox::warning(
                ICore::dialogParent(),
                Tr::tr("Download Error"),
                Tr::tr("Cannot download extension") + "\n\n" + storage->url.toString() + "\n\n"
                    + Tr::tr("Code: %1.").arg(query.reply()->error()));
        }
    };

    const auto onPluginInstallation = [storage]() {
        if (storage->packageData.isEmpty())
            return false;
        const FilePath source = FilePath::fromUrl(storage->url);
        TempFileSaver saver(
            TemporaryDirectory::masterDirectoryPath() + "/XXXXXX-" + source.fileName());

        saver.write(storage->packageData);
        if (saver.finalize(ICore::dialogParent()))
            return executePluginInstallWizard(saver.filePath());
        return false;
    };

    const auto onDownloadSetup = [id](NetworkQuery &query) {
        query.setOperation(NetworkOperation::Post);
        query.setRequest(QNetworkRequest(
            QUrl(settings().externalRepoUrl() + "/api/v1/downloads/completed/" + id)));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };

    const auto onDownloadDone = [id](const NetworkQuery &query, DoneWith result) {
        if (result != DoneWith::Success) {
            qCWarning(widgetLog) << "Failed to notify download completion for" << id;
            qCWarning(widgetLog) << query.reply()->errorString();
            qCWarning(widgetLog) << query.reply()->readAll();
        } else {
            qCDebug(widgetLog) << "Download completion notification sent for" << id;
            qCDebug(widgetLog) << query.reply()->readAll();
        }
    };

    Group group{
        storage,
        NetworkQueryTask{onQuerySetup, onQueryDone},
        Sync{onPluginInstallation},
        NetworkQueryTask{onDownloadSetup, onDownloadDone},
    };

    m_dlTaskTreeRunner.start(group);
}

QWidget *createExtensionManagerWidget()
{
    return new ExtensionManagerWidget;
}

} // ExtensionManager::Internal

#include "extensionmanagerwidget.moc"
