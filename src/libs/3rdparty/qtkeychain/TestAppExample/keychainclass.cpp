// fix for https://github.com/frankosterfeld/qtkeychain/issues/288

#include <QDebug>

#include "keychainclass.h"

static const QString service = "keychain.example.project.app";

KeyChainClass::KeyChainClass(QObject *parent)
    : QObject(parent)
{
    
}

void KeyChainClass::readKey(const QString &key)
{
    auto readCredentialJob = new QKeychain::ReadPasswordJob(service);
    readCredentialJob->setAutoDelete(true);
    readCredentialJob->setKey(key);

    QObject::connect(readCredentialJob, &QKeychain::ReadPasswordJob::finished, this,
                     [this, key](QKeychain::Job *job) {
                         auto j = static_cast<QKeychain::ReadPasswordJob*>(job);
                         if (j->error() == QKeychain::NoError) {
                             emit keyRestored(key, j->textData());
                         } else {
                             emit error(tr("Read key failed: %1").arg(qPrintable(j->errorString())));
                         }
                         // no delete needed, autoDelete takes care of it
                     });

    readCredentialJob->start();
}

void KeyChainClass::writeKey(const QString &key, const QString &value)
{
    auto writeCredentialJob = new QKeychain::WritePasswordJob(service);
    writeCredentialJob->setAutoDelete(true);
    writeCredentialJob->setKey(key);
    writeCredentialJob->setTextData(value);

    QObject::connect(writeCredentialJob, &QKeychain::WritePasswordJob::finished, this,
                     [this, key](QKeychain::Job *job) {
                         auto j = static_cast<QKeychain::WritePasswordJob*>(job);
                         if (j->error() == QKeychain::NoError) {
                             emit keyStored(key);
                         } else {
                             emit error(tr("Write key failed: %1").arg(qPrintable(j->errorString())));
                         }
                     });

    writeCredentialJob->start();
}

void KeyChainClass::deleteKey(const QString &key)
{
    auto deleteCredentialJob = new QKeychain::DeletePasswordJob(service);
    deleteCredentialJob->setAutoDelete(true);
    deleteCredentialJob->setKey(key);

    QObject::connect(deleteCredentialJob, &QKeychain::DeletePasswordJob::finished, this,
                     [this, key](QKeychain::Job *job) {
                         auto j = static_cast<QKeychain::DeletePasswordJob*>(job);
                         if (j->error() == QKeychain::NoError) {
                             emit keyDeleted(key);
                         } else {
                             emit error(tr("Delete key failed: %1").arg(qPrintable(j->errorString())));
                         }
                     });

    deleteCredentialJob->start();
}
