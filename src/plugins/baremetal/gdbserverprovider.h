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
#include <QVariantMap>

#include <QWidget>

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QLabel;
class QComboBox;
class QSpinBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

class GdbServerProviderConfigWidget;
class GdbServerProviderManager;

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

    virtual bool operator==(const GdbServerProvider &) const;

    virtual QString typeDisplayName() const = 0;

    virtual GdbServerProviderConfigWidget *configurationWidget() = 0;

    virtual QString channel() const = 0;
    virtual GdbServerProvider *clone() const = 0;

    virtual QVariantMap toMap() const;

    virtual QString executable() const;
    virtual QStringList arguments() const;

    virtual bool isValid() const;
    virtual bool canStartupMode(StartupMode) const;

protected:
    explicit GdbServerProvider(const QString &id);
    explicit GdbServerProvider(const GdbServerProvider &);

    void setStartupMode(StartupMode);
    void setInitCommands(const QString &);
    void setResetCommands(const QString &);

    void providerUpdated();

    virtual bool fromMap(const QVariantMap &data);

private:
    QString m_id;
    mutable QString m_displayName;
    StartupMode m_startupMode;
    QString m_initCommands;
    QString m_resetCommands;

    friend class GdbServerProviderConfigWidget;
};

class GdbServerProviderFactory : public QObject
{
    Q_OBJECT

public:
    QString id() const;
    QString displayName() const;

    virtual GdbServerProvider *create() = 0;

    virtual bool canRestore(const QVariantMap &data) = 0;
    virtual GdbServerProvider *restore(const QVariantMap &data) = 0;

    static QString idFromMap(const QVariantMap &data);
    static void idToMap(QVariantMap &data, const QString id);

protected:
    void setId(const QString &id);
    void setDisplayName(const QString &name);

private:
    QString m_displayName;
    QString m_id;
};

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

protected slots:
    void setErrorMessage(const QString &);
    void clearErrorMessage();

protected:
    virtual void applyImpl() = 0;
    virtual void discardImpl() = 0;

    void addErrorLabel();

    GdbServerProvider::StartupMode startupModeFromIndex(int idx) const;
    GdbServerProvider::StartupMode startupMode() const;
    void setStartupMode(GdbServerProvider::StartupMode mode);
    void populateStartupModes();

    static QString defaultInitCommandsTooltip();
    static QString defaultResetCommandsTooltip();

    QFormLayout *m_mainLayout;
    QLineEdit *m_nameLineEdit;
    QComboBox *m_startupModeComboBox;

private:
    void setFromProvider();

    GdbServerProvider *m_provider;
    QLabel *m_errorLabel = 0;
};

class HostWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HostWidget(QWidget *parent = 0);

    void setHost(const QString &host);
    QString host() const;
    void setPort(const quint16 &port);
    quint16 port() const;

signals:
    void dataChanged();

private:
    QLineEdit *m_hostLineEdit;
    QSpinBox *m_portSpinBox;
};

} // namespace Internal
} // namespace BareMetal
