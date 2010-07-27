#ifndef PASSPHRASEFORKEYDIALOG_H
#define PASSPHRASEFORKEYDIALOG_H

#include <QtGui/QDialog>

class QCheckBox;

namespace Qt4ProjectManager {

class PassphraseForKeyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PassphraseForKeyDialog(const QString &keyName, QWidget *parent = 0);

    QString passphrase() const;
    bool savePassphrase() const;

protected slots:
    void accept();
    void reject();
    void setPassphrase(const QString &passphrase);

private:
    QCheckBox *m_checkBox;

    QString m_passphrase;
};

} // namespace Qt4ProjectManager

#endif // PASSPHRASEFORKEYDIALOG_H
