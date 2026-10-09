/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *   Copyright (C) 2016 Mathias Hasselmann <mathias.hasselmann@kdab.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/

#ifndef QTKEYCHAIN_PLAINTEXTSTORE_P_H
#define QTKEYCHAIN_PLAINTEXTSTORE_P_H

#include "keychain_p.h"

namespace QKeychain {

class PlainTextStore
{
    Q_DECLARE_TR_FUNCTIONS(QKeychain::PlainTextStore)

public:
    explicit PlainTextStore(const QString &service, QSettings *settings);

    Error error() const { return m_error; }
    QString errorString() const { return m_errorString; }

    bool contains(const QString &key) const;

    QByteArray readData(const QString &key);
    JobPrivate::Mode readMode(const QString &key);

    void write(const QString &key, const QByteArray &data, JobPrivate::Mode mode);
    void remove(const QString &key);

private:
    void setError(Error error, const QString &errorString);
    QVariant read(const QString &key);

    const QScopedPointer<QSettings> m_localSettings;
    QSettings *const m_actualSettings;
    QString m_errorString;
    Error m_error;
};

} // namespace QKeychain

#endif // QTKEYCHAIN_PLAINTEXTSTORE_P_H
