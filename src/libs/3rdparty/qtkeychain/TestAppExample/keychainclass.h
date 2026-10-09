#ifndef KEYCHAINCLASS_H
#define KEYCHAINCLASS_H

#include <QObject>

#include <qtkeychain/keychain.h>

class KeyChainClass : public QObject
{
    Q_OBJECT
public:
    KeyChainClass(QObject *parent = nullptr);

    Q_INVOKABLE void readKey(const QString &key);
    Q_INVOKABLE void writeKey(const QString &key, const QString &value);
    Q_INVOKABLE void deleteKey(const QString &key);

Q_SIGNALS:
    void keyStored(const QString &key);
    void keyRestored(const QString &key, const QString &value);
    void keyDeleted(const QString &key);
    void error(const QString &errorText);

private:
};

#endif // KEYCHAINCLASS_H
