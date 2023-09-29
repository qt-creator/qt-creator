/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include "keychain_p.h"
#include "gnomekeyring_p.h"
#include "libsecret_p.h"
#include "plaintextstore_p.h"

#include <QScopedPointer>

using namespace QKeychain;

enum KeyringBackend {
    Backend_LibSecretKeyring,
    Backend_GnomeKeyring,
    Backend_Kwallet4,
    Backend_Kwallet5,
    Backend_Kwallet6,
};

enum DesktopEnvironment {
    DesktopEnv_Gnome,
    DesktopEnv_Kde4,
    DesktopEnv_Plasma5,
    DesktopEnv_Plasma6,
    DesktopEnv_Unity,
    DesktopEnv_Xfce,
    DesktopEnv_Other
};

static constexpr const char KWALLET6_DBUS_IFACE[] = "org.kde.kwalletd6";
static constexpr const char KWALLET6_DBUS_PATH[] = "/modules/kwalletd6";
static constexpr const char KWALLET5_DBUS_IFACE[] = "org.kde.kwalletd5";
static constexpr const char KWALLET5_DBUS_PATH[] = "/modules/kwalletd5";
static constexpr const char KWALLET4_DBUS_IFACE[] = "org.kde.kwalletd";
static constexpr const char KWALLET4_DBUS_PATH[] = "/modules/kwalletd";

// the following detection algorithm is derived from chromium,
// licensed under BSD, see base/nix/xdg_util.cc

static DesktopEnvironment getKdeVersion() {
    QByteArray value = qgetenv("KDE_SESSION_VERSION");
    if ( value == "6" ) {
        return DesktopEnv_Plasma6;
    } else if ( value == "5" ) {
        return DesktopEnv_Plasma5;
    } else if (value == "4" ) {
        return DesktopEnv_Kde4;
    } else {
        // most likely KDE3
        return DesktopEnv_Other;
    }
}

static DesktopEnvironment detectDesktopEnvironment() {
    QByteArray xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    if ( xdgCurrentDesktop == "GNOME" ) {
        return DesktopEnv_Gnome;
    } else if ( xdgCurrentDesktop == "Unity" ) {
        return DesktopEnv_Unity;
    } else if ( xdgCurrentDesktop == "KDE" ) {
        return getKdeVersion();
    } else if ( xdgCurrentDesktop == "XFCE" ) {
        return DesktopEnv_Xfce;
    }

    QByteArray desktopSession = qgetenv("DESKTOP_SESSION");
    if ( desktopSession == "gnome" ) {
        return DesktopEnv_Gnome;
    } else if ( desktopSession == "kde" ) {
        return getKdeVersion();
    } else if ( desktopSession == "kde4" ) {
        return DesktopEnv_Kde4;
    } else if ( desktopSession.contains("xfce") || desktopSession == "xubuntu" ) {
        return DesktopEnv_Xfce;
    }

    if ( !qgetenv("GNOME_DESKTOP_SESSION_ID").isEmpty() ) {
        return DesktopEnv_Gnome;
    } else if ( !qgetenv("KDE_FULL_SESSION").isEmpty() ) {
        return getKdeVersion();
    }

    return DesktopEnv_Other;
}

static bool isKwalletAvailable(const char *dbusIface, const char *dbusPath)
{
    if (!QDBusConnection::sessionBus().isConnected())
        return false;

    org::kde::KWallet iface(
        QLatin1String(dbusIface),
        QLatin1String(dbusPath),
        QDBusConnection::sessionBus());

    // At this point iface.isValid() can return false even though the
    // interface is activatable by making a call. Hence we check whether
    // a wallet can be opened.

    iface.setTimeout(500);
    QDBusMessage reply = iface.call(QLatin1String("networkWallet"));
    return reply.type() == QDBusMessage::ReplyMessage;
}

static KeyringBackend detectKeyringBackend()
{
    /* The secret service dbus api, accessible through libsecret, is supposed
     * to unify password services.
     *
     * Unfortunately at the time of Kubuntu 18.04 the secret service backend
     * in KDE is gnome-keyring-daemon - using it has several complications:
     * - the default collection isn't opened on session start, so users need
     *   to manually unlock it when the first application uses it
     * - it's separate from the kwallet5 keyring, so switching to it means the
     *   existing keyring data can't be accessed anymore
     *
     * Thus we still prefer kwallet backends on KDE even if libsecret is
     * available.
     */

    switch (detectDesktopEnvironment()) {
    case DesktopEnv_Kde4:
        return Backend_Kwallet4;

    case DesktopEnv_Plasma5:
        if (isKwalletAvailable(KWALLET5_DBUS_IFACE, KWALLET5_DBUS_PATH)) {
            return Backend_Kwallet5;
        }
        if (LibSecretKeyring::isAvailable()) {
            return Backend_LibSecretKeyring;
        }
        if (GnomeKeyring::isAvailable()) {
            return Backend_GnomeKeyring;
        }
        // During startup the keychain backend might just not have started yet
        return Backend_Kwallet5;

    case DesktopEnv_Plasma6:
        if (isKwalletAvailable(KWALLET6_DBUS_IFACE, KWALLET6_DBUS_PATH)) {
            return Backend_Kwallet6;
        }
        if (LibSecretKeyring::isAvailable()) {
            return Backend_LibSecretKeyring;
        }
        if (GnomeKeyring::isAvailable()) {
            return Backend_GnomeKeyring;
        }
        // During startup the keychain backend might just not have started yet
        return Backend_Kwallet6;

    case DesktopEnv_Gnome:
    case DesktopEnv_Unity:
    case DesktopEnv_Xfce:
    case DesktopEnv_Other:
    default:
        if (LibSecretKeyring::isAvailable()) {
            return Backend_LibSecretKeyring;
        }
        if (GnomeKeyring::isAvailable()) {
            return Backend_GnomeKeyring;
        }
        if (isKwalletAvailable(KWALLET6_DBUS_IFACE, KWALLET6_DBUS_PATH)) {
            return Backend_Kwallet6;
        }
        if (isKwalletAvailable(KWALLET5_DBUS_IFACE, KWALLET5_DBUS_PATH)) {
            return Backend_Kwallet5;
        }
        // During startup the keychain backend might just not have started yet
        //
        // This doesn't need to be libsecret because LibSecretKeyring::isAvailable()
        // only fails if the libsecret shared library couldn't be loaded. In contrast
        // to that GnomeKeyring::isAvailable() can return false if the shared library
        // *was* loaded but its libgnome_keyring::is_available() returned false.
        //
        // In the future there should be a difference between "API available" and
        // "keychain available".
        return Backend_GnomeKeyring;
    }

}

static KeyringBackend getKeyringBackend()
{
    static KeyringBackend backend = detectKeyringBackend();
    return backend;
}

static void kwalletReadPasswordScheduledStartImpl(const char * service, const char * path, ReadPasswordJobPrivate * priv) {
    if ( QDBusConnection::sessionBus().isConnected() )
    {
        priv->iface = new org::kde::KWallet( QLatin1String(service), QLatin1String(path), QDBusConnection::sessionBus(), priv );
        const QDBusPendingReply<QString> reply = priv->iface->networkWallet();
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher( reply, priv );
        priv->connect( watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), priv, SLOT(kwalletWalletFound(QDBusPendingCallWatcher*)) );
    }
    else
    {
        // D-Bus is not reachable so none can tell us something about KWalletd
        QDBusError err( QDBusError::NoServer, ReadPasswordJobPrivate::tr("D-Bus is not running") );
        priv->fallbackOnError( err );
    }
}

void ReadPasswordJobPrivate::scheduledStart() {
    switch ( getKeyringBackend() ) {
    case Backend_LibSecretKeyring: {
        if ( !LibSecretKeyring::findPassword(key, q->service(), this) ) {
            q->emitFinishedWithError( OtherError, tr("Unknown error") );
        }
    } break;
    case Backend_GnomeKeyring:
        this->mode = JobPrivate::Text;
        if ( !GnomeKeyring::find_network_password( key.toUtf8().constData(),
                                                   q->service().toUtf8().constData(),
                                                   "plaintext",
                                                   reinterpret_cast<GnomeKeyring::OperationGetStringCallback>( &JobPrivate::gnomeKeyring_readCb ),
                                                   this, 0 ) )
            q->emitFinishedWithError( OtherError, tr("Unknown error") );
        break;

    case Backend_Kwallet4:
        kwalletReadPasswordScheduledStartImpl(KWALLET4_DBUS_IFACE, KWALLET4_DBUS_PATH, this);
        break;
    case Backend_Kwallet5:
        kwalletReadPasswordScheduledStartImpl(KWALLET5_DBUS_IFACE, KWALLET5_DBUS_PATH, this);
        break;
    case Backend_Kwallet6:
        kwalletReadPasswordScheduledStartImpl(KWALLET6_DBUS_IFACE, KWALLET6_DBUS_PATH, this);
        break;
    }
}

void JobPrivate::kwalletWalletFound(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    const QDBusPendingReply<QString> reply = *watcher;
    const QDBusPendingReply<int> pendingReply = iface->open( reply.value(), 0, q->service() );
    QDBusPendingCallWatcher* pendingWatcher = new QDBusPendingCallWatcher( pendingReply, this );
    connect( pendingWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
             this, SLOT(kwalletOpenFinished(QDBusPendingCallWatcher*)) );
}

static QPair<Error, QString> mapGnomeKeyringError( int result )
{
    Q_ASSERT( result != GnomeKeyring::RESULT_OK );

    switch ( result ) {
    case GnomeKeyring::RESULT_DENIED:
        return qMakePair( AccessDenied, QObject::tr("Access to keychain denied") );
    case GnomeKeyring::RESULT_NO_KEYRING_DAEMON:
        return qMakePair( NoBackendAvailable, QObject::tr("No keyring daemon") );
    case GnomeKeyring::RESULT_ALREADY_UNLOCKED:
        return qMakePair( OtherError, QObject::tr("Already unlocked") );
    case GnomeKeyring::RESULT_NO_SUCH_KEYRING:
        return qMakePair( OtherError, QObject::tr("No such keyring") );
    case GnomeKeyring::RESULT_BAD_ARGUMENTS:
        return qMakePair( OtherError, QObject::tr("Bad arguments") );
    case GnomeKeyring::RESULT_IO_ERROR:
        return qMakePair( OtherError, QObject::tr("I/O error") );
    case GnomeKeyring::RESULT_CANCELLED:
        return qMakePair( OtherError, QObject::tr("Cancelled") );
    case GnomeKeyring::RESULT_KEYRING_ALREADY_EXISTS:
        return qMakePair( OtherError, QObject::tr("Keyring already exists") );
    case GnomeKeyring::RESULT_NO_MATCH:
        return qMakePair(  EntryNotFound, QObject::tr("No match") );
    default:
        break;
    }

    return qMakePair( OtherError, QObject::tr("Unknown error") );
}

void JobPrivate::gnomeKeyring_readCb( int result, const char* string, JobPrivate* self )
{
    if ( result == GnomeKeyring::RESULT_OK ) {
        if (self->mode == JobPrivate::Text)
            self->data = QByteArray(string);
        else
            self->data = QByteArray::fromBase64(string);

        self->q->emitFinished();
    } else if (self->mode == JobPrivate::Text) {
        self->mode = JobPrivate::Binary;
        if ( !GnomeKeyring::find_network_password( self->key.toUtf8().constData(),
                                                   self->q->service().toUtf8().constData(),
                                                   "base64",
                                                   reinterpret_cast<GnomeKeyring::OperationGetStringCallback>( &JobPrivate::gnomeKeyring_readCb ),
                                                   self, 0 ) )
            self->q->emitFinishedWithError( OtherError, tr("Unknown error") );
    } else {
        const QPair<Error, QString> errorResult = mapGnomeKeyringError( result );
        self->q->emitFinishedWithError( errorResult.first, errorResult.second );
    }
}

void ReadPasswordJobPrivate::fallbackOnError(const QDBusError& err )
{
    PlainTextStore plainTextStore( q->service(), q->settings() );

    if ( q->insecureFallback() && plainTextStore.contains( key ) ) {
        mode = plainTextStore.readMode( key );
        data = plainTextStore.readData( key );

        if ( plainTextStore.error() != NoError )
            q->emitFinishedWithError( plainTextStore.error(), plainTextStore.errorString() );
        else
            q->emitFinished();
    } else {
        if ( err.type() == QDBusError::ServiceUnknown ) //KWalletd not running
            q->emitFinishedWithError( NoBackendAvailable, tr("No keychain service available") );
        else
            q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
    }
}

void ReadPasswordJobPrivate::kwalletOpenFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    const QDBusPendingReply<int> reply = *watcher;

    if ( reply.isError() ) {
        fallbackOnError( reply.error() );
        return;
    }

    PlainTextStore plainTextStore( q->service(), q->settings() );

    if ( plainTextStore.contains( key ) ) {
        // We previously stored data in the insecure QSettings, but now have KWallet available.
        // Do the migration

        data = plainTextStore.readData( key );
        const WritePasswordJobPrivate::Mode mode = plainTextStore.readMode( key );
        plainTextStore.remove( key );

        q->emitFinished();


        WritePasswordJob* j = new WritePasswordJob( q->service(), 0 );
        j->setSettings( q->settings() );
        j->setKey( key );
        j->setAutoDelete( true );
        if ( mode == WritePasswordJobPrivate::Binary )
            j->setBinaryData( data );
        else if ( mode == WritePasswordJobPrivate::Text )
            j->setTextData( QString::fromUtf8( data ) );
        else
            Q_ASSERT( false );

        j->start();

        return;
    }

    walletHandle = reply.value();

    if ( walletHandle < 0 ) {
        q->emitFinishedWithError( AccessDenied, tr("Access to keychain denied") );
        return;
    }

    const QDBusPendingReply<int> nextReply = iface->entryType( walletHandle, q->service(), key, q->service() );
    QDBusPendingCallWatcher* nextWatcher = new QDBusPendingCallWatcher( nextReply, this );
    connect( nextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletEntryTypeFinished(QDBusPendingCallWatcher*)) );
}

//Must be in sync with KWallet::EntryType (kwallet.h)
enum KWalletEntryType {
    Unknown=0,
    Password,
    Stream,
    Map
};

void ReadPasswordJobPrivate::kwalletEntryTypeFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    if ( watcher->isError() ) {
        const QDBusError err = watcher->error();
        q->emitFinishedWithError( OtherError, tr("Could not determine data type: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
        return;
    }

    const QDBusPendingReply<int> reply = *watcher;
    const int value = reply.value();

    switch ( value ) {
    case Unknown:
        q->emitFinishedWithError( EntryNotFound, tr("Entry not found") );
        return;
    case Password:
        mode = Text;
        break;
    case Stream:
        mode = Binary;
        break;
    case Map:
        q->emitFinishedWithError( EntryNotFound, tr("Unsupported entry type 'Map'") );
        return;
    default:
        q->emitFinishedWithError( OtherError, tr("Unknown kwallet entry type '%1'").arg( value ) );
        return;
    }

    const QDBusPendingCall nextReply = (mode == Text)
            ? QDBusPendingCall( iface->readPassword( walletHandle, q->service(), key, q->service() ) )
            : QDBusPendingCall( iface->readEntry( walletHandle, q->service(), key, q->service() ) );
    QDBusPendingCallWatcher* nextWatcher = new QDBusPendingCallWatcher( nextReply, this );
    connect( nextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletFinished(QDBusPendingCallWatcher*)) );
}

void ReadPasswordJobPrivate::kwalletFinished( QDBusPendingCallWatcher* watcher ) {
    if ( !watcher->isError() ) {
        if ( mode == Binary ) {
            QDBusPendingReply<QByteArray> reply = *watcher;
            if (reply.isValid()) {
                data = reply.value();
            }
        } else {
            QDBusPendingReply<QString> reply = *watcher;
            if (reply.isValid()) {
                data = reply.value().toUtf8();
            }
        }
    }

    JobPrivate::kwalletFinished(watcher);
}

static void kwalletWritePasswordScheduledStart( const char * service, const char * path, JobPrivate * priv ) {
    if ( QDBusConnection::sessionBus().isConnected() )
    {
        priv->iface = new org::kde::KWallet( QLatin1String(service), QLatin1String(path), QDBusConnection::sessionBus(), priv );
        const QDBusPendingReply<QString> reply = priv->iface->networkWallet();
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher( reply, priv );
        priv->connect( watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), priv, SLOT(kwalletWalletFound(QDBusPendingCallWatcher*)) );
    }
    else
    {
        // D-Bus is not reachable so none can tell us something about KWalletd
        QDBusError err( QDBusError::NoServer, WritePasswordJobPrivate::tr("D-Bus is not running") );
        priv->fallbackOnError( err );
    }
}

void WritePasswordJobPrivate::scheduledStart() {
    switch ( getKeyringBackend() ) {
    case Backend_LibSecretKeyring: {
        if ( !LibSecretKeyring::writePassword(service, key, service, mode,
                                              data, this) ) {
            q->emitFinishedWithError( OtherError, tr("Unknown error") );
        }
    } break;
    case Backend_GnomeKeyring: {
        QString type;
        QByteArray password;

        switch(mode) {
        case JobPrivate::Text:
            type = QLatin1String("plaintext");
            password = data;
            break;
        default:
            type = QLatin1String("base64");
            password = data.toBase64();
            break;
        }

        QByteArray service = q->service().toUtf8();
        if ( !GnomeKeyring::store_network_password( GnomeKeyring::GNOME_KEYRING_DEFAULT,
                                                    service.constData(),
                                                    key.toUtf8().constData(),
                                                    service.constData(),
                                                    type.toUtf8().constData(),
                                                    password.constData(),
                                                    reinterpret_cast<GnomeKeyring::OperationDoneCallback>( &JobPrivate::gnomeKeyring_writeCb ),
                                                    this, 0 ) )
            q->emitFinishedWithError( OtherError, tr("Unknown error") );
    }
        break;

    case Backend_Kwallet4:
        kwalletWritePasswordScheduledStart(KWALLET4_DBUS_IFACE, KWALLET4_DBUS_PATH, this);
        break;
    case Backend_Kwallet5:
        kwalletWritePasswordScheduledStart(KWALLET5_DBUS_IFACE, KWALLET5_DBUS_PATH, this);
        break;
    case Backend_Kwallet6:
        kwalletWritePasswordScheduledStart(KWALLET6_DBUS_IFACE, KWALLET6_DBUS_PATH, this);
        break;
    }
}

void WritePasswordJobPrivate::fallbackOnError(const QDBusError &err)
{
    if ( !q->insecureFallback() ) {
        q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2").arg( QDBusError::errorString( err.type() ), err.message() ) );
        return;
    }

    PlainTextStore plainTextStore( q->service(), q->settings() );
    plainTextStore.write( key, data, mode );

    if ( plainTextStore.error() != NoError )
        q->emitFinishedWithError( plainTextStore.error(), plainTextStore.errorString() );
    else
        q->emitFinished();
}

void JobPrivate::gnomeKeyring_writeCb(int result, JobPrivate* self )
{
    if ( result == GnomeKeyring::RESULT_OK ) {
        self->q->emitFinished();
    } else {
        const QPair<Error, QString> errorResult = mapGnomeKeyringError( result );
        self->q->emitFinishedWithError( errorResult.first, errorResult.second );
    }
}

void JobPrivate::kwalletOpenFinished( QDBusPendingCallWatcher* watcher ) {
    watcher->deleteLater();
    QDBusPendingReply<int> reply = *watcher;

    if ( reply.isError() ) {
        fallbackOnError( reply.error() );
        return;
    }

    PlainTextStore plainTextStore( q->service(), q->settings() );
    if ( plainTextStore.contains( key ) ) {
        // If we had previously written to QSettings, but we now have a kwallet available, migrate and delete old insecure data
        plainTextStore.remove( key );
    }

    const int handle = reply.value();

    if ( handle < 0 ) {
        q->emitFinishedWithError( AccessDenied, tr("Access to keychain denied") );
        return;
    }

    QDBusPendingReply<int> nextReply;

    if ( !data.isNull() ) {
        if ( mode == Text ) {
            nextReply = iface->writePassword( handle, q->service(), key, QString::fromUtf8(data), q->service() );
        } else {
            Q_ASSERT( mode == Binary );
            nextReply = iface->writeEntry( handle, q->service(), key, data, q->service() );
        }
    } else {
        nextReply = iface->removeEntry( handle, q->service(), key, q->service() );
    }

    QDBusPendingCallWatcher* nextWatcher = new QDBusPendingCallWatcher( nextReply, this );
    connect( nextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(kwalletFinished(QDBusPendingCallWatcher*)) );
}

void JobPrivate::kwalletFinished( QDBusPendingCallWatcher* watcher ) {
    if ( !watcher->isError() ) {
        if ( mode == Binary ) {
            QDBusPendingReply<QByteArray> reply = *watcher;
            if (reply.isValid()) {
                data = reply.value();
            }
        } else {
            QDBusPendingReply<QString> reply = *watcher;
            if (reply.isValid()) {
                data = reply.value().toUtf8();
            }
        }
    }

    q->emitFinished();
}

void DeletePasswordJobPrivate::scheduledStart() {
    switch ( getKeyringBackend() ) {
    case Backend_LibSecretKeyring: {
        if ( !LibSecretKeyring::deletePassword(key, q->service(), this) ) {
            q->emitFinishedWithError( OtherError, tr("Unknown error") );
        }
    } break;
    case Backend_GnomeKeyring: {
        if ( !GnomeKeyring::delete_network_password(
                 key.toUtf8().constData(), q->service().toUtf8().constData(),
                 reinterpret_cast<GnomeKeyring::OperationDoneCallback>( &JobPrivate::gnomeKeyring_writeCb ),
                 this, 0 ) )
            q->emitFinishedWithError( OtherError, tr("Unknown error") );
    }
        break;

    case Backend_Kwallet4:
        kwalletWritePasswordScheduledStart(KWALLET4_DBUS_IFACE, KWALLET4_DBUS_PATH, this);
        break;
    case Backend_Kwallet5:
        kwalletWritePasswordScheduledStart(KWALLET5_DBUS_IFACE, KWALLET5_DBUS_PATH, this);
        break;
    case Backend_Kwallet6:
        kwalletWritePasswordScheduledStart(KWALLET6_DBUS_IFACE, KWALLET6_DBUS_PATH, this);
        break;
    }
}

void DeletePasswordJobPrivate::fallbackOnError(const QDBusError &err) {
    QScopedPointer<QSettings> local( !q->settings() ? new QSettings( q->service() ) : 0 );
    QSettings* actual = q->settings() ? q->settings() : local.data();

    if ( !q->insecureFallback() ) {
        q->emitFinishedWithError( OtherError, tr("Could not open wallet: %1; %2")
                                  .arg( QDBusError::errorString( err.type() ), err.message() ) );
        return;
    }

    actual->remove( key );
    actual->sync();

    q->emitFinished();


    q->emitFinished();
}

bool QKeychain::isAvailable()
{
    return LibSecretKeyring::isAvailable() || GnomeKeyring::isAvailable()
        || isKwalletAvailable(KWALLET6_DBUS_IFACE, KWALLET6_DBUS_PATH)
        || isKwalletAvailable(KWALLET5_DBUS_IFACE, KWALLET5_DBUS_PATH);
}
