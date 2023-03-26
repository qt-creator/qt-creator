// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

namespace Utils { class InfoLabel; }

namespace Android::Internal {

class AndroidCreateKeystoreCertificate : public QDialog
{
    Q_OBJECT
    enum PasswordStatus
    {
        Invalid,
        NoMatch,
        Match
    };

public:
    explicit AndroidCreateKeystoreCertificate(QWidget *parent = nullptr);
    ~AndroidCreateKeystoreCertificate() override;

    Utils::FilePath keystoreFilePath() const;
    QString keystorePassword() const;
    QString certificateAlias() const;
    QString certificatePassword() const;

private:
    PasswordStatus checkKeystorePassword();
    PasswordStatus checkCertificatePassword();
    bool checkCertificateAlias();
    bool checkCountryCode();

    void keystoreShowPassStateChanged(int state);
    void certificateShowPassStateChanged(int state);
    void buttonBoxAccepted();
    void samePasswordStateChanged(int state);

    bool validateUserInput();

    Utils::FilePath m_keystoreFilePath;

    QLineEdit *m_commonNameLineEdit;
    QLineEdit *m_organizationUnitLineEdit;
    QLineEdit *m_organizationNameLineEdit;
    QLineEdit *m_localityNameLineEdit;
    QLineEdit *m_stateNameLineEdit;
    QLineEdit *m_countryLineEdit;
    QLineEdit *m_certificateRetypePassLineEdit;
    QCheckBox *m_certificateShowPassCheckBox;
    QSpinBox *m_validitySpinBox;
    QLineEdit *m_certificateAliasLineEdit;
    QLineEdit *m_certificatePassLineEdit;
    QSpinBox *m_keySizeSpinBox;
    QCheckBox *m_samePasswordCheckBox;
    QLineEdit *m_keystorePassLineEdit;
    QLineEdit *m_keystoreRetypePassLineEdit;
    Utils::InfoLabel *m_infoLabel;
};

} // Android::Internal
