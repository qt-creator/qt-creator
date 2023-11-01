// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerconstants.h>

#include <projectexplorer/abi.h>

#include <utils/filepath.h>
#include <utils/store.h>

#include <QSet>
#include <QUrl>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLabel;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

namespace Debugger {
class DebuggerRunTool;
}

namespace ProjectExplorer {
class RunControl;
class RunWorker;
}

namespace BareMetal {
namespace Internal {

class BareMetalDevice;
class IDebugServerProviderConfigWidget;

// IDebugServerProvider

class IDebugServerProvider
{
protected:
    explicit IDebugServerProvider(const QString &id);
    IDebugServerProvider(const IDebugServerProvider &provider) = delete;
    IDebugServerProvider &operator=(const IDebugServerProvider &provider) = delete;

public:
    virtual ~IDebugServerProvider();

    QString displayName() const;
    void setDisplayName(const QString &name);

    QUrl channel() const;
    void setChannel(const QUrl &channel);
    void setChannel(const QString &host, int port);

    virtual QString channelString() const;

    QString id() const;
    QString typeDisplayName() const;
    Debugger::DebuggerEngineType engineType() const;

    virtual bool operator==(const IDebugServerProvider &other) const;

    IDebugServerProviderConfigWidget *configurationWidget() const;
    void setConfigurationWidgetCreator
        (const std::function<IDebugServerProviderConfigWidget *()> &configurationWidgetCreator);

    virtual void toMap(Utils::Store &data) const;
    virtual void fromMap(const Utils::Store &data);

    virtual bool aboutToRun(Debugger::DebuggerRunTool *runTool,
                            QString &errorMessage) const = 0;
    virtual ProjectExplorer::RunWorker *targetRunner(
            ProjectExplorer::RunControl *runControl) const = 0;

    virtual bool isValid() const = 0;
    virtual bool isSimulator() const { return false; }

    void registerDevice(BareMetalDevice *device);
    void unregisterDevice(BareMetalDevice *device);

protected:
    void setTypeDisplayName(const QString &typeDisplayName);
    void setEngineType(Debugger::DebuggerEngineType engineType);

    void providerUpdated();
    void resetId();

    QString m_id;
    mutable QString m_displayName;
    QString m_typeDisplayName;
    QUrl m_channel;
    Debugger::DebuggerEngineType m_engineType = Debugger::NoEngineType;
    QSet<BareMetalDevice *> m_devices;
    std::function<IDebugServerProviderConfigWidget *()> m_configurationWidgetCreator;

    friend class DebugServerProvidersSettingsWidget;
    friend class IDebugServerProviderConfigWidget;
    friend class IDebugServerProviderFactory;
};

// IDebugServerProviderFactory

class IDebugServerProviderFactory
{
public:
    QString id() const;
    QString displayName() const;

    IDebugServerProvider *create() const;
    IDebugServerProvider *restore(const Utils::Store &data) const;

    bool canRestore(const Utils::Store &data) const;

    static QString idFromMap(const Utils::Store &data);
    static void idToMap(Utils::Store &data, const QString &id);

protected:
    IDebugServerProviderFactory();
    void setId(const QString &id);
    void setDisplayName(const QString &name);
    void setCreator(const std::function<IDebugServerProvider *()> &creator);

private:
    IDebugServerProviderFactory(const IDebugServerProviderFactory &) = delete;
    IDebugServerProviderFactory &operator=(const IDebugServerProviderFactory &) = delete;

    QString m_displayName;
    QString m_id;
    std::function<IDebugServerProvider *()> m_creator;
};

// IDebugServerProviderConfigWidget

class IDebugServerProviderConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IDebugServerProviderConfigWidget(IDebugServerProvider *provider);

    virtual void apply();
    virtual void discard();

signals:
    void dirty();

protected:
    void setErrorMessage(const QString &);
    void clearErrorMessage();
    void addErrorLabel();
    void setFromProvider();

    IDebugServerProvider *m_provider = nullptr;
    QFormLayout *m_mainLayout = nullptr;
    QLineEdit *m_nameLineEdit = nullptr;
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

protected:
    QLineEdit *m_hostLineEdit = nullptr;
    QSpinBox *m_portSpinBox = nullptr;
};

} // namespace Internal
} // namespace BareMetal
