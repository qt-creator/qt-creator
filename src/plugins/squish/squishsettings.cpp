/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "squishsettings.h"

#include "squishconstants.h"
#include "squishtools.h"
#include "squishtr.h"

#include <utils/basetreeview.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QDialogButtonBox>
#include <QFrame>
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QXmlStreamReader>

using namespace Utils;

namespace Squish {
namespace Internal {

SquishSettings::SquishSettings()
{
    setSettingsGroup("Squish");
    setAutoApply(false);

    registerAspect(&squishPath);
    squishPath.setSettingsKey("SquishPath");
    squishPath.setLabelText(Tr::tr("Squish path:"));
    squishPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    squishPath.setExpectedKind(PathChooser::ExistingDirectory);
    squishPath.setPlaceHolderText(Tr::tr("Path to Squish installation"));

    registerAspect(&licensePath);
    licensePath.setSettingsKey("LicensePath");
    licensePath.setLabelText(Tr::tr("License path:"));
    licensePath.setDisplayStyle(StringAspect::PathChooserDisplay);
    licensePath.setExpectedKind(PathChooser::ExistingDirectory);

    registerAspect(&local);
    local.setSettingsKey("Local");
    local.setLabel(Tr::tr("Local Server"));
    local.setDefaultValue(true);

    registerAspect(&serverHost);
    serverHost.setSettingsKey("ServerHost");
    serverHost.setLabelText(Tr::tr("Server host:"));
    serverHost.setDisplayStyle(StringAspect::LineEditDisplay);
    serverHost.setDefaultValue("localhost");
    serverHost.setEnabled(false);

    registerAspect(&serverPort);
    serverPort.setSettingsKey("ServerPort");
    serverPort.setLabel(Tr::tr("Server Port"));
    serverPort.setRange(1, 65535);
    serverPort.setDefaultValue(9999);
    serverPort.setEnabled(false);

    registerAspect(&verbose);
    verbose.setSettingsKey("Verbose");
    verbose.setLabel(Tr::tr("Verbose log"));
    verbose.setDefaultValue(false);

    connect(&local, &BoolAspect::volatileValueChanged,
            this, [this] (bool checked) {
        serverHost.setEnabled(!checked);
        serverPort.setEnabled(!checked);
    });
}

SquishSettingsPage::SquishSettingsPage(SquishSettings *settings)
{
    setId("A.Squish.General");
    setDisplayName(Tr::tr("General"));
    setCategory(Constants::SQUISH_SETTINGS_CATEGORY);
    setDisplayCategory("Squish");
    setCategoryIcon(Icon({{":/squish/images/settingscategory_squish.png",
                           Theme::PanelTextColorDark}}, Icon::Tint));

    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        SquishSettings &s = *settings;
        using namespace Layouting;

        Grid grid {
            s.squishPath, br,
            s.licensePath, br,
            Span {2, Row { s.local, s.serverHost, s.serverPort } }, br,
            s.verbose, br,
        };
        Column { Row { grid }, st }.attachTo(widget);
    });
}

class SquishServerSettings : public AspectContainer
{
public:
    SquishServerSettings();

    void setFromXmlOutput(const QByteArray &output);

    QMap<QString, QString> mappedAuts; // name, path
    QMap<QString, QString> attachableAuts; // name, host:port
    QStringList autPaths; // absolute path
    IntegerAspect autTimeout;
    IntegerAspect responseTimeout;
    IntegerAspect postMortemWaitTime;
    BoolAspect animatedCursor;
};

SquishServerSettings::SquishServerSettings()
{
    setAutoApply(false);

    registerAspect(&autTimeout);
    autTimeout.setLabel(Tr::tr("Maximum startup time:"));
    autTimeout.setToolTip(Tr::tr("Specifies how many seconds Squish should wait for a reply from the "
                             "AUT directly after starting it."));
    autTimeout.setRange(1, 65535);
    autTimeout.setSuffix("s");
    autTimeout.setDefaultValue(20);

    registerAspect(&responseTimeout);
    responseTimeout.setLabel(Tr::tr("Maximum response time:"));
    responseTimeout.setToolTip(Tr::tr("Specifies how many seconds Squish should wait for a reply from "
                                  "the hooked up AUT before raising a timeout error."));
    responseTimeout.setRange(1, 65535);
    responseTimeout.setDefaultValue(300);
    responseTimeout.setSuffix("s");

    registerAspect(&postMortemWaitTime);
    postMortemWaitTime.setLabel(Tr::tr("Maximum post-mortem wait time:"));
    postMortemWaitTime.setToolTip(Tr::tr("Specifies how many seconds Squish should wait after the the "
                                     "first AUT process has exited."));
    postMortemWaitTime.setRange(1, 65535);
    postMortemWaitTime.setDefaultValue(1500);
    postMortemWaitTime.setSuffix("ms");

    registerAspect(&animatedCursor);
    animatedCursor.setLabel(Tr::tr("Animate mouse cursor:"));
    animatedCursor.setDefaultValue(true);
}

enum InfoMode {None, Applications, AutPaths, AttachableAuts, AutTimeout, AutPMTimeout,
      AutResponseTimeout, AnimatedCursor};

InfoMode infoModeFromType(const QString &type)
{
    if (type == "applications")
        return Applications;
    if (type == "autPaths")
        return AutPaths;
    if (type == "attachableApplications")
        return AttachableAuts;
    if (type == "AUTTimeout")
        return AutTimeout;
    if (type == "AUTPMTimeout")
        return AutPMTimeout;
    if (type == "responseTimeout")
        return AutResponseTimeout;
    if (type == "cursorAnimation")
        return AnimatedCursor;
    return None;
}

void SquishServerSettings::setFromXmlOutput(const QByteArray &output)
{
    SquishServerSettings newSettings;
    InfoMode infoMode = None;
    QXmlStreamReader reader(output);
    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType type = reader.readNext();
        if (type == QXmlStreamReader::Invalid) {
            // MessageBox?
            return;
        } else if (type == QXmlStreamReader::StartElement) {
            const QString tagName = reader.name().toString();
            if (tagName == "info") {
                const QString typeString = reader.attributes().value("type").toString();
                QTC_ASSERT(!typeString.isEmpty(), infoMode = None; continue);
                infoMode = infoModeFromType(typeString);
            } else if (tagName == "item") {
                const QXmlStreamAttributes attributes = reader.attributes();
                switch (infoMode) {
                case Applications:
                    if (attributes.value("mappedOrViaAUTPaths").toString() == "path")
                        continue; // ignore applications provided by autPaths
                    newSettings.mappedAuts.insert(attributes.value("executableName").toString(),
                                                  attributes.value("executablePath").toString());
                    break;
                case AutPaths:
                    newSettings.autPaths.append(attributes.value("value").toString());
                    break;
                case AttachableAuts:
                    newSettings.attachableAuts.insert(attributes.value("name").toString(),
                                                      attributes.value("hostAndPort").toString());
                    break;
                case AutTimeout:
                    newSettings.autTimeout.setValue(attributes.value("value").toInt());
                    break;
                case AutPMTimeout:
                    newSettings.postMortemWaitTime.setValue(attributes.value("value").toInt());
                    break;
                case AutResponseTimeout:
                    newSettings.responseTimeout.setValue(attributes.value("value").toInt());
                    break;
                case AnimatedCursor:
                    newSettings.animatedCursor.setValue(attributes.value("value").toString() == "on");
                    break;
                default:
                    break;
                }
            }
        }
    }
    // if we end here, we update the settings with the read settings
    mappedAuts = newSettings.mappedAuts;
    autPaths = newSettings.autPaths;
    attachableAuts = newSettings.attachableAuts;
    autTimeout.setValue(newSettings.autTimeout.value());
    postMortemWaitTime.setValue(newSettings.postMortemWaitTime.value());
    responseTimeout.setValue(newSettings.responseTimeout.value());
    animatedCursor.setValue(newSettings.animatedCursor.value());
}

class SquishServerItem : public TreeItem
{
public:
    explicit SquishServerItem(const QString &col1 = {}, const QString &col2 = {});
    QVariant data(int column, int role) const override;
private:
    QString m_first;
    QString m_second;
};

SquishServerItem::SquishServerItem(const QString &col1, const QString &col2)
    : m_first(col1)
    , m_second(col2)
{
}

QVariant SquishServerItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (column) {
        case 0: return m_first;
        case 1: return m_second;
        default: return QVariant();
        }
    }
    return QVariant();
}

class SquishServerSettingsWidget : public QWidget
{
public:
    explicit SquishServerSettingsWidget(QWidget *parent = nullptr);

private:
    void repopulateApplicationView();

    SquishServerSettings m_serverSettings;
    BaseTreeView m_applicationsView;
};

SquishServerSettingsWidget::SquishServerSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_applicationsView.setHeaderHidden(true);
    m_applicationsView.setAttribute(Qt::WA_MacShowFocusRect, false);
    m_applicationsView.setFrameStyle(QFrame::NoFrame);
    m_applicationsView.setRootIsDecorated(true);
    m_applicationsView.setSelectionMode(QAbstractItemView::SingleSelection);
    m_applicationsView.header()->setStretchLastSection(false);
    m_applicationsView.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_applicationsView.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_applicationsView.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    using namespace Layouting;
    Form grid {
        &m_applicationsView, br,
        &m_serverSettings.autTimeout,
        &m_serverSettings.responseTimeout,
        &m_serverSettings.postMortemWaitTime,
        &m_serverSettings.animatedCursor,
    };
    // TODO buttons for add, edit, remove
    Column { Row { grid }, st }.attachTo(this);

    repopulateApplicationView(); // initial

    auto progress = new ProgressIndicator(ProgressIndicatorSize::Large, this);
    progress->attachToWidget(this);
    setEnabled(false);
    progress->show();

    // query settings
    SquishTools *squishTools = SquishTools::instance();
    connect(squishTools, &SquishTools::queryFinished, this,
            [this, progress] (const QByteArray &out) {
        m_serverSettings.setFromXmlOutput(out);
        repopulateApplicationView();
        progress->hide();
        setEnabled(true);
    });
    squishTools->queryServerSettings();
}

void SquishServerSettingsWidget::repopulateApplicationView()
{
    TreeModel<SquishServerItem> *model = new TreeModel<SquishServerItem>;
    model->setHeader({QString(), QString()}); // enforce 2 columns

    SquishServerItem *mapped = new SquishServerItem(Tr::tr("Mapped AUTs"));
    model->rootItem()->appendChild(mapped);
    for (auto it = m_serverSettings.mappedAuts.begin(),
         end = m_serverSettings.mappedAuts.end(); it != end; ++it) {
        mapped->appendChild(new SquishServerItem(it.key(), it.value()));
    }

    SquishServerItem *autPaths = new SquishServerItem(Tr::tr("AUT Paths"));
    model->rootItem()->appendChild(autPaths);
    for (const QString &path : qAsConst(m_serverSettings.autPaths))
        autPaths->appendChild(new SquishServerItem(path, ""));

    SquishServerItem *attachable = new SquishServerItem(Tr::tr("Attachable AUTs"));
    model->rootItem()->appendChild(attachable);
    for (auto it = m_serverSettings.attachableAuts.begin(),
         end = m_serverSettings.attachableAuts.end(); it != end; ++it) {
        attachable->appendChild(new SquishServerItem(it.key(), it.value()));
    }

    auto oldModel = m_applicationsView.model();
    m_applicationsView.setModel(model);
    delete oldModel;
}

SquishServerSettingsDialog::SquishServerSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("Squish Server Settings"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(new SquishServerSettingsWidget);
    auto buttonBox = new QDialogButtonBox(/*QDialogButtonBox::Apply|*/QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
//    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
//            this, &SquishServerSettingsDialog::onApply);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, &QDialog::reject);
}

void SquishServerSettingsDialog::onApply()
{
    // TODO write settings to server
    accept();
}

} // namespace Internal
} // namespace Squish
