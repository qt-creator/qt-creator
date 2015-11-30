/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef STLINKUTILGDBSERVERPROVIDER_H
#define STLINKUTILGDBSERVERPROVIDER_H

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
    QString typeDisplayName() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    bool operator==(const GdbServerProvider &) const;

    GdbServerProviderConfigWidget *configurationWidget();
    GdbServerProvider *clone() const;

    QString channel() const;
    QString executable() const;
    QStringList arguments() const;

    bool canStartupMode(StartupMode mode) const;
    bool isValid() const;

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

    GdbServerProvider *create();

    bool canRestore(const QVariantMap &data);
    GdbServerProvider *restore(const QVariantMap &data);

    GdbServerProviderConfigWidget *configurationWidget(GdbServerProvider *);
};

class StLinkUtilGdbServerProviderConfigWidget : public GdbServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit StLinkUtilGdbServerProviderConfigWidget(StLinkUtilGdbServerProvider *);

private slots:
    void startupModeChanged();

private:
    void applyImpl();
    void discardImpl();

    StLinkUtilGdbServerProvider::TransportLayer transportLayerFromIndex(int idx) const;
    StLinkUtilGdbServerProvider::TransportLayer transportLayer() const;
    void setTransportLayer(StLinkUtilGdbServerProvider::TransportLayer);

    void populateTransportLayers();
    void setFromProvider();

    QPointer<HostWidget> m_hostWidget;
    QPointer<Utils::PathChooser> m_executableFileChooser;
    QPointer<QSpinBox> m_verboseLevelSpinBox;
    QPointer<QCheckBox> m_extendedModeCheckBox;
    QPointer<QCheckBox> m_resetBoardCheckBox;
    QPointer<QComboBox> m_transportLayerComboBox;
    QPointer<QPlainTextEdit> m_initCommandsTextEdit;
    QPointer<QPlainTextEdit> m_resetCommandsTextEdit;
};

} // namespace Internal
} // namespace BareMetal

#endif // STLINKUTILGDBSERVERPROVIDER_H
