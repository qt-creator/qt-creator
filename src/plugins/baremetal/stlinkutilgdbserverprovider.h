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

#pragma once

#include "gdbserverprovider.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal {
namespace Internal {

class StLinkUtilGdbServerProviderConfigWidget;
class StLinkUtilGdbServerProviderFactory;

class StLinkUtilGdbServerProvider : public GdbServerProvider
{
public:
    enum TransportLayer { ScsiOverUsb = 1, RawUsb = 2 };
    QString typeDisplayName() const final;

    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const GdbServerProvider &) const final;

    GdbServerProviderConfigWidget *configurationWidget() final;
    GdbServerProvider *clone() const final;

    QString channel() const final;
    QString executable() const final;
    QStringList arguments() const final;

    bool canStartupMode(StartupMode mode) const final;
    bool isValid() const final;

private:
    explicit StLinkUtilGdbServerProvider();
    explicit StLinkUtilGdbServerProvider(const StLinkUtilGdbServerProvider &);

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    QString m_host;
    quint16 m_port;
    QString m_executableFile;
    int m_verboseLevel; // 0..99
    bool m_extendedMode; // Listening for connections after disconnect
    bool m_resetBoard;
    TransportLayer m_transport;

    friend class StLinkUtilGdbServerProviderConfigWidget;
    friend class StLinkUtilGdbServerProviderFactory;
};

class StLinkUtilGdbServerProviderFactory : public GdbServerProviderFactory
{
    Q_OBJECT

public:
    explicit StLinkUtilGdbServerProviderFactory();

    GdbServerProvider *create() final;

    bool canRestore(const QVariantMap &data) const final;
    GdbServerProvider *restore(const QVariantMap &data) final;

    GdbServerProviderConfigWidget *configurationWidget(GdbServerProvider *);
};

class StLinkUtilGdbServerProviderConfigWidget : public GdbServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit StLinkUtilGdbServerProviderConfigWidget(StLinkUtilGdbServerProvider *);

private:
    void startupModeChanged();

    void applyImpl() final;
    void discardImpl() final;

    StLinkUtilGdbServerProvider::TransportLayer transportLayerFromIndex(int idx) const;
    StLinkUtilGdbServerProvider::TransportLayer transportLayer() const;
    void setTransportLayer(StLinkUtilGdbServerProvider::TransportLayer);

    void populateTransportLayers();
    void setFromProvider();

    HostWidget *m_hostWidget;
    Utils::PathChooser *m_executableFileChooser;
    QSpinBox *m_verboseLevelSpinBox;
    QCheckBox *m_extendedModeCheckBox;
    QCheckBox *m_resetBoardCheckBox;
    QComboBox *m_transportLayerComboBox;
    QPlainTextEdit *m_initCommandsTextEdit;
    QPlainTextEdit *m_resetCommandsTextEdit;
};

} // namespace Internal
} // namespace BareMetal
