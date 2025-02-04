// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtcsettings.h"
#include "store.h"

namespace Utils {

/*!
    \class Utils::QtcSettings
    \inheaderfile utils/qtcsettings.h
    \inmodule QtCreator

    \brief The QtcSettings class is an extension of the QSettings class
    the uses Utils::Key instead of QString for keys.

    Use Utils::QtcSettings::setValueWithDefault() to write values with a
    default.
*/

/*!
    \fn template<typename T> void setValueWithDefault(const QString &key, const T &val, const T &defaultValue)

    Sets the value of setting \a key to \a val. If \a val is the same as the \a
    defaultValue, the settings key is removed instead. This makes sure that
    settings are only written if actually necessary, namely when the user
    changed them from the default. It also makes a new default value for a
    setting in a new version of the application take effect, if the user did
    not change the setting before.

    \sa QSettings::setValue()
*/

/*!
    Begins the settings group \a prefix, calls the \a function, and ends the settings group.

    \sa beginGroup()
    \sa endGroup()
*/
void QtcSettings::withGroup(const Key &prefix, const std::function<void(QtcSettings *)> &function)
{
    beginGroup(prefix);
    function(this);
    endGroup();
}

void QtcSettings::beginGroup(const Key &prefix)
{
    QSettings::beginGroup(stringFromKey(prefix));
}

QVariant QtcSettings::value(const Key &key) const
{
    return QSettings::value(stringFromKey(key));
}

QVariant QtcSettings::value(const Key &key, const QVariant &def) const
{
    return QSettings::value(stringFromKey(key), def);
}

void QtcSettings::setValue(const Key &key, const QVariant &value)
{
    QSettings::setValue(stringFromKey(key), mapEntryFromStoreEntry(value));
}

void QtcSettings::remove(const Key &key)
{
    QSettings::remove(stringFromKey(key));
}

bool QtcSettings::contains(const Key &key) const
{
    return QSettings::contains(stringFromKey(key));
}

KeyList QtcSettings::childKeys() const
{
    return keysFromStrings(QSettings::childKeys());
}

} // namespace Utils
