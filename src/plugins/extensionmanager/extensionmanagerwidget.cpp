// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagerwidget.h"

#include "extensionmanagersettings.h"
#include "extensionmanagertr.h"
#include "extensionsbrowser.h"
#include "extensionsmodel.h"
#include "remotespec.h"

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
#include <utils/appinfo.h>
#include <utils/dropsupport.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/icon.h>
#include <utils/infobar.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/markdownbrowser.h>
#include <utils/mimeutils.h>
#include <utils/networkaccessmanager.h>
#include <utils/progressdialog.h>
#include <utils/stringutils.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/temporarydirectory.h>
#include <utils/textutils.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QScrollArea>

using namespace Core;
using namespace Utils;
using namespace StyleHelper;
using namespace WelcomePageHelpers;
using namespace ExtensionSystem;

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

const char kRestartSetting[] = "RestartAfterPluginEnabledChanged";

static void requestRestart()
{
    if (ICore::infoBar()->canInfoBeAdded(kRestartSetting)) {
        Utils::InfoBarEntry
            info(kRestartSetting, Core::Tr::tr("Plugin changes will take effect after restart."));
        info.setTitle(Tr::tr("Restart Required"));
        info.addCustomButton(Tr::tr("Restart Now"), [] {
            ICore::infoBar()->removeInfo(kRestartSetting);
            QTimer::singleShot(0, ICore::instance(), &ICore::restart);
        });
        ICore::infoBar()->addInfo(info);
    }
}

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

class VersionSelector final : public QWidget
{
    Q_OBJECT
public:
    VersionSelector(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_versionSelector = new QComboBox;
        m_versionSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        connect(m_versionSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
            if (index < 0 || size_t(index) >= m_versions.size())
                return;

            const auto &remoteSpec = m_versions.at(index);
            emit versionSelected(remoteSpec.get());

            if (remoteSpec->hasError()) {
                m_versionSelector->setToolTip(remoteSpec->errorString());
                return;
            }
        });

        using namespace Layouting;
        // clang-format off
        Row {
            m_versionSelector,
        }.attachTo(this);
        // clang-format on
    }

    void updateEntries()
    {
        m_versionSelector->clear();
        m_versionSelector->setEnabled(m_versions.size() > 0);
        // Add to version selector
        int initialIndex = -1;

        for (int i = 0; const auto &remoteSpec : m_versions) {
            const bool isCompatible = remoteSpec->resolveDependencies(PluginManager::plugins());

            QString versionStr = remoteSpec->version();
            if (!isCompatible)
                versionStr += Tr::tr(" (Incompatible)");
            else if (initialIndex == -1)
                initialIndex = i;

            m_versionSelector->addItem(versionStr);
            i++;
        }
        if (initialIndex != -1)
            m_versionSelector->setCurrentIndex(initialIndex);
        else
            emit versionSelected(nullptr);
    }

    void setExtension(const RemoteSpec *spec)
    {
        m_versions.clear();
        m_versionSelector->clear();

        m_versionSelector->setEnabled(!!spec);

        if (spec) {
            m_versions = spec->versions();
            Utils::sort(m_versions, [](const auto &a, const auto &b) {
                return RemoteSpec::versionCompare(a->version(), b->version()) > 0;
            });
        }

        updateEntries();
    }

    RemoteSpec *selectedVersion() const
    {
        if (m_versionSelector->currentIndex() < 0)
            return nullptr;
        return m_versions.at(m_versionSelector->currentIndex()).get();
    }

signals:
    void versionSelected(const RemoteSpec *spec);

private:
    std::vector<std::unique_ptr<RemoteSpec>> m_versions;
    QComboBox *m_versionSelector;
    Tasking::TaskTreeRunner m_fetchVersionsRunner;
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
        installButton = new Button(Tr::tr("Install..."), Button::LargePrimary);
        installButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        installButton->hide();
        connect(
            installButton,
            &QAbstractButton::pressed,
            this,
            &HeadingWidget::pluginInstallationRequested);

        removeButton = new Button(Tr::tr("Remove..."), Button::SmallSecondary);
        removeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        removeButton->hide();
        connect(removeButton, &QAbstractButton::pressed, this, [this]() {
            PluginManager::removePluginOnRestart(m_currentPluginId);
            requestRestart();
        });

        updateButton = new Button(Tr::tr("Update..."), Button::LargePrimary);
        updateButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        updateButton->hide();
        connect(updateButton, &QAbstractButton::pressed, this, &HeadingWidget::pluginUpdateRequested);

        m_versionSelector = new VersionSelector();
        connect(
            m_versionSelector,
            &VersionSelector::versionSelected,
            this,
            &HeadingWidget::versionSelected);

        using namespace Layouting;
        // clang-format off
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
                updateButton,
                removeButton,
                m_versionSelector,
                st,
            },
            noMargin, spacing(SpacingTokens::ExPaddingGapL),
        }.attachTo(this);
        // clang-format on

        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
        m_dlCountItems->setVisible(false);

        connect(m_vendor, &QAbstractButton::pressed, this, [this]() {
            emit vendorClicked(m_currentVendor);
        });

        update({});
    }

    RemoteSpec *selectedVersion() { return m_versionSelector->selectedVersion(); }

    void versionSelected(const RemoteSpec *spec)
    {
        installButton->setVisible(false);
        if (spec) {
            const PluginSpec *installedSpec = PluginManager::specById(spec->id());

            installButton->setVisible(
                !installedSpec || (installedSpec->version() != spec->version()));
            installButton->setEnabled(false);

            if (spec->hasError()) {
                installButton->setToolTip(
                    Tr::tr("Cannot install extension: %1").arg(spec->errorString()));
                return;
            }

            const std::optional<Source> source = spec->compatibleSource();
            if (!source)
                return;

            installButton->setEnabled(true);
            installButton->setToolTip(source->url);
        }
    }

    void update(const QModelIndex &current)
    {
        if (!current.isValid())
            return;

        m_currentPluginId = current.data(RoleId).toString();

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

        QVariant spec = current.data(RoleSpec);

        const PluginSpec *pluginSpec = qvariant_cast<const PluginSpec *>(spec);
        const RemoteSpec *remoteSpec = qvariant_cast<const RemoteSpec *>(spec);

        if (remoteSpec)
            pluginSpec = PluginManager::specById(remoteSpec->id());

        const ItemType itemType = current.data(RoleItemType).value<ItemType>();
        const bool isPack = itemType == ItemTypePack;
        const bool isRemotePlugin = !(isPack || pluginSpec);
        const QString downloadUrl = current.data(RoleDownloadUrl).toString();
        removeButton->setVisible(!isRemotePlugin && pluginSpec && !pluginSpec->isSystemPlugin());

        updateButton->setVisible(
            pluginSpec
            && PluginSpec::versionCompare(pluginSpec->version(), current.data(RoleVersion).toString())
                   < 0);

        m_versionSelector->setVisible(isRemotePlugin);

        //const RemoteSpec *remoteSpec = dynamic_cast<RemoteSpec *>(pluginSpec);
        m_versionSelector->setExtension(remoteSpec);

        if (isRemotePlugin) {
            auto spec = m_versionSelector->selectedVersion();
            versionSelected(spec);
        }
    }

signals:
    void pluginInstallationRequested();
    void pluginUpdateRequested();
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
    QAbstractButton *removeButton;
    QAbstractButton *updateButton;
    VersionSelector *m_versionSelector;
    QString m_currentVendor;
    QString m_currentPluginId;
};

class PluginStatusWidget : public QWidget
{
public:
    explicit PluginStatusWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_label = new InfoLabel;
        m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_switch = new Switch(Tr::tr("Active"));
        m_pluginView.hide();
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        using namespace Layouting;
        Grid {
            Span(2, m_label), br,
            m_switch, empty, br,
        }.attachTo(this);

        connect(m_switch, &QCheckBox::clicked, this, [this](bool checked) {
            PluginSpec *spec = PluginManager::specById(m_pluginId);
            if (spec == nullptr)
                return;
            const bool doIt = m_pluginView.data().setPluginsEnabled({spec}, checked);
            if (doIt) {
                if (checked && spec->isEffectivelySoftloadable())
                    PluginManager::loadPluginsAtRuntime({spec});
                else
                    requestRestart();

                PluginManager::writeSettings();
            } else {
                m_switch->setChecked(!checked);
            }
        });

        connect(
            PluginManager::instance(),
            &PluginManager::pluginsChanged,
            this,
            &PluginStatusWidget::update);

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
        const PluginSpec *spec = PluginManager::specById(m_pluginId);
        setVisible(spec != nullptr);
        if (spec == nullptr)
            return;

        if (spec->hasError()) {
            m_label->setType(InfoLabel::Error);
            m_label->setText(Tr::tr("Error"));
        } else if (spec->state() == PluginSpec::Running) {
            m_label->setType(InfoLabel::Ok);
            m_label->setText(Tr::tr("Loaded"));
        } else {
            m_label->setType(InfoLabel::NotOk);
            m_label->setText(Tr::tr("Not loaded"));
        }
        m_label->setAdditionalToolTip(spec->errorString());

        m_switch->setChecked(spec->isRequired() || spec->isEnabledBySettings());
        m_switch->setEnabled(!spec->isRequired());
    }

    InfoLabel *m_label;
    Switch *m_switch;
    QString m_pluginId;
    PluginView m_pluginView{this};
};

class TagList : public QWidget
{
    Q_OBJECT

public:
    using QWidget::QWidget;

    void setTags(const QStringList &tags)
    {
        delete layout();
        qDeleteAll(children());

        if (!tags.empty()) {
            const auto tagToButton = [this](const QString &tag) {
                auto btn = new Button(tag, Button::Tag);
                connect(btn, &QAbstractButton::clicked, [tag, this] { emit tagSelected(tag); });
                return btn;
            };

            using namespace Layouting;
            // clang-format off
            Flow {
                noMargin,
                spacing(SpacingTokens::HGapXs),
                Utils::transform(tags, tagToButton)
            }.attachTo(this);
            // clang-format on
        }

        updateGeometry();
    }

signals:
    void tagSelected(const QString &tag);
};

class ExtensionManagerWidget final : public Core::ResizeSignallingWidget
{
public:
    ExtensionManagerWidget();

private:
    void updateView(const QModelIndex &current);
    void fetchAndInstallPlugin(const QUrl &url, bool update, const QString &sha);

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
    m_dependencies->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    connect(m_dependencies, &QLabel::linkActivated, this, [this](const QString &link) {
        m_extensionBrowser->selectIndex(m_extensionModel->indexOfId(link));
    });

    m_packExtensionsTitle = sectionTitle(h6TF, Tr::tr("Extensions in pack"));
    m_packExtensions = new QLabel;
    applyTf(m_packExtensions, contentTF, false);

    m_packExtensions->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    connect(m_packExtensions, &QLabel::linkActivated, this, [this](const QString &link) {
        m_extensionBrowser->selectIndex(m_extensionModel->indexOfId(link));
    });

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

    const auto installOrUpdate = [this](bool update) {
        QTC_ASSERT(m_headingWidget->selectedVersion(), return);
        const std::optional<Source> source = m_headingWidget->selectedVersion()->compatibleSource();
        QTC_ASSERT(source, return);
        fetchAndInstallPlugin(QUrl::fromUserInput(source->url), update, source->sha);
    };

    connect(m_headingWidget, &HeadingWidget::pluginInstallationRequested, this, [installOrUpdate] {
        installOrUpdate(false);
    });
    connect(m_headingWidget, &HeadingWidget::pluginUpdateRequested, this, [installOrUpdate]() {
        installOrUpdate(true);
    });

    connect(m_tags, &TagList::tagSelected, m_extensionBrowser, &ExtensionsBrowser::setFilter);
    connect(m_headingWidget, &HeadingWidget::vendorClicked,
            m_extensionBrowser, &ExtensionsBrowser::setFilter);

    auto dropSupport = new DropSupport(this, [](QDropEvent *event, DropSupport *) {
        // only accept drops from the "outside" (e.g. file manager)
        return event->source() == nullptr;
    });
    connect(
        dropSupport,
        &DropSupport::filesDropped,
        this,
        [](const QList<DropSupport::FileSpec> &files, const QPoint &) {
            bool needsRestart = false;
            for (const auto &file : files) {
                InstallResult result = executePluginInstallWizard(file.filePath);
                if (result == InstallResult::NeedsRestart)
                    needsRestart = true;
                if (result == InstallResult::Error)
                    break;
            }
            if (needsRestart)
                requestRestart();
        });

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

    m_currentId = current.data(RoleFullId).toString();

    const QString description = current.data(RoleDescriptionLong).toString();
    m_description->setMarkdown(description);
    m_description->document()->setDocumentMargin(SpacingTokens::VPaddingM);

    auto idToDisplayName = [this](const QString &id) {
        const QModelIndex dependencyIndex = m_extensionModel->indexOfId(id);
        QString displayName = dependencyIndex.data(RoleName).toString();
        if (displayName.isEmpty())
            displayName = id;
        return QString("<a href=\"%1\">%2</a>").arg(id).arg(displayName);
    };

    auto toContentParagraph =
        [](const QStringList &text) {
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

void ExtensionManagerWidget::fetchAndInstallPlugin(const QUrl &url, bool update, const QString &sha)
{
    using namespace Tasking;

    struct StorageStruct
    {
        StorageStruct() {
            progressDialog.reset(createProgressDialog(0, Tr::tr("Download Extension"),
                                                      Tr::tr("Downloading...")));
        }
        std::unique_ptr<QProgressDialog> progressDialog;
        QByteArray packageData;
        QUrl url;
        QString sha;
        QString filename;
    };
    Storage<StorageStruct> storage;

    const auto onQuerySetup = [url, storage, sha](NetworkQuery &query) {
        storage->url = url;
        storage->sha = sha;
        query.setRequest(QNetworkRequest(url));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };
    const auto onQueryDone = [storage](const NetworkQuery &query, DoneWith result) -> DoneResult {
        storage->progressDialog->close();

        if (result != DoneWith::Success) {
            QMessageBox::warning(
                ICore::dialogParent(),
                Tr::tr("Download Error"),
                Tr::tr("Cannot download extension") + "\n\n" + storage->url.toString() + "\n\n"
                    + Tr::tr("Code: %1.").arg(query.reply()->error()));
            return DoneResult::Error;
        }

            storage->packageData = query.reply()->readAll();

        const QByteArray hash
            = QCryptographicHash::hash(storage->packageData, QCryptographicHash::Sha256);

        if (QString::fromLatin1(hash.toHex()) != storage->sha) {
            QMessageBox::warning(
                ICore::dialogParent(),
                Tr::tr("Download Error"),
                Tr::tr("Downloaded extension has invalid hash"));
            return DoneResult::Error;
        }

        const auto checkContentDisposition = [storage, &query] {
            QString contentDispo
                = query.reply()->header(QNetworkRequest::ContentDispositionHeader).toString();

            if (contentDispo.isEmpty())
                return;

            // Example: `content-disposition: attachment; filename=project-build-windows-.7z`
            // see also: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
            static QRegularExpression re(
                R"(^(?P<disposition>attachment|inline)(?:\s*;\s*(?P<paramlist>.*))?$)");

            QRegularExpressionMatch matches = re.match(contentDispo);
            if (!matches.hasMatch())
                return;

            const QString disposition = matches.captured("disposition");
            if (disposition != "attachment")
                return;

            const QString paramlist = matches.captured("paramlist");

            // Parse the "filename" parameter from the Content-Disposition header
            static QRegularExpression reParam(
                R"(filename\*?=['"]?(?:UTF-\d['"]*)?([^;\r\n"']*)['"]?;?)");

            QRegularExpressionMatch match = reParam.match(paramlist);
            if (!match.hasMatch())
                return;

            storage->filename = match.captured(1);
        };

        checkContentDisposition();

        return DoneResult::Success;
    };

    const auto onPluginInstallation = [storage, update]() {
        if (storage->packageData.isEmpty())
            return false;
        const FilePath source = FilePath::fromUrl(storage->url);
        const QString filename = storage->filename.isEmpty() ? source.fileName()
                                                             : storage->filename;
        TempFileSaver saver(TemporaryDirectory::masterDirectoryPath() + "/XXXXXX-" + filename);

        saver.write(storage->packageData);
        if (const Result<> res = saver.finalize()) {
            auto result = executePluginInstallWizard(saver.filePath(), update);
            switch (result) {
            case InstallResult::Success:
                return true;
            case InstallResult::NeedsRestart:
                requestRestart();
                return true;
            case InstallResult::Error:
                return false;
            }
        } else {
            FileUtils::showError(res.error());
        }
        return false;
    };

    /*
    // TODO: Implement download completion notification

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
    */

    Group group{
        storage,
        NetworkQueryTask{onQuerySetup, onQueryDone},
        Sync{onPluginInstallation},
        Sync{[this]() { updateView(m_extensionBrowser->currentIndex()); }},
        //NetworkQueryTask{onDownloadSetup, onDownloadDone},
    };

    m_dlTaskTreeRunner.start(group);
}

QWidget *createExtensionManagerWidget()
{
    return new ExtensionManagerWidget;
}

} // ExtensionManager::Internal

#include "extensionmanagerwidget.moc"
