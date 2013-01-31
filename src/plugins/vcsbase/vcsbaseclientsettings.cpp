/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion and Hugues Delorme
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "vcsbaseclientsettings.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>

#include <QSettings>

namespace {

class SettingValue
{
public:
    union Composite
    {
        QString *strPtr; // Union can't store class objects ...
        int intValue;
        bool boolValue;
    };

    SettingValue() :
        m_type(QVariant::Invalid)
    {
    }

    explicit SettingValue(const QVariant &v) :
        m_type(v.type())
    {
        switch (v.type()) {
        case QVariant::UInt:
            m_type = QVariant::Int;
        case QVariant::Int:
            m_comp.intValue = v.toInt();
            break;
        case QVariant::Bool:
            m_comp.boolValue = v.toBool();
            break;
        case QVariant::String:
            m_comp.strPtr = new QString(v.toString());
            break;
        default:
            m_type = QVariant::Invalid;
            break;
        }
    }

    SettingValue(const SettingValue &other) :
        m_comp(other.m_comp),
        m_type(other.type())
    {
        copyInternalString(other);
    }

    ~SettingValue()
    {
        deleteInternalString();
    }

    SettingValue &operator=(const SettingValue &other)
    {
        if (this != &other) {
            deleteInternalString();
            m_type = other.type();
            m_comp = other.m_comp;
            copyInternalString(other);
        }
        return *this;
    }

    QString stringValue(const QString &defaultString = QString()) const
    {
        if (type() == QVariant::String && m_comp.strPtr != 0)
            return *(m_comp.strPtr);
        return defaultString;
    }

    QVariant::Type type() const
    {
        return m_type;
    }

    static bool isUsableVariantType(QVariant::Type varType)
    {
        return varType == QVariant::UInt || varType == QVariant::Int ||
                varType == QVariant::Bool || varType == QVariant::String;
    }

    Composite m_comp;

private:
    void deleteInternalString()
    {
        if (m_type == QVariant::String && m_comp.strPtr != 0) {
            delete m_comp.strPtr;
            m_comp.strPtr = 0;
        }
    }

    void copyInternalString(const SettingValue &other)
    {
        if (type() == QVariant::String) {
            const QString *otherString = other.m_comp.strPtr;
            m_comp.strPtr = new QString(otherString != 0 ? *otherString : QString());
        }
    }

    QVariant::Type m_type;
};

bool operator==(const SettingValue &lhs, const SettingValue &rhs)
{
    if (lhs.type() == rhs.type()) {
        switch (lhs.type()) {
        case QVariant::Int:
            return lhs.m_comp.intValue == rhs.m_comp.intValue;
        case QVariant::Bool:
            return lhs.m_comp.boolValue == rhs.m_comp.boolValue;
        case QVariant::String:
            return lhs.stringValue() == rhs.stringValue();
        default:
            return false;
        }
    }
    return false;
}

} // Anonymous namespace

namespace VcsBase {

namespace Internal {

class VcsBaseClientSettingsPrivate : public QSharedData
{
public:
    VcsBaseClientSettingsPrivate() {}

    VcsBaseClientSettingsPrivate(const VcsBaseClientSettingsPrivate &other) :
        QSharedData(other),
        m_valueHash(other.m_valueHash),
        m_defaultValueHash(other.m_defaultValueHash),
        m_settingsGroup(other.m_settingsGroup),
        m_binaryFullPath(other.m_binaryFullPath)
    {
    }

    QHash<QString, SettingValue> m_valueHash;
    QVariantHash m_defaultValueHash;
    QString m_settingsGroup;
    mutable QString m_binaryFullPath;
};

} // namespace Internal

/*!
    \class VcsBase::VcsBaseClientSettings

    \brief Settings used in VcsBaseClient.

    \sa VcsBase::VcsBaseClient
*/

const QLatin1String VcsBaseClientSettings::binaryPathKey("BinaryPath");
const QLatin1String VcsBaseClientSettings::userNameKey("Username");
const QLatin1String VcsBaseClientSettings::userEmailKey("UserEmail");
const QLatin1String VcsBaseClientSettings::logCountKey("LogCount");
const QLatin1String VcsBaseClientSettings::promptOnSubmitKey("PromptOnSubmit");
const QLatin1String VcsBaseClientSettings::timeoutKey("Timeout");
const QLatin1String VcsBaseClientSettings::pathKey("Path");

VcsBaseClientSettings::VcsBaseClientSettings() :
    d(new Internal::VcsBaseClientSettingsPrivate)
{
    declareKey(binaryPathKey, QLatin1String(""));
    declareKey(userNameKey, QLatin1String(""));
    declareKey(userEmailKey, QLatin1String(""));
    declareKey(logCountKey, 100);
    declareKey(promptOnSubmitKey, true);
    declareKey(timeoutKey, 30);
    declareKey(pathKey, QString());
}

VcsBaseClientSettings::VcsBaseClientSettings(const VcsBaseClientSettings &other) :
    d(other.d)
{
}

VcsBaseClientSettings &VcsBaseClientSettings::operator=(const VcsBaseClientSettings &other)
{
    if (this != &other)
        d = other.d;
    return *this;
}

VcsBaseClientSettings::~VcsBaseClientSettings()
{
}

void VcsBaseClientSettings::writeSettings(QSettings *settings) const
{
    settings->beginGroup(settingsGroup());
    foreach (const QString &key, keys())
        settings->setValue(key, value(key));
    settings->endGroup();
}

void VcsBaseClientSettings::readSettings(const QSettings *settings)
{
    const QString keyRoot = settingsGroup() + QLatin1Char('/');
    foreach (const QString &key, keys()) {
        const QVariant value = settings->value(keyRoot + key, keyDefaultValue(key));
        // For some reason QSettings always return QVariant(QString) when the
        // key exists. The type is explicited to avoid wrong conversions
        switch (valueType(key)) {
        case QVariant::Int:
            setValue(key, value.toInt());
            break;
        case QVariant::Bool:
            setValue(key, value.toBool());
            break;
        case QVariant::String:
            setValue(key, value.toString());
            break;
        default:
            break;
        }
    }
}

bool VcsBaseClientSettings::equals(const VcsBaseClientSettings &rhs) const
{
    if (this == &rhs)
        return true;
    return d->m_valueHash == rhs.d->m_valueHash;
}

QStringList VcsBaseClientSettings::keys() const
{
    return d->m_valueHash.keys();
}

bool VcsBaseClientSettings::hasKey(const QString &key) const
{
    return d->m_valueHash.contains(key);
}

int *VcsBaseClientSettings::intPointer(const QString &key)
{
    if (hasKey(key))
        return &(d->m_valueHash[key].m_comp.intValue);
    return 0;
}

bool *VcsBaseClientSettings::boolPointer(const QString &key)
{
    if (hasKey(key))
        return &(d->m_valueHash[key].m_comp.boolValue);
    return 0;
}

QString *VcsBaseClientSettings::stringPointer(const QString &key)
{
    if (hasKey(key) && valueType(key) == QVariant::String)
        return d->m_valueHash[key].m_comp.strPtr;
    return 0;
}

int VcsBaseClientSettings::intValue(const QString &key, int defaultValue) const
{
    if (hasKey(key) && valueType(key) == QVariant::Int)
        return d->m_valueHash[key].m_comp.intValue;
    return defaultValue;
}

bool VcsBaseClientSettings::boolValue(const QString &key, bool defaultValue) const
{
    if (hasKey(key) && valueType(key) == QVariant::Bool)
        return d->m_valueHash[key].m_comp.boolValue;
    return defaultValue;
}

QString VcsBaseClientSettings::stringValue(const QString &key, const QString &defaultValue) const
{
    if (hasKey(key))
        return d->m_valueHash[key].stringValue(defaultValue);
    return defaultValue;
}

QVariant VcsBaseClientSettings::value(const QString &key) const
{
    switch (valueType(key)) {
    case QVariant::Int:
        return intValue(key);
    case QVariant::Bool:
        return boolValue(key);
    case QVariant::String:
        return stringValue(key);
    case QVariant::Invalid:
        return QVariant();
    default:
        return QVariant();
    }
}

void VcsBaseClientSettings::setValue(const QString &key, const QVariant &v)
{
    if (SettingValue::isUsableVariantType(valueType(key))) {
        d->m_valueHash.insert(key, SettingValue(v));
        d->m_binaryFullPath.clear();
    }
}

QVariant::Type VcsBaseClientSettings::valueType(const QString &key) const
{
    if (hasKey(key))
        return d->m_valueHash[key].type();
    return QVariant::Invalid;
}

QString VcsBaseClientSettings::binaryPath() const
{
    if (d->m_binaryFullPath.isEmpty()) {
        d->m_binaryFullPath = Utils::Environment::systemEnvironment().searchInPath(
                    stringValue(binaryPathKey), stringValue(pathKey).split(
                        Utils::HostOsInfo::pathListSeparator()));
    }
    return d->m_binaryFullPath;
}

QString VcsBaseClientSettings::settingsGroup() const
{
    return d->m_settingsGroup;
}

void VcsBaseClientSettings::setSettingsGroup(const QString &group)
{
    d->m_settingsGroup = group;
}

void VcsBaseClientSettings::declareKey(const QString &key, const QVariant &defaultValue)
{
    if (SettingValue::isUsableVariantType(defaultValue.type())) {
        d->m_valueHash.insert(key, SettingValue(defaultValue));
        d->m_defaultValueHash.insert(key, defaultValue);
    }
}

QVariant VcsBaseClientSettings::keyDefaultValue(const QString &key) const
{
    if (d->m_defaultValueHash.contains(key))
        return d->m_defaultValueHash.value(key);
    return QVariant(valueType(key));
}

} // namespace VcsBase
