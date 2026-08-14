// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "secretaspect.h"

#include "coreplugintr.h"
#include "credentialquery.h"

#include <qtkeychain/keychain.h>

#include <QtTaskTree/QTaskTree>
#include <QtTaskTree/QParallelTaskTreeRunner>
#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/guardedcallback.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/passworddialog.h>
#include <utils/qtcsettings.h>
#include <utils/utilsicons.h>

#include <QIcon>
#include <QLabel>
#include <QPointer>

using namespace QKeychain;
using namespace QtTaskTree;
using namespace Utils;

namespace Core {

using ReadCallback = std::function<void(Utils::Result<QString>)>;

class SecretAspectPrivate
{
public:
    void callReadCallbacks(const Result<QString> &value)
    {
        for (const auto &callback : readCallbacks)
            callback(value);
        readCallbacks.clear();
    }

    // The keychain service and key default to the settings key split at the
    // last '.', for callers that do not set them explicitly.
    bool applyTo(CredentialQuery &op, const Key &settingsKey) const
    {
        if (!service.isEmpty() && !key.isEmpty()) {
            op.setService(service);
            op.setKey(key);
            return true;
        }
        QStringList keyParts = stringFromKey(settingsKey).split('.');
        if (keyParts.size() < 2)
            return false;
        op.setKey(keyParts.takeLast());
        op.setService(keyParts.join('.'));
        return true;
    }

    Key plaintextKey(const Key &settingsKey) const
    {
        if (!settingsKey.isEmpty())
            return settingsKey;
        return Utils::keyFromString(service + '.' + key);
    }

public:
    QSingleTaskTreeRunner readRunner;
    QSingleTaskTreeRunner writeRunner;
    bool wasFetchedFromSecretStorage = false;
    bool wasEdited = false;
    bool repeatWriting = false;
    std::vector<ReadCallback> readCallbacks;
    QString value;
    QString service;
    QString key;
};

SecretAspect::SecretAspect(AspectContainer *container)
    : Utils::BaseAspect(container)
    , d(new SecretAspectPrivate)
{}

SecretAspect::~SecretAspect() = default;

void SecretAspect::setService(const QString &service)
{
    d->service = service;
}

void SecretAspect::setKey(const QString &key)
{
    d->key = key;
}

void SecretAspect::readSecret(const std::function<void(Result<QString>)> &cb) const
{
    d->readCallbacks.push_back(cb);

    if (d->readRunner.isRunning())
        return;

    if (!QKeychain::isAvailable()) {
        qWarning() << "No Keychain available, reading from plaintext";
        QtcSettings &settings = Utils::userSettings();
        settings.beginGroup("Secrets");
        QVariant value = settings.value(d->plaintextKey(settingsKey()));
        settings.endGroup();

        d->callReadCallbacks(fromSettingsValue(value).toString());
        return;
    }

    const auto onGetCredentialSetup = [this](CredentialQuery &credential) {
        credential.setOperation(CredentialOperation::Get);
        if (!d->applyTo(credential, settingsKey()))
            return SetupResult::StopWithError;
        return SetupResult::Continue;
    };
    const auto onGetCredentialDone = [this](const CredentialQuery &credential, DoneWith result) {
        if (result == DoneWith::Success) {
            d->value = QString::fromUtf8(credential.data().value_or(QByteArray{}));
            d->wasFetchedFromSecretStorage = true;
            d->callReadCallbacks(d->value);
        } else {
            d->callReadCallbacks(make_unexpected(credential.errorString()));
        }
        return DoneResult::Success;
    };

    d->readRunner.start({CredentialQueryTask(onGetCredentialSetup, onGetCredentialDone)});
}

QString SecretAspect::warningThatNoSecretStorageIsAvailable()
{
    static QString warning
        = Tr::tr("Secret storage is not available! "
                 "Your values will be stored as plaintext in the settings!")
          + (HostOsInfo::isLinuxHost()
                 ? (" " + Tr::tr("You can install libsecret or KWallet to enable secret storage."))
                 : QString());
    return warning;
}

void SecretAspect::readSettings()
{
    readSecret([](const Result<QString> &) {});
}

void SecretAspect::writeSettings() const
{
    if (!d->wasEdited)
        return;

    if (!QKeychain::isAvailable()) {
        QtcSettings &settings = Utils::userSettings();
        settings.beginGroup("Secrets");
        settings.setValue(d->plaintextKey(settingsKey()), toSettingsValue(d->value));
        settings.endGroup();
        d->wasEdited = false;
        return;
    }

    d->repeatWriting = true;

    if (d->writeRunner.isRunning())
        return;

    const auto onSetCredentialSetup = [this](CredentialQuery &credential) {
        credential.setOperation(CredentialOperation::Set);
        credential.setData(d->value.toUtf8());

        if (!d->applyTo(credential, settingsKey()))
            return SetupResult::StopWithError;
        return SetupResult::Continue;
    };

    const auto onSetCredentialsDone = [this](const CredentialQuery &, DoneWith result) {
        if (result == DoneWith::Success)
            d->wasEdited = false;
        return DoneResult::Success;
    };

    const UntilIterator iterator([this](int) { return std::exchange(d->repeatWriting, false); });

    // clang-format off
    d->writeRunner.start(
        For (iterator) >> Do {
            CredentialQueryTask(onSetCredentialSetup, onSetCredentialsDone)
        }
    );
    // clang-format on
}

bool SecretAspect::isDirty() const
{
    return d->wasEdited;
}

void SecretAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto edit = createSubWidget<FancyLineEdit>();
    edit->setObjectName(stringFromKey(settingsKey()) + ".secret");
    edit->setEchoMode(QLineEdit::Password);
    auto showPasswordButton = createSubWidget<Utils::ShowPasswordButton>();
    // Keep read-only/disabled until we have retrieved the value.
    edit->setReadOnly(true);
    showPasswordButton->setEnabled(false);
    QLabel *warningLabel = nullptr;

    if (!QKeychain::isAvailable()) {
        warningLabel = new QLabel();
        warningLabel->setPixmap(Utils::Icons::WARNING.icon().pixmap(16, 16));
        warningLabel->setToolTip(warningThatNoSecretStorageIsAvailable());
        edit->setToolTip(warningThatNoSecretStorageIsAvailable());
    }

    requestValue(
        guardedCallback(edit, [edit, showPasswordButton](const Utils::Result<QString> &value) {
            if (!value) {
                edit->setPlaceholderText(value.error());
                return;
            }

            edit->setReadOnly(false);
            showPasswordButton->setEnabled(true);
            edit->setText(*value);
        }));

    connect(showPasswordButton, &ShowPasswordButton::toggled, edit, [showPasswordButton, edit] {
        edit->setEchoMode(showPasswordButton->isChecked() ? QLineEdit::Normal : QLineEdit::Password);
    });

    connect(edit, &FancyLineEdit::textChanged, this, [this](const QString &text) {
        d->value = text;
        d->wasEdited = true;
    });

    addLabeledItem(parent, Layouting::Row{Layouting::noMargin, edit, warningLabel, showPasswordButton}.emerge());
}

void SecretAspect::requestValue(
    const std::function<void(const Utils::Result<QString> &)> &callback) const
{
    if (d->wasEdited)
        callback(d->value);
    else if (d->wasFetchedFromSecretStorage)
        callback(d->value);
    else
        readSecret(callback);
}

void SecretAspect::setValue(const QString &value)
{
    d->value = value;
    d->wasEdited = true;
}

bool SecretAspect::isSecretStorageAvailable()
{
    return QKeychain::isAvailable();
}

void deleteSecret(const QString &service, const QString &key)
{
    // Written by writeSettings() when no keychain was available, possibly by an
    // earlier run, so remove it regardless of what is available now.
    QtcSettings &settings = Utils::userSettings();
    settings.beginGroup("Secrets");
    settings.remove(Utils::keyFromString(service + '.' + key));
    settings.endGroup();

    if (!QKeychain::isAvailable())
        return;

    const auto onDeleteSetup = [service, key](CredentialQuery &credential) {
        credential.setOperation(CredentialOperation::Delete);
        credential.setService(service);
        credential.setKey(key);
    };
    // Outlives the caller, which is typically going away right now.
    static QParallelTaskTreeRunner runner;
    runner.start({CredentialQueryTask(onDeleteSetup)});
}

} // namespace Core
