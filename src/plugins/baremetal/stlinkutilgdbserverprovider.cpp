/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "baremetalconstants.h"

#include "gdbserverprovidermanager.h"
#include "stlinkutilgdbserverprovider.h"

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <coreplugin/variablechooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>

using namespace Utils;

namespace BareMetal {
namespace Internal {

const char executableFileKeyC[] = "BareMetal.StLinkUtilGdbServerProvider.ExecutableFile";
const char verboseLevelKeyC[] = "BareMetal.StLinkUtilGdbServerProvider.VerboseLevel";
const char extendedModeKeyC[] = "BareMetal.StLinkUtilGdbServerProvider.ExtendedMode";
const char resetBoardKeyC[] = "BareMetal.StLinkUtilGdbServerProvider.ResetBoard";
const char transportLayerKeyC[] = "BareMetal.StLinkUtilGdbServerProvider.TransportLayer";

// StLinkUtilGdbServerProvider

StLinkUtilGdbServerProvider::StLinkUtilGdbServerProvider()
    : GdbServerProvider(QLatin1String(Constants::STLINK_UTIL_PROVIDER_ID))
{
    setInitCommands(defaultInitCommands());
    setResetCommands(defaultResetCommands());
    setDefaultChannel("localhost", 4242);
    setSettingsKeyBase("BareMetal.StLinkUtilGdbServerProvider");
    setTypeDisplayName(StLinkUtilGdbServerProviderFactory::tr("ST-LINK Utility"));
}

StLinkUtilGdbServerProvider::StLinkUtilGdbServerProvider(
        const StLinkUtilGdbServerProvider &other)
    : GdbServerProvider(other)
    , m_executableFile(other.m_executableFile)
    , m_verboseLevel(0)
    , m_extendedMode(false)
    , m_resetBoard(true)
    , m_transport(RawUsb)
{
}

QString StLinkUtilGdbServerProvider::defaultInitCommands()
{
    return QLatin1String("load\n");
}

QString StLinkUtilGdbServerProvider::defaultResetCommands()
{
    return {};
}

QString StLinkUtilGdbServerProvider::channelString() const
{
    switch (startupMode()) {
    case NoStartup:
        // fallback
    case StartupOnNetwork:
        // Just return as "host:port" form.
        return GdbServerProvider::channelString();
    case StartupOnPipe:
        // Unsupported mode
        return {};
    default: // wrong
        return {};
    }
}

CommandLine StLinkUtilGdbServerProvider::command() const
{
    CommandLine cmd{m_executableFile, {}};

    if (m_extendedMode)
        cmd.addArg("--multi");

    if (!m_resetBoard)
        cmd.addArg("--no-reset");

    cmd.addArg("--stlink_version=" + QString::number(m_transport));
    cmd.addArg("--listen_port=" + QString::number(channel().port()));
    cmd.addArg("--verbose=" + QString::number(m_verboseLevel));

    return cmd;
}

bool StLinkUtilGdbServerProvider::canStartupMode(StartupMode m) const
{
    return m == NoStartup || m == StartupOnNetwork;
}

bool StLinkUtilGdbServerProvider::isValid() const
{
    if (!GdbServerProvider::isValid())
        return false;

    const StartupMode m = startupMode();

    if (m == NoStartup || m == StartupOnNetwork) {
        if (channel().host().isEmpty())
            return false;
    }

    if (m == StartupOnNetwork) {
        if (m_executableFile.isEmpty())
            return false;
    }

    return true;
}

GdbServerProvider *StLinkUtilGdbServerProvider::clone() const
{
    return new StLinkUtilGdbServerProvider(*this);
}

QVariantMap StLinkUtilGdbServerProvider::toMap() const
{
    QVariantMap data = GdbServerProvider::toMap();
    data.insert(QLatin1String(executableFileKeyC), m_executableFile.toVariant());
    data.insert(QLatin1String(verboseLevelKeyC), m_verboseLevel);
    data.insert(QLatin1String(extendedModeKeyC), m_extendedMode);
    data.insert(QLatin1String(resetBoardKeyC), m_resetBoard);
    data.insert(QLatin1String(transportLayerKeyC), m_transport);
    return data;
}

bool StLinkUtilGdbServerProvider::fromMap(const QVariantMap &data)
{
    if (!GdbServerProvider::fromMap(data))
        return false;

    m_executableFile = FilePath::fromVariant(data.value(QLatin1String(executableFileKeyC)));
    m_verboseLevel = data.value(QLatin1String(verboseLevelKeyC)).toInt();
    m_extendedMode = data.value(QLatin1String(extendedModeKeyC)).toBool();
    m_resetBoard = data.value(QLatin1String(resetBoardKeyC)).toBool();
    m_transport = static_cast<TransportLayer>(
                data.value(QLatin1String(transportLayerKeyC)).toInt());
    return true;
}

bool StLinkUtilGdbServerProvider::operator==(const GdbServerProvider &other) const
{
    if (!GdbServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const StLinkUtilGdbServerProvider *>(&other);
    return m_executableFile == p->m_executableFile
            && m_verboseLevel == p->m_verboseLevel
            && m_extendedMode == p->m_extendedMode
            && m_resetBoard == p->m_resetBoard
            && m_transport == p->m_transport;
}

GdbServerProviderConfigWidget *StLinkUtilGdbServerProvider::configurationWidget()
{
    return new StLinkUtilGdbServerProviderConfigWidget(this);
}

// StLinkUtilGdbServerProviderFactory

StLinkUtilGdbServerProviderFactory::StLinkUtilGdbServerProviderFactory()
{
    setId(QLatin1String(Constants::STLINK_UTIL_PROVIDER_ID));
    setDisplayName(tr("ST-LINK Utility"));
}

GdbServerProvider *StLinkUtilGdbServerProviderFactory::create()
{
    return new StLinkUtilGdbServerProvider;
}

bool StLinkUtilGdbServerProviderFactory::canRestore(const QVariantMap &data) const
{
    const QString id = idFromMap(data);
    return id.startsWith(QLatin1String(Constants::STLINK_UTIL_PROVIDER_ID)
                         + QLatin1Char(':'));
}

GdbServerProvider *StLinkUtilGdbServerProviderFactory::restore(const QVariantMap &data)
{
    const auto p = new StLinkUtilGdbServerProvider;
    const auto updated = data;
    if (p->fromMap(updated))
        return p;
    delete p;
    return nullptr;
}

// StLinkUtilGdbServerProviderConfigWidget

StLinkUtilGdbServerProviderConfigWidget::StLinkUtilGdbServerProviderConfigWidget(
        StLinkUtilGdbServerProvider *p)
    : GdbServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_hostWidget = new HostWidget(this);
    m_mainLayout->addRow(tr("Host:"), m_hostWidget);

    m_executableFileChooser = new Utils::PathChooser;
    m_executableFileChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_mainLayout->addRow(tr("Executable file:"), m_executableFileChooser);

    m_verboseLevelSpinBox = new QSpinBox;
    m_verboseLevelSpinBox->setRange(0, 99);
    m_verboseLevelSpinBox->setToolTip(tr("Specify the verbosity level (0..99)."));
    m_mainLayout->addRow(tr("Verbosity level:"), m_verboseLevelSpinBox);

    m_extendedModeCheckBox = new QCheckBox;
    m_extendedModeCheckBox->setToolTip(tr("Continue listening for connections "
                                          "after disconnect."));
    m_mainLayout->addRow(tr("Extended mode:"), m_extendedModeCheckBox);

    m_resetBoardCheckBox = new QCheckBox;
    m_resetBoardCheckBox->setToolTip(tr("Reset board on connection."));
    m_mainLayout->addRow(tr("Reset on connection:"), m_resetBoardCheckBox);

    m_transportLayerComboBox = new QComboBox;
    m_transportLayerComboBox->setToolTip(tr("Transport layer type."));
    m_mainLayout->addRow(tr("Version:"), m_transportLayerComboBox);

    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(tr("Reset commands:"), m_resetCommandsTextEdit);

    populateTransportLayers();
    addErrorLabel();
    setFromProvider();

    const auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_initCommandsTextEdit);
    chooser->addSupportedWidget(m_resetCommandsTextEdit);

    connect(m_hostWidget, &HostWidget::dataChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_executableFileChooser, &Utils::PathChooser::rawPathChanged,
            this, &GdbServerProviderConfigWidget::dirty);

    connect(m_verboseLevelSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_extendedModeCheckBox, &QAbstractButton::clicked,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetBoardCheckBox, &QAbstractButton::clicked,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_transportLayerComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GdbServerProviderConfigWidget::dirty);

    connect(m_initCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);

    connect(m_startupModeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StLinkUtilGdbServerProviderConfigWidget::startupModeChanged);
}

void StLinkUtilGdbServerProviderConfigWidget::startupModeChanged()
{
    const GdbServerProvider::StartupMode m = startupMode();
    const bool isStartup = m != GdbServerProvider::NoStartup;
    m_executableFileChooser->setVisible(isStartup);
    m_mainLayout->labelForField(m_executableFileChooser)->setVisible(isStartup);
    m_verboseLevelSpinBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_verboseLevelSpinBox)->setVisible(isStartup);
    m_extendedModeCheckBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_extendedModeCheckBox)->setVisible(isStartup);
    m_resetBoardCheckBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_resetBoardCheckBox)->setVisible(isStartup);
    m_transportLayerComboBox->setVisible(isStartup);
    m_mainLayout->labelForField(m_transportLayerComboBox)->setVisible(isStartup);
}

void StLinkUtilGdbServerProviderConfigWidget::applyImpl()
{
    const auto p = static_cast<StLinkUtilGdbServerProvider *>(provider());
    Q_ASSERT(p);

    p->setChannel(m_hostWidget->channel());
    p->m_executableFile = m_executableFileChooser->fileName();
    p->m_verboseLevel = m_verboseLevelSpinBox->value();
    p->m_extendedMode = m_extendedModeCheckBox->isChecked();
    p->m_resetBoard = m_resetBoardCheckBox->isChecked();
    p->m_transport = transportLayer();
    p->setInitCommands(m_initCommandsTextEdit->toPlainText());
    p->setResetCommands(m_resetCommandsTextEdit->toPlainText());
}

void StLinkUtilGdbServerProviderConfigWidget::discardImpl()
{
    setFromProvider();
}

StLinkUtilGdbServerProvider::TransportLayer
StLinkUtilGdbServerProviderConfigWidget::transportLayerFromIndex(int idx) const
{
    return static_cast<StLinkUtilGdbServerProvider::TransportLayer>(
                m_transportLayerComboBox->itemData(idx).toInt());
}

StLinkUtilGdbServerProvider::TransportLayer
StLinkUtilGdbServerProviderConfigWidget::transportLayer() const
{
    const int idx = m_transportLayerComboBox->currentIndex();
    return transportLayerFromIndex(idx);
}

void StLinkUtilGdbServerProviderConfigWidget::setTransportLayer(
        StLinkUtilGdbServerProvider::TransportLayer tl)
{
    for (int idx = 0; idx < m_transportLayerComboBox->count(); ++idx) {
        if (tl == transportLayerFromIndex(idx)) {
            m_transportLayerComboBox->setCurrentIndex(idx);
            break;
        }
    }
}

void StLinkUtilGdbServerProviderConfigWidget::populateTransportLayers()
{
    m_transportLayerComboBox->insertItem(
                m_transportLayerComboBox->count(), tr("ST-LINK/V1"),
                StLinkUtilGdbServerProvider::ScsiOverUsb);
    m_transportLayerComboBox->insertItem(
                m_transportLayerComboBox->count(), tr("ST-LINK/V2"),
                StLinkUtilGdbServerProvider::RawUsb);
}

void StLinkUtilGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<StLinkUtilGdbServerProvider *>(provider());
    Q_ASSERT(p);

    const QSignalBlocker blocker(this);
    startupModeChanged();
    m_hostWidget->setChannel(p->channel());
    m_executableFileChooser->setFileName(p->m_executableFile);
    m_verboseLevelSpinBox->setValue(p->m_verboseLevel);
    m_extendedModeCheckBox->setChecked(p->m_extendedMode);
    m_resetBoardCheckBox->setChecked(p->m_resetBoard);
    setTransportLayer(p->m_transport);
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
}

} // namespace Internal
} // namespace ProjectExplorer
