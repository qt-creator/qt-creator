// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagerwidget.h"

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

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QMovie>
#include <QPainter>
#include <QProgressDialog>
#include <QScrollArea>
#include <QSignalMapper>

using namespace Core;
using namespace Utils;
using namespace StyleHelper;
using namespace WelcomePageHelpers;

namespace ExtensionManager::Internal {

Q_LOGGING_CATEGORY(widgetLog, "qtc.extensionmanager.widget", QtWarningMsg)

constexpr TextFormat h5TF
    {Theme::Token_Text_Default, UiElement::UiElementH5};
constexpr TextFormat h6TF
    {h5TF.themeColor, UiElement::UiElementH6};
constexpr TextFormat h6CapitalTF
    {Theme::Token_Text_Muted, UiElement::UiElementH6Capital};
constexpr TextFormat contentTF
    {Theme::Token_Text_Default, UiElement::UiElementBody2};

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

        const auto pluginData = current.data(RolePlugins).value<PluginsData>();
        if (current.data(RoleItemType).toInt() == ItemTypePack) {
            const int pluginsCount = pluginData.count();
            const QString details = Tr::tr("Pack contains %n plugins.", nullptr, pluginsCount);
            m_details->setText(details);
        } else {
            m_details->setText({});
        }

        const ItemType itemType = current.data(RoleItemType).value<ItemType>();
        const bool isPack = itemType == ItemTypePack;
        const bool isRemotePlugin = !(isPack || pluginSpecForName(name));
        installButton->setVisible(isRemotePlugin && !pluginData.empty());
        if (installButton->isVisible())
            installButton->setToolTip(pluginData.constFirst().second);
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
        m_checkBox = new QCheckBox(Tr::tr("Load on start"));
        m_restartButton = new Button(Tr::tr("Restart Now"), Button::MediumPrimary);
        m_restartButton->setVisible(false);
        m_pluginView.hide();

        using namespace Layouting;
        Column {
            m_label,
            m_checkBox,
            m_restartButton,
        }.attachTo(this);

        connect(m_checkBox, &QCheckBox::clicked, this, [this](bool checked) {
            ExtensionSystem::PluginSpec *spec = pluginSpecForName(m_pluginName);
            if (spec == nullptr)
                return;
            const bool doIt = m_pluginView.data().setPluginsEnabled({spec}, checked);
            if (doIt) {
                m_restartButton->show();
                ExtensionSystem::PluginManager::writeSettings();
            } else {
                m_checkBox->setChecked(!checked);
            }
        });

        connect(m_restartButton, &QAbstractButton::clicked,
                ICore::instance(), &ICore::restart, Qt::QueuedConnection);

        update();
    }

    void setPluginName(const QString &name)
    {
        m_pluginName = name;
        update();
    }

private:
    void update()
    {
        const ExtensionSystem::PluginSpec *spec = pluginSpecForName(m_pluginName);
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

        m_checkBox->setChecked(spec->isRequired() || spec->isEnabledBySettings());
        m_checkBox->setEnabled(!spec->isRequired());
    }

    InfoLabel *m_label;
    QCheckBox *m_checkBox;
    QAbstractButton *m_restartButton;
    QString m_pluginName;
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
        m_signalMapper = new QSignalMapper(this);
        connect(m_signalMapper, &QSignalMapper::mappedString, this, &TagList::tagSelected);
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
                        m_signalMapper, qOverload<>(&QSignalMapper::map));
                m_signalMapper->setMapping(tagButton, tag);
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
    QSignalMapper *m_signalMapper;
};

class ExtensionManagerWidget final : public Core::ResizeSignallingWidget
{
public:
    ExtensionManagerWidget();

private:
    void updateView(const QModelIndex &current);
    void fetchAndInstallPlugin(const QUrl &url);
    void fetchAndDisplayImage(const QUrl &url);

    QString m_currentItemName;
    ExtensionsBrowser *m_extensionBrowser;
    CollapsingWidget *m_secondaryDescriptionWidget;
    HeadingWidget *m_headingWidget;
    QWidget *m_primaryContent;
    QWidget *m_secondaryContent;
    QLabel *m_description;
    QLabel *m_linksTitle;
    QLabel *m_links;
    QLabel *m_imageTitle;
    QLabel *m_image;
    QBuffer m_imageDataBuffer;
    QMovie m_imageMovie;
    QLabel *m_tagsTitle;
    TagList *m_tags;
    QLabel *m_compatVersionTitle;
    QLabel *m_compatVersion;
    QLabel *m_platformsTitle;
    QLabel *m_platforms;
    QLabel *m_dependenciesTitle;
    QLabel *m_dependencies;
    QLabel *m_packExtensionsTitle;
    QLabel *m_packExtensions;
    PluginStatusWidget *m_pluginStatus;
    PluginsData m_currentItemPlugins;
    Tasking::TaskTreeRunner m_dlTaskTreeRunner;
    Tasking::TaskTreeRunner m_imgTaskTreeRunner;
};

ExtensionManagerWidget::ExtensionManagerWidget()
{
    m_extensionBrowser = new ExtensionsBrowser;
    auto descriptionColumns = new QWidget;
    m_secondaryDescriptionWidget = new CollapsingWidget;

    m_headingWidget = new HeadingWidget;
    m_description = tfLabel(contentTF, false);
    m_description->setWordWrap(true);
    m_linksTitle = sectionTitle(h6CapitalTF, Tr::tr("More information"));
    m_links = tfLabel(contentTF, false);
    m_links->setOpenExternalLinks(true);
    m_imageTitle = sectionTitle(h6CapitalTF, {});
    m_image = new QLabel;
    m_imageMovie.setDevice(&m_imageDataBuffer);

    using namespace Layouting;
    auto primary = new QWidget;
    const auto spL = spacing(SpacingTokens::VPaddingL);
    Column {
        m_description,
        Column { m_linksTitle, m_links, spL },
        Column { m_imageTitle, m_image, spL },
        st,
        noMargin, spacing(SpacingTokens::ExVPaddingGapXl),
    }.attachTo(primary);
    m_primaryContent = toScrollableColumn(primary);

    m_tagsTitle = sectionTitle(h6TF, Tr::tr("Tags"));
    m_tags = new TagList;
    m_compatVersionTitle = sectionTitle(h6TF, Tr::tr("Compatibility"));
    m_compatVersion = tfLabel(contentTF, false);
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
            Column { m_tagsTitle, m_tags, spXxs },
            Column { m_compatVersionTitle, m_compatVersion, spXxs },
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
    connect(m_headingWidget, &HeadingWidget::pluginInstallationRequested, this, [this](){
        fetchAndInstallPlugin(QUrl::fromUserInput(m_currentItemPlugins.constFirst().second));
    });
    connect(m_tags, &TagList::tagSelected, m_extensionBrowser, &ExtensionsBrowser::setFilter);
    connect(m_headingWidget, &HeadingWidget::vendorClicked,
            m_extensionBrowser, &ExtensionsBrowser::setFilter);

    updateView({});
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

    m_currentItemName = current.data().toString();
    const bool isPack = current.data(RoleItemType) == ItemTypePack;
    m_pluginStatus->setPluginName(isPack ? QString() : m_currentItemName);
    m_currentItemPlugins = current.data(RolePlugins).value<PluginsData>();

    auto toContentParagraph = [](const QString &text) {
        const QString pHtml = QString::fromLatin1("<p style=\"margin-top:0;margin-bottom:0;"
                                                  "line-height:%1px\">%2</p>")
                                  .arg(contentTF.lineHeight()).arg(text);
        return pHtml;
    };

    {
        const TextData textData = current.data(RoleDescriptionText).value<TextData>();
        const bool hasDescription = !textData.isEmpty();
        if (hasDescription) {
            const QString headerCssTemplate =
                ";margin-top:%1;margin-bottom:%2;padding-top:0;padding-bottom:0;";
            const QString h4Css = fontToCssProperties(uiFont(UiElementH4))
                                  + headerCssTemplate.arg(0).arg(SpacingTokens::VGapL);
            const QString h5Css = fontToCssProperties(uiFont(UiElementH5))
                                  + headerCssTemplate.arg(SpacingTokens::ExVPaddingGapXl)
                                        .arg(SpacingTokens::VGapL);
            QString descriptionHtml;
            for (const TextData::Type &text : textData) {
                if (text.second.isEmpty())
                    continue;
                const QString paragraph =
                    QString::fromLatin1("<div style=\"%1\">%2</div>%3")
                        .arg(descriptionHtml.isEmpty() ? h4Css : h5Css)
                        .arg(text.first)
                        .arg(toContentParagraph(text.second.join("<br/>")));
                descriptionHtml.append(paragraph);
            }
            descriptionHtml.prepend(QString::fromLatin1("<body style=\"color:%1;\">")
                                        .arg(creatorColor(Theme::Token_Text_Default).name()));
            descriptionHtml.append("</body>");
            m_description->setText(descriptionHtml);
        }
        m_description->setVisible(hasDescription);

        const LinksData linksData = current.data(RoleDescriptionLinks).value<LinksData>();
        const bool hasLinks = !linksData.isEmpty();
        if (hasLinks) {
            QString linksHtml;
            const QStringList links = transform(linksData, [](const LinksData::Type &link) {
                const QString anchor = link.first.isEmpty() ? link.second : link.first;
                return QString::fromLatin1(R"(<a href="%1" style="color:%2">%3 &gt;</a>)")
                    .arg(link.second)
                    .arg(creatorColor(Theme::Token_Text_Accent).name())
                    .arg(anchor);
            });
            linksHtml = links.join("<br/>");
            m_links->setText(toContentParagraph(linksHtml));
        }
        m_linksTitle->setVisible(hasLinks);
        m_links->setVisible(hasLinks);

        m_imgTaskTreeRunner.reset();
        m_imageMovie.stop();
        m_imageDataBuffer.close();
        m_image->clear();
        const ImagesData imagesData = current.data(RoleDescriptionImages).value<ImagesData>();
        const bool hasImages = !imagesData.isEmpty();
        if (hasImages) {
            const ImagesData::Type &image = imagesData.constFirst(); // Only show one image
            m_imageTitle->setText(image.first);
            fetchAndDisplayImage(image.second);
        }
        m_imageTitle->setVisible(hasImages);
        m_image->setVisible(hasImages);
    }

    {
        const QStringList tags = current.data(RoleTags).toStringList();
        m_tags->setTags(tags);
        const bool hasTags = !tags.isEmpty();
        m_tagsTitle->setVisible(hasTags);
        m_tags->setVisible(hasTags);

        const QString compatVersion = current.data(RoleCompatVersion).toString();
        const bool hasCompatVersion = !compatVersion.isEmpty();
        if (hasCompatVersion)
            m_compatVersion->setText(compatVersion);
        m_compatVersionTitle->setVisible(hasCompatVersion);
        m_compatVersion->setVisible(hasCompatVersion);

        const QStringList platforms = current.data(RolePlatforms).toStringList();
        const bool hasPlatforms = !platforms.isEmpty();
        if (hasPlatforms)
            m_platforms->setText(toContentParagraph(platforms.join("<br/>")));
        m_platformsTitle->setVisible(hasPlatforms);
        m_platforms->setVisible(hasPlatforms);

        const QStringList dependencies = current.data(RoleDependencies).toStringList();
        const bool hasDependencies = !dependencies.isEmpty();
        if (hasDependencies)
            m_dependencies->setText(toContentParagraph(dependencies.join("<br/>")));
        m_dependenciesTitle->setVisible(hasDependencies);
        m_dependencies->setVisible(hasDependencies);

        const PluginsData plugins = current.data(RolePlugins).value<PluginsData>();
        const bool hasExtensions = isPack && !plugins.isEmpty();
        if (hasExtensions) {
            const QStringList extensions = transform(plugins, &QPair<QString, QString>::first);
            m_packExtensions->setText(toContentParagraph(extensions.join("<br/>")));
        }
        m_packExtensionsTitle->setVisible(hasExtensions);
        m_packExtensions->setVisible(hasExtensions);
    }
}

void ExtensionManagerWidget::fetchAndInstallPlugin(const QUrl &url)
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
            return;
        const FilePath source = FilePath::fromUrl(storage->url);
        TempFileSaver saver(TemporaryDirectory::masterDirectoryPath()
                            + "/XXXXXX" + source.fileName());

        saver.write(storage->packageData);
        if (saver.finalize(ICore::dialogParent()))
            executePluginInstallWizard(saver.filePath());;
    };

    Group group{
        storage,
        NetworkQueryTask{onQuerySetup, onQueryDone},
        onGroupDone(onPluginInstallation),
    };

    m_dlTaskTreeRunner.start(group);
}

void ExtensionManagerWidget::fetchAndDisplayImage(const QUrl &url)
{
    using namespace Tasking;

    struct StorageStruct
    {
        QByteArray imageData;
        QUrl url;
    };
    Storage<StorageStruct> storage;

    const auto onFetchSetup = [url, storage](NetworkQuery &query) {
        storage->url = url;
        query.setRequest(QNetworkRequest(url));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        qCDebug(widgetLog).noquote() << "Sending image request:" << url.toDisplayString();
    };
    const auto onFetchDone = [storage](const NetworkQuery &query, DoneWith result) {
        qCDebug(widgetLog) << "Got image QNetworkReply:" << query.reply()->error();
        if (result == DoneWith::Success)
            storage->imageData = query.reply()->readAll();
    };

    const auto onShowImage = [storage, this]() {
        if (storage->imageData.isEmpty())
            return;
        m_imageDataBuffer.setData(storage->imageData);
        qCDebug(widgetLog).noquote() << "Image reponse size:"
                                     << QLocale::system().formattedDataSize(
                                            m_imageDataBuffer.size());
        if (!m_imageDataBuffer.open(QIODevice::ReadOnly))
            return;
        QImageReader reader(&m_imageDataBuffer);
        const bool animated = reader.supportsAnimation();
        if (animated) {
            m_image->setMovie(&m_imageMovie);
            m_imageMovie.start();
        } else {
            const QPixmap pixmap = QPixmap::fromImage(reader.read());
            m_image->setPixmap(pixmap);
        }
        qCDebug(widgetLog) << "Image dimensions:" << reader.size();
        qCDebug(widgetLog) << "Image is animated:" << animated;
    };

    Group group{
        storage,
        NetworkQueryTask{onFetchSetup, onFetchDone},
        onGroupDone(onShowImage),
    };

    m_imgTaskTreeRunner.start(group);
}

QWidget *createExtensionManagerWidget()
{
    return new ExtensionManagerWidget;
}

} // ExtensionManager::Internal

#include "extensionmanagerwidget.moc"
