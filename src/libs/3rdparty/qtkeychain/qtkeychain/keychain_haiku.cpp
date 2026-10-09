/******************************************************************************
 *   Copyright (C) 2018 François Revol <revol@free.fr>                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include "keychain_p.h"

#include <KeyStore.h>

#include <Application.h>
#include <AppFileInfo.h>
#include <File.h>

#include <QDebug>
#include <QCoreApplication>
#include <QString>

using namespace QKeychain;

class AutoApp
{
public:
    AutoApp();
    ~AutoApp();
    BApplication *app;
};

AutoApp::AutoApp() : app(nullptr)
{
    if (be_app)
        return;

    // no BApplication object, probably using QCoreApplication
    // but we need one around

    QString appSignature;

    char signature[B_MIME_TYPE_LENGTH];
    signature[0] = '\0';

    QString appPath = QCoreApplication::applicationFilePath();

    BFile appFile(appPath.toUtf8(), B_READ_ONLY);
    if (appFile.InitCheck() == B_OK) {
        BAppFileInfo info(&appFile);
        if (info.InitCheck() == B_OK) {
            if (info.GetSignature(signature) != B_OK)
                signature[0] = '\0';
        }
    }

    if (signature[0] != '\0')
        appSignature = QLatin1String(signature);
    else
        appSignature = QLatin1String("application/x-vnd.qtkeychain-")
                + QCoreApplication::applicationName().remove("_x86");

    app = new BApplication(appSignature.toUtf8().constData());
}

AutoApp::~AutoApp()
{
    delete app;
}

static QString strForStatus(status_t os)
{
    const char *const buf = strerror(os);
    return QObject::tr("error 0x%1: %2").arg(os, 8, 16).arg(QString::fromUtf8(buf, strlen(buf)));
}

void ReadPasswordJobPrivate::scheduledStart()
{
    AutoApp aa;
    QString errorString;
    Error error = NoError;
    BKeyStore keyStore;
    BPasswordKey password;

    status_t result = keyStore.GetKey(B_KEY_TYPE_PASSWORD, q->service().toUtf8().constData(),
                                      q->key().toUtf8().constData(), false, password);

    data = QByteArray(reinterpret_cast<const char *>(password.Data()));

    switch (result) {
    case B_OK:
        q->emitFinished();
        return;
    case B_ENTRY_NOT_FOUND:
        errorString = tr("Password not found");
        error = EntryNotFound;
        break;
    default:
        errorString = strForStatus(result);
        error = OtherError;
        break;
    }

    q->emitFinishedWithError(error, errorString);
}

void WritePasswordJobPrivate::scheduledStart()
{
    AutoApp aa;
    QString errorString;
    Error error = NoError;
    BKeyStore keyStore;
    BPasswordKey password(data.constData(), B_KEY_PURPOSE_GENERIC,
                          q->service().toUtf8().constData(), q->key().toUtf8().constData());
    status_t result = B_OK;

    // re-add as binary if it's not text
    if (mode == Binary)
        result = password.SetData(reinterpret_cast<const uint8 *>(data.constData()), data.size());

    if (result == B_OK)
        result = keyStore.AddKey(password);

    if (result == B_NAME_IN_USE) {
        BPasswordKey old_password;
        result = keyStore.GetKey(B_KEY_TYPE_PASSWORD, q->service().toUtf8().constData(),
                                 q->key().toUtf8().constData(), false, old_password);
        if (result == B_OK)
            result = keyStore.RemoveKey(old_password);
        if (result == B_OK)
            result = keyStore.AddKey(password);
    }

    switch (result) {
    case B_OK:
        q->emitFinished();
        return;
    case B_ENTRY_NOT_FOUND:
        errorString = tr("Password not found");
        error = EntryNotFound;
        break;
    default:
        errorString = strForStatus(result);
        error = OtherError;
        break;
    }

    q->emitFinishedWithError(error, errorString);
}

void DeletePasswordJobPrivate::scheduledStart()
{
    AutoApp aa;
    QString errorString;
    Error error = NoError;
    BKeyStore keyStore;
    BPasswordKey password;

    status_t result = keyStore.GetKey(B_KEY_TYPE_PASSWORD, q->service().toUtf8().constData(),
                                      q->key().toUtf8().constData(), false, password);

    if (result == B_OK)
        result = keyStore.RemoveKey(password);

    switch (result) {
    case B_OK:
        q->emitFinished();
        return;
    case B_ENTRY_NOT_FOUND:
        errorString = tr("Password not found");
        error = EntryNotFound;
        break;
    default:
        errorString = strForStatus(result);
        error = CouldNotDeleteEntry;
        break;
    }

    q->emitFinishedWithError(error, errorString);
}

bool QKeychain::isAvailable()
{
    return true;
}
