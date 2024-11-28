// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagerwidget.h"

#include "extensionmanagersettings.h"
#include "extensionmanagertr.h"
#include "extensionsbrowser.h"
#include "extensionsmodel.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
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
#include <utils/infobar.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/markdownbrowser.h>
#include <utils/mimeutils.h>
#include <utils/networkaccessmanager.h>
#include <utils/stringutils.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/temporarydirectory.h>
#include <utils/textutils.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProgressDialog>
#include <QScrollArea>

using namespace Core;
using namespace Utils;
using namespace StyleHelper;
using namespace WelcomePageHelpers;

namespace ExtensionManager::Internal {

Q_LOGGING_CATEGORY(widgetLog, "qtc.extensionmanager.widget", QtWarningMsg)

constexpr TextFormat contentTF
    {Theme::Token_Text_Default, UiElement::UiElementBody2};

constexpr TextFormat h6TF
    {contentTF.themeColor, UiElement::UiElementH6};
constexpr TextFormat h6CapitalTF
    {Theme::Token_Text_Muted, UiElement::UiElementH6Capital};

static QLabel *sectionTitle(const TextFormat &tf, const QString &title)
{
    auto *label = new ElidingLabel(title);
    applyTf(label, tf);
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
            {titleTF.themeColor, Utils::StyleHelper::UiElementCaption};

        m_title = new ElidingLabel;
        applyTf(m_title, titleTF);
        m_vendor = new Button({}, Button::SmallLink);
        m_vendor->setContentsMargins({});
        m_divider = new QLabel;
        m_divider->setFixedSize(1, dividerH);
        WelcomePageHelpers::setBackgroundColor(m_divider, dlTF.themeColor);
        m_dlIcon = new QLabel;
        const QPixmap dlIcon = Icon({{":/extensionmanager/images/download.png", dlTF.themeColor}},
                                    Icon::Tint).pixmap();
        m_dlIcon->setPixmap(dlIcon);
        m_dlCount = new ElidingLabel;
        applyTf(m_dlCount, dlTF);
        m_dlCount->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        m_details = new ElidingLabel;
        applyTf(m_details, detailsTF);
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

        const QString description = current.data(RoleDescriptionShort).toString();
        m_details->setText(description);

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

const char kRestartSetting[] = "RestartAfterPluginEnabledChanged";

class PluginStatusWidget : public QWidget
{
public:
    explicit PluginStatusWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_label = new InfoLabel;
        m_switch = new Switch(Tr::tr("Active"));
        m_pluginView.hide();
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        using namespace Layouting;
        Column {
            m_label,
            m_switch,
        }.attachTo(this);

        connect(m_switch, &QCheckBox::clicked, this, [this](bool checked) {
            ExtensionSystem::PluginSpec *spec = pluginSpecForId(m_pluginId);
            if (spec == nullptr)
                return;
            const bool doIt = m_pluginView.data().setPluginsEnabled({spec}, checked);
            if (doIt) {
                if (checked && spec->isEffectivelySoftloadable()) {
                    ExtensionSystem::PluginManager::loadPluginsAtRuntime({spec});
                } else if (ICore::infoBar()->canInfoBeAdded(kRestartSetting)) {
                    Utils::InfoBarEntry info(
                        kRestartSetting,
                        Core::Tr::tr("Plugin changes will take effect after restart."));
                    info.addCustomButton(Tr::tr("Restart Now"), [] {
                        ICore::infoBar()->removeInfo(kRestartSetting);
                        QTimer::singleShot(0, ICore::instance(), &ICore::restart);
                    });
                    ICore::infoBar()->addInfo(info);
                }

                ExtensionSystem::PluginManager::writeSettings();
            } else {
                m_switch->setChecked(!checked);
            }
        });

        connect(ExtensionSystem::PluginManager::instance(),
                &ExtensionSystem::PluginManager::pluginsChanged, this, &PluginStatusWidget::update);

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
    QStackedWidget *m_detailsStack;
    CollapsingWidget *m_secondaryDescriptionWidget;
    HeadingWidget *m_headingWidget;
    QWidget *m_secondaryContent;
    MarkdownBrowser *m_description;
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

static QWidget *descriptionPlaceHolder()
{
    auto placeHolder = new QWidget;
    static const TextFormat tF {
        Theme::Token_Text_Muted, UiElement::UiElementH4
    };
    auto title = new ElidingLabel(Tr::tr("No details to show"));
    applyTf(title, tF);
    title->setAlignment(Qt::AlignCenter);
    auto text = new QLabel(Tr::tr("Select an extension to see more information about it."));
    applyTf(text, tF, false);
    text->setAlignment(Qt::AlignCenter);
    text->setFont({});
    using namespace Layouting;
    // clang-format off
    Row {
        st,
        Column {
            Stretch(2),
            title,
            WelcomePageHelpers::createRule(Qt::Horizontal),
            text,
            Stretch(3),
            spacing(SpacingTokens::ExPaddingGapL),
        },
        st,
        noMargin,
    }.attachTo(placeHolder);
    // clang-format on
    WelcomePageHelpers::setBackgroundColor(placeHolder, Theme::Token_Background_Muted);
    placeHolder->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    return placeHolder;
}

ExtensionManagerWidget::ExtensionManagerWidget()
{
    m_extensionModel = new ExtensionsModel(this);
    m_extensionBrowser = new ExtensionsBrowser(m_extensionModel);
    auto descriptionColumns = new QWidget;
    m_secondaryDescriptionWidget = new CollapsingWidget;

    m_headingWidget = new HeadingWidget;
    m_description = new MarkdownBrowser;
    m_description->setAllowRemoteImages(true);
    m_description->setFrameStyle(QFrame::NoFrame);
    m_description->setOpenExternalLinks(true);
    QPalette browserPal = m_description->palette();
    browserPal.setColor(QPalette::Base, creatorColor(Theme::Token_Background_Default));
    m_description->setPalette(browserPal);
    const int verticalPadding = SpacingTokens::ExVPaddingGapXl - SpacingTokens::VPaddingM;
    m_description->setMargins({verticalPadding, 0, verticalPadding, 0});

    m_dateUpdatedTitle = sectionTitle(h6TF, Tr::tr("Last Update"));
    m_dateUpdated = new QLabel;
    applyTf(m_dateUpdated, contentTF, false);
    m_tagsTitle = sectionTitle(h6TF, Tr::tr("Tags"));
    m_tags = new TagList;
    m_platformsTitle = sectionTitle(h6TF, Tr::tr("Platforms"));
    m_platforms = new QLabel;
    applyTf(m_platforms, contentTF, false);
    m_dependenciesTitle = sectionTitle(h6TF, Tr::tr("Dependencies"));
    m_dependencies = new QLabel;
    applyTf(m_dependencies, contentTF, false);
    m_packExtensionsTitle = sectionTitle(h6TF, Tr::tr("Extensions in pack"));
    m_packExtensions = new QLabel;
    applyTf(m_packExtensions, contentTF, false);
    m_pluginStatus = new PluginStatusWidget;

    auto secondary = new QWidget;

    using namespace Layouting;
    const auto spL = spacing(SpacingTokens::VPaddingL);
    const auto spXxs = spacing(SpacingTokens::VPaddingXxs);
    // clang-format off
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
        },
        noMargin, spacing(0),
    }.attachTo(m_secondaryDescriptionWidget);

    Row {
        Row {
            Column {
                Row {
                    m_headingWidget,
                    m_pluginStatus,
                    customMargins(SpacingTokens::ExVPaddingGapXl, SpacingTokens::ExVPaddingGapXl,
                                  SpacingTokens::ExVPaddingGapXl, SpacingTokens::ExVPaddingGapXl),
                },
                m_description,
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
            WelcomePageHelpers::createRule(Qt::Vertical),
            Stack {
                bindTo(&m_detailsStack),
                descriptionPlaceHolder(),
                descriptionColumns,
            },
        },
        noMargin, spacing(0),
    }.attachTo(this);
    // clang-format on

    WelcomePageHelpers::setBackgroundColor(this, Theme::Token_Background_Default);

    connect(m_extensionBrowser, &ExtensionsBrowser::itemSelected,
            this, &ExtensionManagerWidget::updateView);
    connect(this, &ResizeSignallingWidget::resized, this, [this](const QSize &size) {
        const int intendedBrowserColumnWidth = size.width() / 3;
        m_extensionBrowser->adjustToWidth(intendedBrowserColumnWidth);
        const int availableDescriptionWidth = size.width() - m_extensionBrowser->width();
        const bool secondaryDescriptionVisible = availableDescriptionWidth > 1000;
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

void ExtensionManagerWidget::updateView(const QModelIndex &current)
{
    if (current.isValid()) {
        m_detailsStack->setCurrentIndex(1);
    } else {
        m_detailsStack->setCurrentIndex(0);
        return;
    }

    m_headingWidget->update(current);

    m_currentItemName = current.data(RoleName).toString();
    const bool isPack = current.data(RoleItemType) == ItemTypePack;
    m_pluginStatus->setPluginId(isPack ? QString() : current.data(RoleId).toString());
    m_currentDownloadUrl = current.data(RoleDownloadUrl).toString();

    m_currentId = current.data(RoleVendorId).toString() + "." + current.data(RoleId).toString();

    {
        const QString description = current.data(RoleDescriptionLong).toString();
        m_description->setMarkdown(description);
        m_description->document()->setDocumentMargin(SpacingTokens::VPaddingM);
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
