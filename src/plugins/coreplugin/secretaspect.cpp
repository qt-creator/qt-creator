// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "secretaspect.h"

#include "coreplugintr.h"
#include "credentialquery.h"

#include <qtkeychain/keychain.h>

#include <tasking/tasktree.h>
#include <tasking/tasktreerunner.h>

#include <utils/guardedcallback.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/passworddialog.h>
#include <utils/utilsicons.h>

#include <QIcon>
#include <QPointer>

using namespace QKeychain;
using namespace Tasking;
using namespace Utils;

namespace Core {

using ReadCallback = std::function<void(Utils::expected_str<QString>)>;

class SecretAspectPrivate
{
public:
    void callReadCallbacks(const expected_str<QString> &value)
    {
        for (const auto &callback : readCallbacks)
            callback(value);
        readCallbacks.clear();
    }

public:
    TaskTreeRunner readRunner;
    TaskTreeRunner writeRunner;
    bool wasFetchedFromSecretStorage = false;
    bool wasEdited = false;
    bool repeatWriting = false;
    std::vector<ReadCallback> readCallbacks;
    QString value;
};

SecretAspect::SecretAspect(AspectContainer *container)
    : Utils::BaseAspect(container)
    , d(new SecretAspectPrivate)
{}

SecretAspect::~SecretAspect() = default;

static bool applyKey(const SecretAspect &aspect, CredentialQuery &op)
{
    QStringList keyParts = stringFromKey(aspect.settingsKey()).split('.');
    if (keyParts.size() < 2)
        return false;
    op.setKey(keyParts.takeLast());
    op.setService(keyParts.join('.'));
    return true;
}

void SecretAspect::readSecret(const std::function<void(Utils::expected_str<QString>)> &cb) const
{
    d->readCallbacks.push_back(cb);

    if (d->readRunner.isRunning())
        return;

    if (!QKeychain::isAvailable()) {
        qWarning() << "No Keychain available, reading from plaintext";
        qtcSettings()->beginGroup("Secrets");
        auto value = qtcSettings()->value(settingsKey());
        qtcSettings()->endGroup();

        d->callReadCallbacks(fromSettingsValue(value).toString());
        return;
    }

    const auto onGetCredentialSetup = [this](CredentialQuery &credential) {
        credential.setOperation(CredentialOperation::Get);
        if (!applyKey(*this, credential))
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

    d->readRunner.start({
        CredentialQueryTask(onGetCredentialSetup, onGetCredentialDone),
    });
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
    readSecret([](const expected_str<QString> &) {});
}

void SecretAspect::writeSettings() const
{
    if (!d->wasEdited)
        return;

    if (!QKeychain::isAvailable()) {
        qtcSettings()->beginGroup("Secrets");
        qtcSettings()->setValue(settingsKey(), toSettingsValue(d->value));
        qtcSettings()->endGroup();
        d->wasEdited = false;
        return;
    }

    d->repeatWriting = true;

    if (d->writeRunner.isRunning())
        return;

    const auto onSetCredentialSetup = [this](CredentialQuery &credential) {
        credential.setOperation(CredentialOperation::Set);
        credential.setData(d->value.toUtf8());

        if (!applyKey(*this, credential))
            return SetupResult::StopWithError;
        return SetupResult::Continue;
    };

    const auto onSetCredentialsDone = [this](const CredentialQuery &, DoneWith result) {
        if (result == DoneWith::Success)
            d->wasEdited = false;
        return DoneResult::Success;
    };

    const LoopUntil iterator([this](int) { return std::exchange(d->repeatWriting, false); });

    // clang-format off
    d->writeRunner.start({
        For(iterator) >> Do {
             CredentialQueryTask(onSetCredentialSetup, onSetCredentialsDone),
         }
    });
    // clang-format on
}

bool SecretAspect::isDirty()
{
    return d->wasEdited;
}

void SecretAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto edit = createSubWidget<FancyLineEdit>();
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
        guardedCallback(edit, [edit, showPasswordButton](const Utils::expected_str<QString> &value) {
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
    const std::function<void(const Utils::expected_str<QString> &)> &callback) const
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

} // namespace Core
