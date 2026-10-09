// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagerwidget.h"

#include "extensionmanagerconstants.h"
#include "extensionmanagertr.h"
#include "extensionsbrowser.h"
#include "extensionsmodel.h"
#include "remotespec.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwelcomepage.h>
#include <coreplugin/plugininstallwizard.h>
#include <coreplugin/welcomepagehelper.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginview.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QTaskTree>
#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/algorithm.h>
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
#include <utils/qtdesignwidgets.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/temporarydirectory.h>
#include <utils/textutils.h>
#include <utils/widgets.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMetaEnum>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QtMath>

using namespace Core;
using namespace ExtensionSystem;
using namespace QtTaskTree;
using namespace Utils;
using namespace Utils::StyleHelper;

namespace ExtensionManager::Internal {

[[maybe_unused]] static Q_LOGGING_CATEGORY(widgetLog, "qtc.extensionmanager.widget", QtWarningMsg)

const char kRestartSetting[] = "RestartAfterPluginEnabledChanged";
const char kUrlSchemeTag[] = "myschemetag";
const char kUrlSchemeDependency[] = "myschemedependency";

// Remembers the selected extension across re-creations of the widget, e.g.
// when the browser is torn down and rebuilt. May refer to an extension that
// is only known once remote extensions are fetched.
static QString &lastSelectedExtensionId()
{
    static QString id;
    return id;
}

static void requestRestart()
{
    InfoBar *infoBar = ICore::popupInfoBar();
    if (infoBar->canInfoBeAdded(kRestartSetting)) {
        Utils::InfoBarEntry info(kRestartSetting, msgPluginChangesRequireRestart());
        info.setTitle(Tr::tr("Restart Required"));
        info.setInfoType(InfoLabelType::Information);
        info.addCustomButton(
            ICore::msgRestartNow(), [] { ICore::restart(); }, {}, InfoBarEntry::ButtonAction::Hide);
        infoBar->addInfo(info);
    }
}

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
                versionStr += " " + Tr::tr("(Incompatible)");
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
    QSingleTaskTreeRunner m_fetchVersionsRunner;
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
        m_vendor = new QtcButton({}, QtcButton::SmallLink);
        m_vendor->setContentsMargins({});
        m_divider = new QLabel;
        m_divider->setFixedSize(1, dividerH);
        StyleHelper::setBackgroundColor(m_divider, dlTF.themeColor);
        m_dlIcon = new QLabel;
        const QPixmap dlIcon = Icon({{":/extensionmanager/images/download.png", dlTF.themeColor}},
                                    Icon::Tint).pixmap();
        m_dlIcon->setPixmap(dlIcon);
        m_dlCount = new ElidingLabel;
        applyTf(m_dlCount, dlTF);
        m_dlCount->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        m_details = new ElidingLabel;
        applyTf(m_details, detailsTF);
        installButton = new QtcButton(Tr::tr("Install..."), QtcButton::SmallPrimary);
        installButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        installButton->hide();
        connect(
            installButton,
            &QAbstractButton::pressed,
            this,
            &HeadingWidget::pluginInstallationRequested);

        removeButton = new QtcButton(Tr::tr("Remove..."), QtcButton::SmallSecondary);
        removeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        removeButton->hide();
        connect(removeButton, &QAbstractButton::pressed, this, [this]() {
            PluginManager::removePluginOnRestart(m_currentPluginId);
            requestRestart();
        });

        updateButton = new QtcButton(Tr::tr("Update..."), QtcButton::SmallPrimary);
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
                            Space(SpacingTokens::GapHM),
                            m_divider,
                            Space(SpacingTokens::GapHM),
                            m_dlIcon,
                            Space(SpacingTokens::GapHXs),
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
            noMargin, spacing(SpacingTokens::GapHL),
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
    QtcButton *m_vendor;
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
        m_switch = new QtcSwitch(Tr::tr("Active"));
        m_pluginView.hide();
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        using namespace Layouting;
        Grid {
            Span(2, m_label), br,
            m_switch, empty, br,
            spacing(SpacingTokens::GapVXs),
            noMargin,
        }.attachTo(this);

        connect(m_switch, &QtcSwitch::clicked, this, [this](bool checked) {
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
            m_label->setType(InfoLabelType::Error);
            m_label->setText(Tr::tr("Error"));
        } else if (spec->state() == PluginSpec::Running) {
            m_label->setType(InfoLabelType::Ok);
            m_label->setText(Tr::tr("Loaded"));
        } else {
            m_label->setType(InfoLabelType::NotOk);
            m_label->setText(Tr::tr("Not loaded"));
        }
        m_label->setAdditionalToolTip(spec->errorString());

        m_switch->setChecked(spec->isRequired() || spec->isEnabledBySettings());
        m_switch->setEnabled(!spec->isRequired());
    }

    InfoLabel *m_label;
    QtcSwitch *m_switch;
    QString m_pluginId;
    PluginView m_pluginView{this};
};

static QString linkHtml(const QString &href, const QString &text)
{
    return QString("<a href=\"%1\" style=\"color:%2; text-decoration:none\">%3</a>")
        .arg(href.toHtmlEscaped(),
             creatorColor(Theme::Token_Text_Accent).name(),
             text.toHtmlEscaped());
}

class ElidedLinkLabel final : public QLabel
{
public:
    ElidedLinkLabel()
    {
        setOpenExternalLinks(true);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }

    void setUrl(const QString &url)
    {
        m_url = url;
        setToolTip(url);
        updateText();
    }

    QSize minimumSizeHint() const override { return {0, QLabel::minimumSizeHint().height()}; }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QLabel::resizeEvent(event);
        updateText();
    }

private:
    void updateText()
    {
        setText(linkHtml(m_url, fontMetrics().elidedText(m_url, Qt::ElideRight, width())));
    }

    QString m_url;
};

class PropertiesWidget : public QWidget
{
public:
    using DisplayNameForId = std::function<QString(const QString &)>;
    using LinkHandler = std::function<void(const QUrl &)>;

    PropertiesWidget(const DisplayNameForId &displayNameForId, const LinkHandler &linkHandler)
        : m_displayNameForId(displayNameForId)
    {
        const auto linkListLabel = [linkHandler] {
            auto label = createValueLabel(false);
            label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
            connect(label, &QLabel::linkActivated, label, [linkHandler](const QString &link) {
                linkHandler(QUrl(link));
            });
            return label;
        };
        const auto urlLabel = [] {
            auto label = new ElidedLinkLabel;
            applyTf(label, valueTF, false);
            label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
            return label;
        };

        m_version = createValueLabel();
        m_compatVersion = createValueLabel();
        m_id = createValueLabel();
        m_vendorId = createValueLabel();
        m_group = createValueLabel();
        m_location = createValueLabel();
        m_location->setElideMode(Qt::ElideMiddle);
        m_platforms = createValueLabel(false);
        m_softLoadable = createValueLabel();
        m_lastUpdate = createValueLabel();
        m_moreInfo = urlLabel();
        m_documentation = urlLabel();
        m_dependencies = linkListLabel();
        m_packPlugins = linkListLabel();
        m_tags = linkListLabel();
        m_copyright = createValueLabel(false);
        m_license = createValueLabel(false);

        const Icon folderIcon({{":/utils/images/fileopen.png", Theme::Token_Text_Muted}},
                              Icon::Tint);
        auto showLocationButton = new QtcButton({}, QtcButton::SmallGhost);
        showLocationButton->setThemedPixmap(folderIcon);
        showLocationButton->setToolTip(Core::FileUtils::msgGraphicalShellAction());
        showLocationButton->setAccessibleName(Core::FileUtils::msgGraphicalShellAction());
        showLocationButton->setCursor(Qt::PointingHandCursor);
        showLocationButton->setFixedSize(
            folderIcon.pixmap().deviceIndependentSize().toSize()
            + QSize(2 * SpacingTokens::PaddingHXxs, 2 * SpacingTokens::PaddingVXxs));
        connect(showLocationButton, &QAbstractButton::clicked, this, [this] {
            Core::FileUtils::showInGraphicalShell(m_locationPath);
        });

        m_blocks = {
            {{field(Tr::tr("More Information"), m_moreInfo)},
             {field(Tr::tr("Documentation"), m_documentation)}},
            {{field(Tr::tr("Version"), m_version),
              field(Tr::tr("Compatibility Version"), m_compatVersion),
              field(Tr::tr("Last Update"), m_lastUpdate)},
             {field(Tr::tr("ID"), m_id),
              field(Tr::tr("Vendor ID"), m_vendorId)}},
            {{field(Tr::tr("Group"), m_group)},
             {field(Tr::tr("Location"), m_location, showLocationButton)}},
            {{field(Tr::tr("Platforms"), m_platforms)},
             {field(Tr::tr("Loadable Without Restart"), m_softLoadable)}},
            {{field(Tr::tr("Dependencies"), m_dependencies),
              field(Tr::tr("Extensions in Pack"), m_packPlugins)},
             {field(Tr::tr("Tags"), m_tags)}},
            {{field(Tr::tr("Copyright"), m_copyright)}, {}, true},
            {{field(Tr::tr("License"), m_license)}, {}, true},
        };

        static const TextFormat headingTF{Theme::Token_Text_Default, UiElementH6Capital};
        auto heading = new QLabel(Tr::tr("More Information"));
        applyTf(heading, headingTF);

        m_grid = new QGridLayout;
        m_grid->setContentsMargins({});
        m_grid->setHorizontalSpacing(SpacingTokens::GapHXl);
        m_grid->setVerticalSpacing(SpacingTokens::GapVL);
        m_grid->setColumnStretch(0, 1);
        m_grid->setColumnStretch(1, 1);

        using namespace Layouting;
        // clang-format off
        Column {
            heading,
            m_grid,
            customMargins(SpacingTokens::PaddingHXl, 0, bigSpacing, SpacingTokens::PaddingVXl),
            spacing(SpacingTokens::GapVL),
        }.attachTo(this);
        // clang-format on

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    }

    // The heading shows the version of an extension that is not installed.
    void setData(const QModelIndex &index, const PluginSpec *spec, bool isInstalled)
    {
        QString version;
        if (spec && isInstalled) {
            version = spec->version();
            if (!spec->revision().isEmpty())
                version += " (" + spec->revision() + ")";
        }
        setValue(m_version, version);
        setValue(m_compatVersion, spec ? spec->compatVersion() : QString());
        setValue(m_id, index.data(RoleId).toString());
        setValue(m_vendorId, index.data(RoleVendorId).toString());
        setValue(m_group, spec ? spec->category() : QString());
        m_locationPath = spec ? spec->filePath() : FilePath();
        setValue(m_location, m_locationPath.toUserOutput());

        setValue(m_platforms, index.data(RolePlatforms).toStringList().join(", "));
        setValue(m_softLoadable,
                 spec ? (spec->isSoftLoadable() ? Tr::tr("Yes") : Tr::tr("No")) : QString());
        const QDate dateUpdated = index.data(RoleDateUpdated).toDate();
        setValue(m_lastUpdate, dateUpdated.isValid() ? dateUpdated.toString() : QString());

        const QString moreInfoUrl = index.data(RoleMoreInfoUrl).toString();
        m_moreInfo->setUrl(moreInfoUrl);
        m_shown[m_moreInfo->parentWidget()] = !moreInfoUrl.isEmpty();
        const QString documentationUrl = index.data(RoleDocumentationUrl).toString();
        m_documentation->setUrl(documentationUrl);
        m_shown[m_documentation->parentWidget()] = !documentationUrl.isEmpty();

        setValue(m_dependencies,
                 links(index.data(RoleDependencies).toStringList(), kUrlSchemeDependency,
                       m_displayNameForId));
        const bool isPack = index.data(RoleItemType) == ItemTypePack;
        setValue(m_packPlugins,
                 isPack ? links(index.data(RolePlugins).toStringList(), kUrlSchemeDependency,
                                m_displayNameForId)
                        : QString());
        setValue(m_tags, links(index.data(RoleTags).toStringList(), kUrlSchemeTag,
                               [](const QString &tag) { return tag; }));

        setValue(m_copyright, spec ? spec->copyright() : QString());
        setValue(m_license, spec ? spec->license() : QString());

        relayout();
    }

private:
    static constexpr TextFormat valueTF{Theme::Token_Text_Default, UiElementBody2};

    // Fields stacked in the left and the right column, or spanning both.
    struct Block
    {
        QList<QWidget *> left;
        QList<QWidget *> right;
        bool fullWidth = false;
    };

    static ElidingLabel *createValueLabel(bool singleLine = true)
    {
        auto label = new ElidingLabel;
        applyTf(label, valueTF, false);
        label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        if (!singleLine) {
            label->setElideMode(Qt::ElideNone);
            label->setWordWrap(true);
        }
        return label;
    }

    QWidget *field(const QString &title, QLabel *value, QWidget *action = nullptr)
    {
        static const TextFormat titleTF{Theme::Token_Text_Muted, UiElementLabelSmall};
        auto titleLabel = new QLabel(title);
        applyTf(titleLabel, titleTF);

        auto widget = new QWidget(this);
        widget->hide();
        using namespace Layouting;
        Column {
            Row {
                titleLabel,
                If(action != nullptr) >> Then { action },
                st,
                noMargin,
                spacing(SpacingTokens::GapHXs),
            },
            value,
            noMargin,
            spacing(SpacingTokens::GapVXs),
        }.attachTo(widget);
        // Columns share the width by stretch, not by the length of their texts.
        widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        return widget;
    }

    static QString links(const QStringList &ids, const QString &scheme,
                         const DisplayNameForId &displayNameForId)
    {
        return Utils::transform(ids, [&](const QString &id) {
                   QUrl url;
                   url.setScheme(scheme);
                   url.setPath(id);
                   return linkHtml(url.toString(), displayNameForId(id));
               }).join(", ");
    }

    void setValue(QLabel *label, const QString &value)
    {
        label->setText(value);
        m_shown[label->parentWidget()] = !value.isEmpty();
    }

    // Stacks the shown fields of each block in their column, so that a missing
    // field does not leave a hole.
    void relayout()
    {
        while (QLayoutItem *item = m_grid->takeAt(0)) {
            if (item->widget())
                item->widget()->hide();
            delete item;
        }

        for (int row = 0; row < m_grid->rowCount(); ++row)
            m_grid->setRowMinimumHeight(row, 0);

        int row = 0;
        for (const Block &block : std::as_const(m_blocks)) {
            // An empty row with a height separates the blocks more than their fields.
            if (row > 0)
                m_grid->setRowMinimumHeight(row++, SpacingTokens::GapVXl);
            const auto addColumn = [this, &block, row](const QList<QWidget *> &fields, int column) {
                int fieldRow = row;
                for (QWidget *field : fields) {
                    if (!m_shown.value(field))
                        continue;
                    m_grid->addWidget(field, fieldRow++, column, 1, block.fullWidth ? 2 : 1);
                    field->show();
                }
                return fieldRow - row;
            };
            const int blockRows = qMax(addColumn(block.left, 0), addColumn(block.right, 1));
            if (blockRows == 0 && row > 0)
                m_grid->setRowMinimumHeight(--row, 0);
            row += blockRows;
        }
        setVisible(row > 0);
    }

    DisplayNameForId m_displayNameForId;
    QList<Block> m_blocks;
    QHash<QWidget *, bool> m_shown;
    QGridLayout *m_grid;
    ElidingLabel *m_version;
    ElidingLabel *m_compatVersion;
    ElidingLabel *m_id;
    ElidingLabel *m_vendorId;
    ElidingLabel *m_group;
    ElidingLabel *m_location;
    ElidingLabel *m_platforms;
    ElidingLabel *m_softLoadable;
    ElidingLabel *m_lastUpdate;
    ElidedLinkLabel *m_moreInfo;
    ElidedLinkLabel *m_documentation;
    ElidingLabel *m_dependencies;
    ElidingLabel *m_packPlugins;
    ElidingLabel *m_tags;
    ElidingLabel *m_copyright;
    ElidingLabel *m_license;
    FilePath m_locationPath;
};

class ExtensionManagerWidget final : public Core::IOptionsPageWidget
{
public:
    ExtensionManagerWidget();
    ~ExtensionManagerWidget() override;

private:
    void resizeEvent(QResizeEvent *event) final;
    void updateView(const QModelIndex &current);
    void fetchAndInstallPlugin(const QUrl &url, bool update, const QString &sha);

    QString m_currentItemName;
    ExtensionsModel *m_extensionModel;
    ExtensionsBrowser *m_extensionBrowser;
    QStackedWidget *m_detailsStack;
    HeadingWidget *m_headingWidget;
    MarkdownBrowser *m_description;
    PluginStatusWidget *m_pluginStatus;
    PropertiesWidget *m_properties;
    QScrollArea *m_detailsScrollArea;
    QString m_currentDownloadUrl;
    QString m_currentId;
    QSingleTaskTreeRunner m_dlTaskTreeRunner;
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
            spacing(SpacingTokens::GapVL),
        },
        st,
        noMargin,
    }.attachTo(placeHolder);
    // clang-format on
    StyleHelper::setBackgroundColor(placeHolder, Theme::Token_Background_Muted);
    placeHolder->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    return placeHolder;
}

ExtensionManagerWidget::ExtensionManagerWidget()
{
    m_extensionModel = new ExtensionsModel(this);
    m_extensionBrowser = new ExtensionsBrowser(m_extensionModel);

    m_headingWidget = new HeadingWidget;
    m_description = new MarkdownBrowser;
    m_description->setWheelZoomEnabled(true);
    m_description->setAllowRemoteImages(true);
    m_description->setFrameStyle(QFrame::NoFrame);
    m_description->setOpenExternalLinks(true);
    const int verticalPadding = bigSpacing - SpacingTokens::PaddingVXl;
    m_description->setMargins({verticalPadding, 0, verticalPadding, 0});
    m_description->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_description->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_description->viewport()->setAutoFillBackground(false);
    connect(m_description->document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            m_description,
            [this](const QSizeF &size) {
                m_description->setFixedHeight(qCeil(size.height())
                                              + 2 * m_description->frameWidth());
            });


    m_pluginStatus = new PluginStatusWidget;
    m_properties = new PropertiesWidget(
        [this](const QString &id) {
            const QString displayName = m_extensionModel->indexOfId(id).data(RoleName).toString();
            return displayName.isEmpty() ? id : displayName;
        },
        [this](const QUrl &url) {
            if (url.scheme() == kUrlSchemeDependency)
                m_extensionBrowser->selectIndex(m_extensionModel->indexOfId(url.path()));
            else if (url.scheme() == kUrlSchemeTag)
                m_extensionBrowser->setFilter(url.path());
        });

    IContext::attach(this, Context(Constants::C_EXTENSIONMANAGER));

    auto detailsContent = new QWidget;
    detailsContent->setAutoFillBackground(false);
    m_detailsScrollArea = new QScrollArea;
    m_detailsScrollArea->setWidget(detailsContent);
    m_detailsScrollArea->setWidgetResizable(true);
    m_detailsScrollArea->setFrameShape(QFrame::NoFrame);
    m_detailsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_detailsScrollArea->viewport()->setAutoFillBackground(false);

    auto primaryDetailsColumn = new QWidget;

    using namespace Layouting;
    // clang-format off
    Column {
        m_description,
        m_properties,
        st,
        noMargin, spacing(0),
    }.attachTo(detailsContent);

    Column {
        Row {
            m_headingWidget,
            m_pluginStatus,
            customMargins(bigSpacing, bigSpacing,
                          bigSpacing, bigSpacing),
        },
        m_detailsScrollArea,
        noMargin, spacing(0),
    }.attachTo(primaryDetailsColumn);

    Row {
        Space(bigSpacing),
        m_extensionBrowser,
        WelcomePageHelpers::createRule(Qt::Vertical),
        Stack {
            bindTo(&m_detailsStack),
            descriptionPlaceHolder(),
            primaryDetailsColumn,
        },
        noMargin, spacing(0),
    }.attachTo(this);
    // clang-format on

    StyleHelper::setBackgroundColor(this, Theme::Token_Background_Default);

    connect(m_extensionBrowser, &ExtensionsBrowser::itemSelected,
            this, &ExtensionManagerWidget::updateView);

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

    const QString idToRestore = lastSelectedExtensionId();
    if (!idToRestore.isEmpty() && !m_extensionBrowser->selectId(idToRestore)) {
        // Not found yet, e.g. because remote extensions are not fetched yet.
        // Retry once the model has been (re-)populated.
        connect(m_extensionModel, &QAbstractItemModel::modelReset, this,
                [this, idToRestore] { m_extensionBrowser->selectId(idToRestore); },
                Qt::SingleShotConnection);
    }
}

ExtensionManagerWidget::~ExtensionManagerWidget()
{
    const QModelIndex current = m_extensionBrowser->currentIndex();
    lastSelectedExtensionId() = current.isValid() ? current.data(RoleId).toString() : QString();
}

void ExtensionManagerWidget::resizeEvent(QResizeEvent *event)
{
    IOptionsPageWidget::resizeEvent(event);
    m_extensionBrowser->adjustToWidth(width() / 3);
}

void ExtensionManagerWidget::updateView(const QModelIndex &current)
{
    const bool currentIsValid = current.isValid();

    if (currentIsValid) {
        m_detailsStack->setCurrentIndex(1);
    } else {
        m_detailsStack->setCurrentIndex(0);
        return;
    }

    m_headingWidget->update(current);

    m_currentItemName = current.data(RoleName).toString();
    const bool isPack = current.data(RoleItemType) == ItemTypePack;
    m_pluginStatus->setPluginId(isPack ? QString() : current.data(RoleId).toString());

    const PluginSpec *installedSpec
        = isPack ? nullptr : PluginManager::specById(current.data(RoleId).toString());
    const PluginSpec *spec = installedSpec;
    if (!isPack && !spec)
        spec = qvariant_cast<const RemoteSpec *>(current.data(RoleSpec));
    m_properties->setData(current, spec, installedSpec != nullptr);
    m_currentDownloadUrl = current.data(RoleDownloadUrl).toString();

    m_currentId = current.data(RoleFullId).toString();

    const QString description = current.data(RoleDescriptionLong).toString();
    m_description->setMarkdown(description);
    m_description->setVisible(!description.isEmpty());
    // Without a description, the details take the space it would have kept from the heading.
    QMargins propertiesMargins = m_properties->layout()->contentsMargins();
    propertiesMargins.setTop(description.isEmpty() ? SpacingTokens::PaddingVXl : 0);
    m_properties->layout()->setContentsMargins(propertiesMargins);
    m_description->document()->setDocumentMargin(SpacingTokens::PaddingVXl);
    m_detailsScrollArea->verticalScrollBar()->setValue(0);
}

void ExtensionManagerWidget::fetchAndInstallPlugin(const QUrl &url, bool update, const QString &sha)
{
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

    const auto onQuerySetup = [url, storage, sha](QNetworkReplyWrapper &query) {
        storage->url = url;
        storage->sha = sha;
        query.setRequest(QNetworkRequest(url));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };
    const auto onQueryDone = [storage](const QNetworkReplyWrapper &query, DoneWith result) -> DoneResult {
        storage->progressDialog->close();

        if (result != DoneWith::Success) {
            const QNetworkReply::NetworkError error = query.reply()->error();
            QMessageBox::warning(
                ICore::dialogParent(),
                Tr::tr("Download Error"),
                Tr::tr("Cannot download extension") + "\n\n" + storage->url.toString() + "\n\n"
                    + Tr::tr("Code: %1 (%2).")
                          .arg(error)
                          .arg(QString::fromUtf8(
                                   QMetaEnum::fromType<QNetworkReply::NetworkError>().key(error))));
            return DoneResult::Error;
        }

            storage->packageData = query.reply()->readAll();

        const QByteArray hash
            = QCryptographicHash::hash(storage->packageData, QCryptographicHash::Sha256);

        if (QString::fromLatin1(hash.toHex()) != storage->sha) {
            QMessageBox::warning(
                ICore::dialogParent(),
                Tr::tr("Download Error"),
                Tr::tr("Downloaded extension has an invalid hash."));
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
            Utils::FileUtils::showError(res.error());
        }
        return false;
    };

    /*
    // TODO: Implement download completion notification

    const auto onDownloadSetup = [id](QNetworkReplyWrapper &query) {
        query.setOperation(NetworkOperation::Post);
        query.setRequest(QNetworkRequest(
            QUrl(settings().externalRepoUrl() + "/api/v1/downloads/completed/" + id)));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };

    const auto onDownloadDone = [id](const QNetworkReplyWrapper &query, DoneWith result) {
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

    const Group recipe {
        storage,
        QNetworkReplyWrapperTask{onQuerySetup, onQueryDone},
        QSyncTask{onPluginInstallation},
        QSyncTask{[this] { updateView(m_extensionBrowser->currentIndex()); }},
        //QNetworkReplyWrapperTask{onDownloadSetup, onDownloadDone},
    };

    m_dlTaskTreeRunner.start(recipe);
}

IOptionsPageWidget *createExtensionManagerWidget()
{
    return new ExtensionManagerWidget;
}

} // ExtensionManager::Internal

#include "extensionmanagerwidget.moc"
