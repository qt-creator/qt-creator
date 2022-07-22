/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
