// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagerwidget.h"

#include "extensionmanagerconstants.h"
#include "extensionmanagertr.h"
#include "extensionsbrowser.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwelcomepage.h>
#include <coreplugin/modemanager.h> // TODO: Remove!
#include <coreplugin/welcomepagehelper.h>

#include <extensionsystem/pluginspec.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QTextBrowser>

using namespace Core;
using namespace Utils;

namespace ExtensionManager::Internal {

static QWidget *createVr(QWidget *parent = nullptr)
{
    auto vr = new QWidget(parent);
    vr->setFixedWidth(1);
    setBackgroundColor(vr, Theme::Token_Stroke_Subtle);
    return vr;
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

ExtensionManagerWidget::ExtensionManagerWidget()
{
    m_leftColumn = new ExtensionsBrowser;

    auto descriptionColumns = new QWidget;

    m_secondarDescriptionWidget = new CollapsingWidget;

    m_primaryDescription = new QTextBrowser;
    m_primaryDescription->setOpenExternalLinks(true);
    m_primaryDescription->setFrameStyle(QFrame::NoFrame);

    m_secondaryDescription = new QTextBrowser;
    m_secondaryDescription->setFrameStyle(QFrame::NoFrame);

    using namespace Layouting;
    Row {
        createVr(),
        m_secondaryDescription,
        noMargin(), spacing(0),
    }.attachTo(m_secondarDescriptionWidget);

    Row {
        createVr(),
        Row {
            m_primaryDescription,
            noMargin(),
        },
        m_secondarDescriptionWidget,
        noMargin(), spacing(0),
    }.attachTo(descriptionColumns);

    Row {
        Space(WelcomePageHelpers::HSpacing),
            m_leftColumn,
        descriptionColumns,
        noMargin(), spacing(0),
    }.attachTo(this);

    setBackgroundColor(this, Theme::Token_Background_Default);

    connect(m_leftColumn, &ExtensionsBrowser::itemSelected,
            this, &ExtensionManagerWidget::updateView);
    connect(this, &ResizeSignallingWidget::resized, this, [this](const QSize &size) {
        const int intendedLeftColumnWidth = size.width() - 580;
        m_leftColumn->adjustToWidth(intendedLeftColumnWidth);
        const bool secondaryDescriptionVisible = size.width() > 970;
        const int secondaryDescriptionWidth = secondaryDescriptionVisible ? 264 : 0;
        m_secondarDescriptionWidget->setWidth(secondaryDescriptionWidth);
    });
    updateView({}, {});
}

void ExtensionManagerWidget::updateView(const QModelIndex &current,
                                        [[maybe_unused]] const QModelIndex &previous)
{
    const QString h5Css =
        StyleHelper::fontToCssProperties(StyleHelper::uiFont(StyleHelper::UiElementH5))
        + "; margin-top: 28px;";
    const QString h6Css =
        StyleHelper::fontToCssProperties(StyleHelper::uiFont(StyleHelper::UiElementH6))
        + "; margin-top: 28px;";
    const QString h6CapitalCss =
        StyleHelper::fontToCssProperties(StyleHelper::uiFont(StyleHelper::UiElementH6Capital))
        + QString::fromLatin1("; color: %1;")
              .arg(creatorTheme()->color(Theme::Token_Text_Muted).name());
    const QString bodyStyle = QString::fromLatin1("color: %1; background-color: %2;"
                                                  "margin-left: %3px; margin-right: %3px;")
                                  .arg(creatorTheme()->color(Theme::Token_Text_Default).name())
                                  .arg(creatorTheme()->color(Theme::Token_Background_Muted).name())
                                  .arg(WelcomePageHelpers::HSpacing);
    const QString htmlStart = QString(R"(
        <html>
        <body style="%1">
    )").arg(bodyStyle);
    const QString htmlEnd = QString(R"(
        </body></html>
    )");

    if (!current.isValid()) {
        const QString emptyHtml = htmlStart + htmlEnd;
        m_primaryDescription->setText(emptyHtml);
        m_secondaryDescription->setText(emptyHtml);
        return;
    }

    const ItemData data = itemData(current);
    const bool isPack = data.type == ItemTypePack;
    const ExtensionSystem::PluginSpec *extension = data.plugins.first();

    {
        const QString shortDescription =
            isPack ? QLatin1String("Short description for pack ") + data.name
                   : extension->description();
        QString longDescription =
            isPack ? QLatin1String("Some longer text that describes the purpose and functionality "
                                   "of the extensions that are part of pack ") + data.name
                   : extension->longDescription();
        longDescription.replace("\n", "<br/>");
        const QString location = isPack ? extension->location() : extension->filePath();

        QString description = htmlStart;

        description.append(QString(R"(
            <div style="%1"><br/>%2</div>
            <p>%3</p>
        )").arg(h5Css)
           .arg(shortDescription)
           .arg(longDescription));

        description.append(QString(R"(
            <div style="%1">%2</div>
            <p>%3</p>
        )").arg(h6Css)
           .arg(Tr::tr("Get started"))
           .arg(Tr::tr("Install the extension from above. Installation starts automatically. "
                       "You can always uninstall the extension afterwards.")));

        description.append(QString(R"(
            <div style="%1">%2</div>
            <p>
            <a href="%3">%4 &gt;</a>
            <br/>
            <a href="%5">%6 &gt;</a>
            </p>
        )").arg(h6Css)
           .arg(Tr::tr("More information"))
           .arg(Tr::tr("Online Documentation"))
           .arg("https://doc.qt.io/qtcreator/")
           .arg(Tr::tr("Tutorials"))
           .arg("https://doc.qt.io/qtcreator/creator-tutorials.html"));

        const QString examplesBoxCss =
            QString::fromLatin1("height: 168px; background-color: %1; ")
                .arg(creatorTheme()->color(Theme::Token_Background_Default).name());
        description.append(QString(R"(
            <div style="%1">%2</div>
            <p style="%3">
                <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>
            </p>
        )").arg(h6CapitalCss)
           .arg(Tr::tr("Examples"))
           .arg(examplesBoxCss));

        const QString captionStrongCss = StyleHelper::fontToCssProperties(
            StyleHelper::uiFont(StyleHelper::UiElementCaptionStrong));
        description.append(QString(R"(
            <div style="%1">%2</div>
            <p>
                <table>
                    <tr><td style="%3">%4</td><td>%5</td></tr>
                    <tr><td style="%3">%6</td><td>%7</td></tr>
                    <tr><td style="%3">%8</td><td>%9</td></tr>
                </table>
            </p>
        )").arg(h6Css)
           .arg(Tr::tr("Extension library details"))
           .arg(captionStrongCss)
           .arg(Tr::tr("Size"))
           .arg("547 MB")
           .arg(Tr::tr("Version"))
           .arg(extension->version())
           .arg(Tr::tr("Location"))
           .arg(location));

        description.append(htmlEnd);
        m_primaryDescription->setText(description);
    }

    {
        QString description = htmlStart;

        description.append(QString(R"(
            <p style="%1"><br/>%2</p>
        )").arg(h6CapitalCss)
           .arg(Tr::tr("Extension details")));

        description.append(QString(R"(
            <div style="%1">%2</div>
            <p>%3</p>
        )").arg(h6Css)
           .arg(Tr::tr("Released"))
           .arg("23.5.2023"));

        const QString tagTemplate = QString(R"(
            <td style="border: 1px solid %1; padding: 3px; ">%2</td>
        )").arg(creatorTheme()->color(Theme::Token_Stroke_Subtle).name());
        const QStringList tags = Utils::transform(data.tags,
                                                  [&tagTemplate] (const QString &tag) {
                                                      return tagTemplate.arg(tag);
                                                  });
        description.append(QString(R"(
            <div style="%1">%2</div>
            <p>%3</p>
        )").arg(h6Css)
           .arg(Tr::tr("Related tags"))
           .arg(tags.join("&nbsp;")));

        description.append(QString(R"(
            <div style="%1">%2</div>
            <p>
                macOS<br/>
                Windows<br/>
                Linux
            </p>
        )").arg(h6Css)
           .arg(Tr::tr("Platforms")));

        QStringList dependencies;
        for (const ExtensionSystem::PluginSpec *spec : data.plugins) {
            dependencies.append(Utils::transform(spec->dependencies(),
                                                 &ExtensionSystem::PluginDependency::toString));
        }
        dependencies.removeDuplicates();
        dependencies.sort();
        description.append(QString(R"(
            <div style="%1">%2</div>
            <p>%3</p>
        )").arg(h6Css)
           .arg(Tr::tr("Dependencies"))
           .arg(dependencies.isEmpty() ? "-" : dependencies.join("<br/>")));

        if (isPack) {
            const QStringList extensions = Utils::transform(data.plugins,
                                                            &ExtensionSystem::PluginSpec::name);
            description.append(QString(R"(
                <div style="%1">%2</div>
                <p>%3</p>
            )").arg(h6Css)
           .arg(Tr::tr("Extensions in pack"))
           .arg(extensions.join("<br/>")));
        }

        description.append(htmlEnd);
        m_secondaryDescription->setText(description);
    }
}

} // ExtensionManager::Internal
