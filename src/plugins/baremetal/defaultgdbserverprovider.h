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

namespace BareMetal {
namespace Internal {

class DefaultGdbServerProviderConfigWidget;
class DefaultGdbServerProviderFactory;

class DefaultGdbServerProvider : public GdbServerProvider
{
public:
    QString typeDisplayName() const final;

    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const GdbServerProvider &) const final;

    GdbServerProviderConfigWidget *configurationWidget() final;
    GdbServerProvider *clone() const final;

    QString channel() const final;

    bool isValid() const final;

    QString host() const;
    void setHost(const QString &host);

    quint16 port() const;
    void setPort(const quint16 &port);

private:
    explicit DefaultGdbServerProvider();
    explicit DefaultGdbServerProvider(const DefaultGdbServerProvider &);

    QString m_host;
    quint16 m_port;

    friend class DefaultGdbServerProviderConfigWidget;
    friend class DefaultGdbServerProviderFactory;
    friend class BareMetalDevice;
};

class DefaultGdbServerProviderFactory : public GdbServerProviderFactory
{
    Q_OBJECT

public:
    explicit DefaultGdbServerProviderFactory();

    GdbServerProvider *create() final;

    bool canRestore(const QVariantMap &data) const final;
    GdbServerProvider *restore(const QVariantMap &data) final;

    GdbServerProviderConfigWidget *configurationWidget(GdbServerProvider *);
};

class DefaultGdbServerProviderConfigWidget : public GdbServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit DefaultGdbServerProviderConfigWidget(DefaultGdbServerProvider *);

private:
    void applyImpl() final;
    void discardImpl() final;

    void setFromProvider();

    HostWidget *m_hostWidget;
    QPlainTextEdit *m_initCommandsTextEdit;
    QPlainTextEdit *m_resetCommandsTextEdit;
};

} // namespace Internal
} // namespace BareMetal
