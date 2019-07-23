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

#include <QObject>
#include <QSet>
#include <QVariantMap>
#include <QWidget>

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

class BareMetalDevice;
class GdbServerProviderConfigWidget;
class GdbServerProviderManager;

// GdbServerProvider

class GdbServerProvider
{
public:
    enum StartupMode {
        NoStartup = 0,
        StartupOnNetwork,
        StartupOnPipe,
        StartupModesCount
    };

    virtual ~GdbServerProvider();

    QString displayName() const;
    void setDisplayName(const QString &name);

    QString id() const;

    StartupMode startupMode() const;
    QString initCommands() const;
    QString resetCommands() const;
    bool useExtendedRemote() const;
    QString typeDisplayName() const;

    virtual bool operator==(const GdbServerProvider &) const;

    virtual GdbServerProviderConfigWidget *configurationWidget() = 0;

    virtual QString channelString() const;
    virtual GdbServerProvider *clone() const = 0;

    virtual QVariantMap toMap() const;

    virtual Utils::CommandLine command() const;

    virtual bool isValid() const;
    virtual bool canStartupMode(StartupMode) const;

    void registerDevice(BareMetalDevice *);
    void unregisterDevice(BareMetalDevice *);

    QUrl channel() const;
    void setChannel(const QUrl &channelString);
    void setDefaultChannel(const QString &host, int port);

protected:
    explicit GdbServerProvider(const QString &id);
    explicit GdbServerProvider(const GdbServerProvider &);

    void setStartupMode(StartupMode);
    void setInitCommands(const QString &);
    void setResetCommands(const QString &);
    void setUseExtendedRemote(bool);
    void setSettingsKeyBase(const QString &settingsBase);
    void setTypeDisplayName(const QString &typeDisplayName);

    void providerUpdated();

    virtual bool fromMap(const QVariantMap &data);

private:
    QString m_id;
    QString m_settingsBase;
    mutable QString m_displayName;
    QString m_typeDisplayName;
    QUrl m_channel;
    StartupMode m_startupMode = NoStartup;
    QString m_initCommands;
    QString m_resetCommands;
    QSet<BareMetalDevice *> m_devices;
    bool m_useExtendedRemote = false;

    friend class GdbServerProviderConfigWidget;
};

// GdbServerProviderFactory

class GdbServerProviderFactory : public QObject
{
    Q_OBJECT

public:
    QString id() const;
    QString displayName() const;

    virtual GdbServerProvider *create() = 0;

    virtual bool canRestore(const QVariantMap &data) const = 0;
    virtual GdbServerProvider *restore(const QVariantMap &data) = 0;

    static QString idFromMap(const QVariantMap &data);
    static void idToMap(QVariantMap &data, const QString &id);

protected:
    void setId(const QString &id);
    void setDisplayName(const QString &name);

private:
    QString m_displayName;
    QString m_id;
};

// GdbServerProviderConfigWidget

class GdbServerProviderConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GdbServerProviderConfigWidget(GdbServerProvider *);
    GdbServerProvider *provider() const;
    void apply();
    void discard();

signals:
    void dirty();

protected:
    virtual void applyImpl() = 0;
    virtual void discardImpl() = 0;

    void setErrorMessage(const QString &);
    void clearErrorMessage();
    void addErrorLabel();

    GdbServerProvider::StartupMode startupModeFromIndex(int idx) const;
    GdbServerProvider::StartupMode startupMode() const;
    void setStartupMode(GdbServerProvider::StartupMode mode);
    void populateStartupModes();

    static QString defaultInitCommandsTooltip();
    static QString defaultResetCommandsTooltip();

    QFormLayout *m_mainLayout = nullptr;
    QLineEdit *m_nameLineEdit = nullptr;
    QComboBox *m_startupModeComboBox = nullptr;

private:
    void setFromProvider();

    GdbServerProvider *m_provider = nullptr;
    QLabel *m_errorLabel = nullptr;
};

// HostWidget

class HostWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit HostWidget(QWidget *parent = nullptr);

    void setChannel(const QUrl &host);
    QUrl channel() const;

signals:
    void dataChanged();

private:
    QLineEdit *m_hostLineEdit = nullptr;
    QSpinBox *m_portSpinBox = nullptr;
};

} // namespace Internal
} // namespace BareMetal
