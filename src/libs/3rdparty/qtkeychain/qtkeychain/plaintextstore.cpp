/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *   Copyright (C) 2016 Mathias Hasselmann <mathias.hasselmann@kdab.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/

#include "plaintextstore_p.h"

using namespace QKeychain;

namespace {
#ifdef Q_OS_WIN
inline QString dataKey(const QString &key)
{
    return key;
}
#else // Q_OS_WIN
inline QString dataKey(const QString &key)
{
    return key + QLatin1String("/data");
}
inline QString typeKey(const QString &key)
{
    return key + QLatin1String("/type");
}
#endif // Q_OS_WIN
} // namespace

PlainTextStore::PlainTextStore(const QString &service, QSettings *settings)
    : m_localSettings(settings ? nullptr : new QSettings(service)),
      m_actualSettings(settings ? settings : m_localSettings.data()),
      m_error(NoError)
{
}

bool PlainTextStore::contains(const QString &key) const
{
    return m_actualSettings->contains(dataKey(key));
}

QByteArray PlainTextStore::readData(const QString &key)
{
    return read(dataKey(key)).toByteArray();
}

#ifndef Q_OS_WIN

JobPrivate::Mode PlainTextStore::readMode(const QString &key)
{
    return JobPrivate::stringToMode(read(typeKey(key)).toString());
}

#endif // Q_OS_WIN

void PlainTextStore::write(const QString &key, const QByteArray &data, JobPrivate::Mode mode)
{
    if (m_actualSettings->status() != QSettings::NoError)
        return;

#ifndef Q_OS_WIN
    m_actualSettings->setValue(typeKey(key), JobPrivate::modeToString(mode));
#else // Q_OS_WIN
    Q_UNUSED(mode);
#endif // Q_OS_WIN
    m_actualSettings->setValue(dataKey(key), data);
    m_actualSettings->sync();

    if (m_actualSettings->status() == QSettings::AccessError) {
        setError(AccessDenied, tr("Could not store data in settings: access error"));
    } else if (m_actualSettings->status() != QSettings::NoError) {
        setError(OtherError, tr("Could not store data in settings: format error"));
    } else {
        setError(NoError, QString());
    }
}

void PlainTextStore::remove(const QString &key)
{
    if (m_actualSettings->status() != QSettings::NoError)
        return;

#ifndef Q_OS_WIN
    m_actualSettings->remove(typeKey(key));
#endif // Q_OS_WIN
    m_actualSettings->remove(dataKey(key));
    m_actualSettings->sync();

    if (m_actualSettings->status() == QSettings::AccessError) {
        setError(AccessDenied, tr("Could not delete data from settings: access error"));
    } else if (m_actualSettings->status() != QSettings::NoError) {
        setError(OtherError, tr("Could not delete data from settings: format error"));
    } else {
        setError(NoError, QString());
    }
}

void PlainTextStore::setError(Error error, const QString &errorString)
{
    m_error = error;
    m_errorString = errorString;
}

QVariant PlainTextStore::read(const QString &key)
{
    const QVariant value = m_actualSettings->value(key);

    if (value.isNull()) {
        setError(EntryNotFound, tr("Entry not found"));
    } else {
        setError(NoError, QString());
    }

    return value;
}
