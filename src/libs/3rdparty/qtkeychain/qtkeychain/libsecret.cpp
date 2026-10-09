#if defined(HAVE_LIBSECRET)
#  include <libsecret/secret.h>
#endif

#include "libsecret_p.h"

#include <QLibrary>

#if defined(HAVE_LIBSECRET)
const SecretSchema *qtkeychainSchema(void)
{
    static const SecretSchema schema = { "org.qt.keychain",
                                         SECRET_SCHEMA_DONT_MATCH_NAME,
                                         { { "user", SECRET_SCHEMA_ATTRIBUTE_STRING },
                                           { "server", SECRET_SCHEMA_ATTRIBUTE_STRING },
                                           { "type", SECRET_SCHEMA_ATTRIBUTE_STRING } } };

    return &schema;
}

typedef struct
{
    QKeychain::JobPrivate *self;
    QString user;
    QString server;
} callbackArg;

typedef void (*secret_password_lookup_t)(const SecretSchema *schema, GCancellable *cancellable,
                                         GAsyncReadyCallback callback, gpointer user_data,
                                         ...) G_GNUC_NULL_TERMINATED;
typedef gchar *(*secret_password_lookup_finish_t)(GAsyncResult *result, GError **error);
typedef void (*secret_password_store_t)(const SecretSchema *schema, const gchar *collection,
                                        const gchar *label, const gchar *password,
                                        GCancellable *cancellable, GAsyncReadyCallback callback,
                                        gpointer user_data, ...) G_GNUC_NULL_TERMINATED;
typedef gboolean (*secret_password_store_finish_t)(GAsyncResult *result, GError **error);
typedef void (*secret_password_clear_t)(const SecretSchema *schema, GCancellable *cancellable,
                                        GAsyncReadyCallback callback, gpointer user_data,
                                        ...) G_GNUC_NULL_TERMINATED;
typedef gboolean (*secret_password_clear_finish_t)(GAsyncResult *result, GError **error);
typedef void (*secret_password_free_t)(gchar *password);
typedef GQuark (*secret_error_get_quark_t)(void) G_GNUC_CONST;

static secret_password_lookup_t secret_password_lookup_fn = nullptr;
static secret_password_lookup_finish_t secret_password_lookup_finish_fn = nullptr;
static secret_password_store_t secret_password_store_fn = nullptr;
static secret_password_store_finish_t secret_password_store_finish_fn = nullptr;
static secret_password_clear_t secret_password_clear_fn = nullptr;
static secret_password_clear_finish_t secret_password_clear_finish_fn = nullptr;
static secret_password_free_t secret_password_free_fn = nullptr;
static secret_error_get_quark_t secret_error_get_quark_fn = nullptr;

static QKeychain::Error gerrorToCode(const GError *error)
{
    if (error->domain != secret_error_get_quark_fn()) {
        return QKeychain::OtherError;
    }

    switch (error->code) {
    case SECRET_ERROR_NO_SUCH_OBJECT:
        return QKeychain::EntryNotFound;
    case SECRET_ERROR_IS_LOCKED:
        return QKeychain::AccessDenied;
    default:
        return QKeychain::OtherError;
    }
}

static void on_password_lookup(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    callbackArg *arg = (callbackArg *)inst;
    gchar *password = secret_password_lookup_finish_fn(result, &error);

    Q_UNUSED(source);

    if (arg) {
        if (error) {
            QKeychain::Error code = gerrorToCode(error);

            arg->self->q->emitFinishedWithError(code, QString::fromUtf8(error->message));
        } else {
            if (password) {
                QByteArray raw = QByteArray(password);
                switch (arg->self->mode) {
                case QKeychain::JobPrivate::Binary:
                    arg->self->data = QByteArray::fromBase64(raw);
                    break;
                case QKeychain::JobPrivate::Text:
                default:
                    arg->self->data = raw;
                }

                arg->self->q->emitFinished();
            } else if (arg->self->mode == QKeychain::JobPrivate::Text) {
                arg->self->mode = QKeychain::JobPrivate::Binary;
                secret_password_lookup_fn(qtkeychainSchema(), nullptr, on_password_lookup, arg,
                                          "user", arg->user.toUtf8().constData(), "server",
                                          arg->server.toUtf8().constData(), "type", "base64",
                                          nullptr);
                return;
            } else {
                arg->self->q->emitFinishedWithError(QKeychain::EntryNotFound,
                                                    QObject::tr("Entry not found"));
            }
        }
    }
    if (error) {
        g_error_free(error);
    }

    if (password) {
        secret_password_free_fn(password);
    }

    if (arg) {
        delete arg;
    }
}

static void on_password_stored(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QKeychain::JobPrivate *self = (QKeychain::JobPrivate *)inst;

    Q_UNUSED(source);

    secret_password_store_finish_fn(result, &error);

    if (self) {
        if (error) {
            self->q->emitFinishedWithError(gerrorToCode(error), QString::fromUtf8(error->message));
        } else {
            self->q->emitFinished();
        }
    }
    if (error) {
        g_error_free(error);
    }
}

static void on_password_cleared(GObject *source, GAsyncResult *result, gpointer inst)
{
    GError *error = nullptr;
    QKeychain::JobPrivate *self = (QKeychain::JobPrivate *)inst;
    gboolean removed = secret_password_clear_finish_fn(result, &error);

    Q_UNUSED(source);
    if (self) {
        if (error) {
            self->q->emitFinishedWithError(gerrorToCode(error), QString::fromUtf8(error->message));
        } else {
            Q_UNUSED(removed);
            self->q->emitFinished();
        }
    }
    if (error) {
        g_error_free(error);
    }
}

static QString modeToString(QKeychain::JobPrivate::Mode mode)
{
    switch (mode) {
    case QKeychain::JobPrivate::Binary:
        return "base64";
    default:
        return "plaintext";
    }
}
#endif

bool LibSecretKeyring::isAvailable()
{
#if defined(HAVE_LIBSECRET)
    const LibSecretKeyring &keyring = instance();
    if (!keyring.isLoaded())
        return false;
    if (secret_password_lookup_fn == nullptr)
        return false;
    if (secret_password_lookup_finish_fn == nullptr)
        return false;
    if (secret_password_store_fn == nullptr)
        return false;
    if (secret_password_store_finish_fn == nullptr)
        return false;
    if (secret_password_clear_fn == nullptr)
        return false;
    if (secret_password_clear_finish_fn == nullptr)
        return false;
    if (secret_password_free_fn == nullptr)
        return false;
    if (secret_error_get_quark_fn == nullptr)
        return false;
    return true;
#else
    return false;
#endif
}

bool LibSecretKeyring::findPassword(const QString &user, const QString &server,
                                    QKeychain::JobPrivate *self)
{
#if defined(HAVE_LIBSECRET)
    if (!isAvailable()) {
        return false;
    }

    self->mode = QKeychain::JobPrivate::Text;
    self->data = QByteArray();

    callbackArg *arg = new callbackArg;
    arg->self = self;
    arg->user = user;
    arg->server = server;

    secret_password_lookup_fn(qtkeychainSchema(), nullptr, on_password_lookup, arg, "user",
                              user.toUtf8().constData(), "server", server.toUtf8().constData(),
                              "type", "plaintext", nullptr);
    return true;
#else
    Q_UNUSED(user)
    Q_UNUSED(server)
    Q_UNUSED(self)
    return false;
#endif
}

bool LibSecretKeyring::writePassword(const QString &display_name, const QString &user,
                                     const QString &server, const QKeychain::JobPrivate::Mode mode,
                                     const QByteArray &password, QKeychain::JobPrivate *self)
{
#if defined(HAVE_LIBSECRET)
    if (!isAvailable()) {
        return false;
    }

    QString type = modeToString(mode);
    QByteArray pwd;
    switch (mode) {
    case QKeychain::JobPrivate::Binary:
        pwd = password.toBase64();
        break;
    default:
        pwd = password;
        break;
    }

    secret_password_store_fn(
            qtkeychainSchema(), SECRET_COLLECTION_DEFAULT, display_name.toUtf8().constData(),
            pwd.constData(), nullptr, on_password_stored, self, "user", user.toUtf8().constData(),
            "server", server.toUtf8().constData(), "type", type.toUtf8().constData(), nullptr);
    return true;
#else
    Q_UNUSED(display_name)
    Q_UNUSED(user)
    Q_UNUSED(server)
    Q_UNUSED(mode)
    Q_UNUSED(password)
    Q_UNUSED(self)
    return false;
#endif
}

bool LibSecretKeyring::deletePassword(const QString &key, const QString &service,
                                      QKeychain::JobPrivate *self)
{
#if defined(HAVE_LIBSECRET)
    if (!isAvailable()) {
        return false;
    }

    secret_password_clear_fn(qtkeychainSchema(), nullptr, on_password_cleared, self, "user",
                             key.toUtf8().constData(), "server", service.toUtf8().constData(),
                             nullptr);
    return true;
#else
    Q_UNUSED(key)
    Q_UNUSED(service)
    Q_UNUSED(self)
    return false;
#endif
}

LibSecretKeyring::LibSecretKeyring() : QLibrary(QLatin1String("secret-1"), 0)
{
#ifdef HAVE_LIBSECRET
    if (load()) {
        secret_password_lookup_fn = (secret_password_lookup_t)resolve("secret_password_lookup");
        secret_password_lookup_finish_fn =
                (secret_password_lookup_finish_t)resolve("secret_password_lookup_finish");
        secret_password_store_fn = (secret_password_store_t)resolve("secret_password_store");
        secret_password_store_finish_fn =
                (secret_password_store_finish_t)resolve("secret_password_store_finish");
        secret_password_clear_fn = (secret_password_clear_t)resolve("secret_password_clear");
        secret_password_clear_finish_fn =
                (secret_password_clear_finish_t)resolve("secret_password_clear_finish");
        secret_password_free_fn = (secret_password_free_t)resolve("secret_password_free");
        secret_error_get_quark_fn = (secret_error_get_quark_t)resolve("secret_error_get_quark");
    }
#endif
}

LibSecretKeyring &LibSecretKeyring::instance()
{
    static LibSecretKeyring instance;

    return instance;
}
