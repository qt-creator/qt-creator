#ifndef QTKEYCHAIN_LIBSECRET_P_H
#define QTKEYCHAIN_LIBSECRET_P_H

#include <QLibrary>

#include "keychain_p.h"

class LibSecretKeyring : public QLibrary
{
public:
    static bool isAvailable();

    static bool findPassword(const QString &user, const QString &server,
                             QKeychain::JobPrivate *self);

    static bool writePassword(const QString &display_name, const QString &user,
                              const QString &server, const QKeychain::JobPrivate::Mode type,
                              const QByteArray &password, QKeychain::JobPrivate *self);

    static bool deletePassword(const QString &key, const QString &service,
                               QKeychain::JobPrivate *self);

private:
    LibSecretKeyring();

    static LibSecretKeyring &instance();
};

#endif
