/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <coreplugin/id.h>
#include <debugger/debuggerconstants.h>
#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

class BareMetalDevice;
class IDebugServerProviderConfigWidget;

// IDebugServerProvider

class IDebugServerProvider
{
public:
    virtual ~IDebugServerProvider();

    QString displayName() const;
    void setDisplayName(const QString &name);

    QString id() const;

    QString typeDisplayName() const;

    virtual bool operator==(const IDebugServerProvider &other) const;

    virtual IDebugServerProviderConfigWidget *configurationWidget() = 0;

    virtual IDebugServerProvider *clone() const = 0;

    virtual QVariantMap toMap() const;

    virtual bool isValid() const = 0;
    virtual bool hasProcess() const { return false; }
    virtual Debugger::DebuggerEngineType engineType() const = 0;

    void registerDevice(BareMetalDevice *device);
    void unregisterDevice(BareMetalDevice *device);

protected:
    explicit IDebugServerProvider(const QString &id);
    explicit IDebugServerProvider(const IDebugServerProvider &provider);

    void setTypeDisplayName(const QString &typeDisplayName);

    void providerUpdated();

    virtual bool fromMap(const QVariantMap &data);

    QString m_id;
    mutable QString m_displayName;
    QString m_typeDisplayName;
    QSet<BareMetalDevice *> m_devices;

    friend class IDebugServerProviderConfigWidget;
};

// IDebugServerProviderFactory

class IDebugServerProviderFactory : public QObject
{
    Q_OBJECT

public:
    QString id() const;
    QString displayName() const;

    virtual IDebugServerProvider *create() = 0;

    virtual bool canRestore(const QVariantMap &data) const = 0;
    virtual IDebugServerProvider *restore(const QVariantMap &data) = 0;

    static QString idFromMap(const QVariantMap &data);
    static void idToMap(QVariantMap &data, const QString &id);

protected:
    void setId(const QString &id);
    void setDisplayName(const QString &name);

private:
    QString m_displayName;
    QString m_id;
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

} // namespace Internal
} // namespace BareMetal
