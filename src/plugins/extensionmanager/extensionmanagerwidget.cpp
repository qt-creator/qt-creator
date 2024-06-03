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
#include <utils/stylehelper.h>
#include <utils/temporarydirectory.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QCheckBox>
#include <QMessageBox>
#include <QTextBrowser>
#include <QProgressDialog>

using namespace Core;
using namespace Utils;

namespace ExtensionManager::Internal {

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

class PluginStatusWidget : public QWidget
{
public:
    explicit PluginStatusWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_label = new InfoLabel;
        m_checkBox = new QCheckBox(Tr::tr("Load on Start"));

        using namespace Layouting;
        Column {
            m_label,
            m_checkBox,
        }.attachTo(this);

        connect(m_checkBox, &QCheckBox::clicked, this, [this](bool checked) {
            ExtensionSystem::PluginSpec *spec = ExtensionsModel::pluginSpecForName(m_pluginName);
            if (spec == nullptr)
                return;
            spec->setEnabledBySettings(checked);
            ExtensionSystem::PluginManager::writeSettings();
        });

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
        const ExtensionSystem::PluginSpec *spec = ExtensionsModel::pluginSpecForName(m_pluginName);
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
    QString m_pluginName;
};

class ExtensionManagerWidgetPrivate
{
public:
    QString currentItemName;
    ExtensionsBrowser *leftColumn;
    CollapsingWidget *secondaryDescriptionWidget;
    QTextBrowser *primaryDescription;
    QTextBrowser *secondaryDescription;
    PluginStatusWidget *pluginStatus;
    QAbstractButton *installButton;
    PluginsData currentItemPlugins;
    Tasking::TaskTreeRunner taskTreeRunner;
};

ExtensionManagerWidget::ExtensionManagerWidget(QWidget *parent)
    : ResizeSignallingWidget(parent)
    , d(new ExtensionManagerWidgetPrivate)
{
    d->leftColumn = new ExtensionsBrowser;

    auto descriptionColumns = new QWidget;

    d->secondaryDescriptionWidget = new CollapsingWidget;

    d->primaryDescription = new QTextBrowser;
    d->primaryDescription->setOpenExternalLinks(true);
    d->primaryDescription->setFrameStyle(QFrame::NoFrame);

    d->secondaryDescription = new QTextBrowser;
    d->secondaryDescription->setFrameStyle(QFrame::NoFrame);

    d->pluginStatus = new PluginStatusWidget;

    d->installButton = new Button(Tr::tr("Install..."), Button::MediumPrimary);
    d->installButton->hide();

    using namespace Layouting;
    Row {
        WelcomePageHelpers::createRule(Qt::Vertical),
        Column {
            d->secondaryDescription,
            d->pluginStatus,
            d->installButton,
        },
        noMargin, spacing(0),
    }.attachTo(d->secondaryDescriptionWidget);

    Row {
        WelcomePageHelpers::createRule(Qt::Vertical),
        Row {
            d->primaryDescription,
            noMargin,
        },
        d->secondaryDescriptionWidget,
        noMargin, spacing(0),
    }.attachTo(descriptionColumns);

    Row {
        Space(StyleHelper::SpacingTokens::ExVPaddingGapXl),
        d->leftColumn,
        descriptionColumns,
        noMargin, spacing(0),
    }.attachTo(this);

    WelcomePageHelpers::setBackgroundColor(this, Theme::Token_Background_Default);

    connect(d->leftColumn, &ExtensionsBrowser::itemSelected,
            this, &ExtensionManagerWidget::updateView);
    connect(this, &ResizeSignallingWidget::resized, this, [this](const QSize &size) {
        const int intendedLeftColumnWidth = size.width() - 580;
        d->leftColumn->adjustToWidth(intendedLeftColumnWidth);
        const bool secondaryDescriptionVisible = size.width() > 970;
        const int secondaryDescriptionWidth = secondaryDescriptionVisible ? 264 : 0;
        d->secondaryDescriptionWidget->setWidth(secondaryDescriptionWidth);
    });
    connect(d->installButton, &QAbstractButton::pressed, this, [this]() {
        fetchAndInstallPlugin(QUrl::fromUserInput(d->currentItemPlugins.constFirst().second));
    });
    updateView({});
}

ExtensionManagerWidget::~ExtensionManagerWidget()
{
    delete d;
}

void ExtensionManagerWidget::updateView(const QModelIndex &current)
{
    const QString h5Css =
        StyleHelper::fontToCssProperties(StyleHelper::uiFont(StyleHelper::UiElementH5))
        + "; margin-top: 0px;";
    const QString h6Css =
        StyleHelper::fontToCssProperties(StyleHelper::uiFont(StyleHelper::UiElementH6))
        + "; margin-top: 28px;";
    const QString h6CapitalCss =
        StyleHelper::fontToCssProperties(StyleHelper::uiFont(StyleHelper::UiElementH6Capital))
        + QString::fromLatin1("; margin-top: 0px; color: %1;")
              .arg(creatorColor(Theme::Token_Text_Muted).name());
    const QString bodyStyle = QString::fromLatin1("color: %1; background-color: %2; "
                                                  "margin-left: %3px; margin-right: %3px;")
                                  .arg(creatorColor(Theme::Token_Text_Default).name())
                                  .arg(creatorColor(Theme::Token_Background_Muted).name())
                                  .arg(StyleHelper::SpacingTokens::ExVPaddingGapXl);
    const QString htmlStart = QString(R"(
        <html>
        <body style="%1"><br/>
    )").arg(bodyStyle);
    const QString htmlEnd = QString(R"(
        </body></html>
    )");

    if (!current.isValid()) {
        const QString emptyHtml = htmlStart + htmlEnd;
        d->primaryDescription->setText(emptyHtml);
        d->secondaryDescription->setText(emptyHtml);
        return;
    }

    d->currentItemName = current.data().toString();
    const bool isPack = current.data(RoleItemType) == ItemTypePack;
    d->pluginStatus->setPluginName(isPack ? QString() : d->currentItemName);
    const bool isRemotePlugin = !(isPack || ExtensionsModel::pluginSpecForName(d->currentItemName));
    d->currentItemPlugins = current.data(RolePlugins).value<PluginsData>();
    d->installButton->setVisible(isRemotePlugin && !d->currentItemPlugins.empty());
    if (!d->currentItemPlugins.empty())
        d->installButton->setToolTip(d->currentItemPlugins.constFirst().second);

    {
        QString description = htmlStart;

        QString descriptionHtml;
        {
            const TextData textData = current.data(RoleDescriptionText).value<TextData>();
            for (const TextData::Type &text : textData) {
                if (text.second.isEmpty())
                    continue;
                const QString paragraph =
                    QString::fromLatin1("<div style=\"%1\">%2</div><p>%3</p>")
                        .arg(descriptionHtml.isEmpty() ? h5Css : h6Css)
                        .arg(text.first)
                        .arg(text.second.join("<br/>"));
                descriptionHtml.append(paragraph);
            }
        }
        description.append(descriptionHtml);

        description.append(QString::fromLatin1("<div style=\"%1\">%2</div>")
                               .arg(h6Css)
                               .arg(Tr::tr("More information")));
        const LinksData linksData = current.data(RoleDescriptionLinks).value<LinksData>();
        if (!linksData.isEmpty()) {
            QString linksHtml;
            const QStringList links = transform(linksData, [](const LinksData::Type &link) {
                const QString anchor = link.first.isEmpty() ? link.second : link.first;
                return QString::fromLatin1("<a href=\"%1\">%2 &gt;</a>")
                    .arg(link.second).arg(anchor);
            });
            linksHtml = links.join("<br/>");
            description.append(QString::fromLatin1("<p>%1</p>").arg(linksHtml));
        }

        const ImagesData imagesData = current.data(RoleDescriptionImages).value<ImagesData>();
        if (!imagesData.isEmpty()) {
            const QString examplesBoxCss =
                QString::fromLatin1("height: 168px; background-color: %1; ")
                    .arg(creatorColor(Theme::Token_Background_Default).name());
            description.append(QString(R"(
                <br/>
                <div style="%1">%2</div>
                <p style="%3">
                    <br/><br/><br/><br/><br/>
                    TODO: Load imagea asynchronously, and show them in a QLabel.
                    Also Use QMovie for animated images.
                    <br/><br/><br/><br/><br/>
                </p>
            )").arg(h6CapitalCss)
               .arg(Tr::tr("Examples"))
               .arg(examplesBoxCss));
        }

        // Library details vanished from the Figma designs. The data is available, though.
        const bool showDetails = false;
        if (showDetails) {
            const QString captionStrongCss = StyleHelper::fontToCssProperties(
                StyleHelper::uiFont(StyleHelper::UiElementCaptionStrong));
            const QLocale locale;
            const uint size = current.data(RoleSize).toUInt();
            const QString sizeFmt = locale.formattedDataSize(size);
            const FilePath location = FilePath::fromVariant(current.data(RoleLocation));
            const QString version = current.data(RoleVersion).toString();
            description.append(QString(R"(
                <div style="%1">%2</div>
                <p>
                    <table>
                        <tr><td style="%3">%4</td><td>%5</td></tr>
                        <tr><td style="%3">%6</td><td>%7</td></tr>
            )").arg(h6Css)
               .arg(Tr::tr("Extension library details"))
               .arg(captionStrongCss)
               .arg(Tr::tr("Size"))
               .arg(sizeFmt)
               .arg(Tr::tr("Version"))
               .arg(version));
            if (!location.isEmpty()) {
                const QString locationFmt =
                    HostOsInfo::isWindowsHost() ? location.toUserOutput()
                                                : location.withTildeHomePath();
                description.append(QString(R"(
                            <tr><td style="%3">%1</td><td>%2</td></tr>
                )").arg(Tr::tr("Location"))
                   .arg(locationFmt));
            }
            description.append(QString(R"(
                    </table>
                </p>
            )"));
        }

        description.append(htmlEnd);
        d->primaryDescription->setText(description);
    }

    {
        QString description = htmlStart;

        description.append(QString(R"(
            <p style="%1">%2</p>
        )").arg(h6CapitalCss)
           .arg(Tr::tr("Extension details")));

        const QStringList tags = current.data(RoleTags).toStringList();
        if (!tags.isEmpty()) {
            const QString tagTemplate = QString(R"(
                <td style="border: 1px solid %1; padding: 3px; ">%2</td>
            )").arg(creatorColor(Theme::Token_Stroke_Subtle).name());
            const QStringList tagsFmt = transform(tags, [&tagTemplate](const QString &tag) {
                return tagTemplate.arg(tag);
            });
            description.append(QString(R"(
                <div style="%1">%2</div>
                <p>%3</p>
            )").arg(h6Css)
               .arg(Tr::tr("Related tags"))
               .arg(tagsFmt.join("&nbsp;")));
        }

        const QStringList platforms = current.data(RolePlatforms).toStringList();
        if (!platforms.isEmpty()) {
            description.append(QString(R"(
                <div style="%1">%2</div>
                <p>%3</p>
            )").arg(h6Css)
               .arg(Tr::tr("Platforms"))
               .arg(platforms.join("<br/>")));
        }

        const QStringList dependencies = current.data(RoleDependencies).toStringList();
        if (!dependencies.isEmpty()) {
            const QString dependenciesFmt = dependencies.join("<br/>");
            description.append(QString(R"(
                <div style="%1">%2</div>
                <p>%3</p>
            )").arg(h6Css)
               .arg(Tr::tr("Dependencies"))
               .arg(dependenciesFmt));
        }

        if (isPack) {
            const PluginsData plugins = current.data(RolePlugins).value<PluginsData>();
            const QStringList extensions = transform(plugins, &QPair<QString, QString>::first);
            const QString extensionsFmt = extensions.join("<br/>");
            description.append(QString(R"(
                <div style="%1">%2</div>
                <p>%3</p>
            )").arg(h6Css)
               .arg(Tr::tr("Extensions in pack"))
               .arg(extensionsFmt));
        }

        description.append(htmlEnd);
        d->secondaryDescription->setText(description);
    }
}

void ExtensionManagerWidget::fetchAndInstallPlugin(const QUrl &url)
{
    using namespace Tasking;

    struct StorageStruct
    {
        StorageStruct() {
            progressDialog.reset(new QProgressDialog(Tr::tr("Downloading Plugin..."),
                                                     Tr::tr("Cancel"), 0, 0,
                                                     ICore::dialogParent()));
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
                Tr::tr("Could not download Plugin") + "\n\n" + storage->url.toString() + "\n\n"
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

    d->taskTreeRunner.start(group);
}

} // ExtensionManager::Internal
