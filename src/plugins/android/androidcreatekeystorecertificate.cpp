// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidcreatekeystorecertificate.h"
#include "androidtr.h"

#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSpinBox>

using namespace Utils;

namespace Android::Internal {

AndroidCreateKeystoreCertificate::AndroidCreateKeystoreCertificate(QWidget *parent)
    : QDialog(parent)
{
    resize(638, 473);
    setWindowTitle(Tr::tr("Create a keystore and a certificate"));

    m_commonNameLineEdit = new QLineEdit;

    m_organizationUnitLineEdit = new QLineEdit;

    m_organizationNameLineEdit = new QLineEdit;

    m_localityNameLineEdit = new QLineEdit;

    m_stateNameLineEdit = new QLineEdit;

    m_countryLineEdit = new QLineEdit;
    m_countryLineEdit->setMaxLength(2);
    m_countryLineEdit->setInputMask(QString());

    m_certificateRetypePassLineEdit = new QLineEdit;
    m_certificateRetypePassLineEdit->setEchoMode(QLineEdit::Password);

    m_certificateShowPassCheckBox = new QCheckBox(Tr::tr("Show password"));

    m_validitySpinBox = new QSpinBox;
    m_validitySpinBox->setRange(10000, 100000);

    m_certificateAliasLineEdit = new QLineEdit;
    m_certificateAliasLineEdit->setInputMask({});
    m_certificateAliasLineEdit->setMaxLength(32);

    m_certificatePassLineEdit = new QLineEdit;
    m_certificatePassLineEdit->setEchoMode(QLineEdit::Password);

    m_keySizeSpinBox = new QSpinBox;
    m_keySizeSpinBox->setRange(2048, 2097152);

    m_samePasswordCheckBox = new QCheckBox(Tr::tr("Use Keystore password"));

    m_keystorePassLineEdit = new QLineEdit;
    m_keystorePassLineEdit->setEchoMode(QLineEdit::Password);

    m_keystoreRetypePassLineEdit = new QLineEdit;
    m_keystoreRetypePassLineEdit->setEchoMode(QLineEdit::Password);

    m_infoLabel = new InfoLabel;
    m_infoLabel->setType(InfoLabel::Error);
    m_infoLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_infoLabel->hide();

    auto keystoreShowPassCheckBox = new QCheckBox(Tr::tr("Show password"));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close|QDialogButtonBox::Save);

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("Keystore")),
            Form {
                Tr::tr("Password:"), m_keystorePassLineEdit, br,
                Tr::tr("Retype password:"), m_keystoreRetypePassLineEdit, br,
                Span(2, keystoreShowPassCheckBox), br,
            }
        },

        Group {
            title(Tr::tr("Certificate")),
            Form {
                Tr::tr("Alias name:"), m_certificateAliasLineEdit, br,
                Tr::tr("Keysize:"), m_keySizeSpinBox, br,
                Tr::tr("Validity (days):"), m_validitySpinBox, br,
                Tr::tr("Password:"), m_certificatePassLineEdit, br,
                Tr::tr("Retype password:"), m_certificateRetypePassLineEdit, br,
                Span(2, m_samePasswordCheckBox), br,
                Span(2, m_certificateShowPassCheckBox), br,
            }
        },

        Group {
            title(Tr::tr("Certificate Distinguished Names")),
            Form {
                Tr::tr("First and last name:"), m_commonNameLineEdit, br,
                Tr::tr("Organizational unit (e.g. Necessitas):"),  m_organizationUnitLineEdit, br,
                Tr::tr("Organization (e.g. KDE):"), m_organizationNameLineEdit, br,
                Tr::tr("City or locality:"), m_localityNameLineEdit, br,
                Tr::tr("State or province:"), m_stateNameLineEdit, br,
                Tr::tr("Two-letter country code for this unit (e.g. RO):"), m_countryLineEdit,
            }
        },

        Row { m_infoLabel, buttonBox }
    }.attachTo(this);

    connect(m_keystorePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkKeystorePassword);
    connect(m_keystoreRetypePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkKeystorePassword);
    connect(m_certificatePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCertificatePassword);
    connect(m_certificateRetypePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCertificatePassword);
    connect(m_certificateAliasLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCertificateAlias);
    connect(m_countryLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCountryCode);
    connect(keystoreShowPassCheckBox, &QCheckBox::stateChanged,
            this, &AndroidCreateKeystoreCertificate::keystoreShowPassStateChanged);
    connect(m_certificateShowPassCheckBox, &QCheckBox::stateChanged,
            this, &AndroidCreateKeystoreCertificate::certificateShowPassStateChanged);
    connect(m_samePasswordCheckBox, &QCheckBox::stateChanged,
            this, &AndroidCreateKeystoreCertificate::samePasswordStateChanged);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &AndroidCreateKeystoreCertificate::buttonBoxAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    connect(m_keystorePassLineEdit, &QLineEdit::editingFinished,
            m_keystoreRetypePassLineEdit, QOverload<>::of(&QWidget::setFocus));
}

AndroidCreateKeystoreCertificate::~AndroidCreateKeystoreCertificate() = default;

FilePath AndroidCreateKeystoreCertificate::keystoreFilePath() const
{
    return m_keystoreFilePath;
}

QString AndroidCreateKeystoreCertificate::keystorePassword() const
{
    return m_keystorePassLineEdit->text();
}

QString AndroidCreateKeystoreCertificate::certificateAlias() const
{
    return m_certificateAliasLineEdit->text();
}

QString AndroidCreateKeystoreCertificate::certificatePassword() const
{
    return (m_samePasswordCheckBox->checkState() == Qt::Checked)
            ? keystorePassword()
            : m_certificatePassLineEdit->text();
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkKeystorePassword()
{
    if (m_keystorePassLineEdit->text().length() < 6) {
        m_infoLabel->show();
        m_infoLabel->setText(Tr::tr("Keystore password is too short."));
        return Invalid;
    }
    if (m_keystorePassLineEdit->text() != m_keystoreRetypePassLineEdit->text()) {
        m_infoLabel->show();
        m_infoLabel->setText(Tr::tr("Keystore passwords do not match."));
        return NoMatch;
    }

    m_infoLabel->hide();
    return Match;
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkCertificatePassword()
{
    if (m_samePasswordCheckBox->checkState() == Qt::Checked)
        return Match;

    if (m_certificatePassLineEdit->text().length() < 6) {
        m_infoLabel->show();
        m_infoLabel->setText(Tr::tr("Certificate password is too short."));
        return Invalid;
    }
    if (m_certificatePassLineEdit->text() != m_certificateRetypePassLineEdit->text()) {
        m_infoLabel->show();
        m_infoLabel->setText(Tr::tr("Certificate passwords do not match."));
        return NoMatch;
    }

    m_infoLabel->hide();
    return Match;
}

bool AndroidCreateKeystoreCertificate::checkCertificateAlias()
{
    if (m_certificateAliasLineEdit->text().length() == 0) {
        m_infoLabel->show();
        m_infoLabel->setText(Tr::tr("Certificate alias is missing."));
        return false;
    }

    m_infoLabel->hide();
    return true;
}

bool AndroidCreateKeystoreCertificate::checkCountryCode()
{
    static const QRegularExpression re("[A-Z]{2}");
    if (!m_countryLineEdit->text().contains(re)) {
        m_infoLabel->show();
        m_infoLabel->setText(Tr::tr("Invalid country code."));
        return false;
    }

    m_infoLabel->hide();
    return true;
}

void AndroidCreateKeystoreCertificate::keystoreShowPassStateChanged(int state)
{
    m_keystorePassLineEdit->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
    m_keystoreRetypePassLineEdit->setEchoMode(m_keystorePassLineEdit->echoMode());
}

void AndroidCreateKeystoreCertificate::certificateShowPassStateChanged(int state)
{
    m_certificatePassLineEdit->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
    m_certificateRetypePassLineEdit->setEchoMode(m_certificatePassLineEdit->echoMode());
}

void AndroidCreateKeystoreCertificate::buttonBoxAccepted()
{
    if (!validateUserInput())
        return;

    m_keystoreFilePath = FileUtils::getSaveFilePath(this, Tr::tr("Keystore Filename"),
                                                    FileUtils::homePath() / "android_release.keystore",
                                                    Tr::tr("Keystore files (*.keystore *.jks)"));
    if (m_keystoreFilePath.isEmpty())
        return;
    QString distinguishedNames(QString::fromLatin1("CN=%1, O=%2, L=%3, C=%4")
                               .arg(m_commonNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(m_organizationNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(m_localityNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(m_countryLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,"))));

    if (!m_organizationUnitLineEdit->text().isEmpty())
        distinguishedNames += QLatin1String(", OU=") + m_organizationUnitLineEdit->text().replace(',', QLatin1String("\\,"));

    if (!m_stateNameLineEdit->text().isEmpty())
        distinguishedNames += QLatin1String(", S=") + m_stateNameLineEdit->text().replace(',', QLatin1String("\\,"));

    const CommandLine command(AndroidConfigurations::currentConfig().keytoolPath(),
                            { "-genkey", "-keyalg", "RSA",
                              "-keystore",  m_keystoreFilePath.toString(),
                              "-storepass", keystorePassword(),
                              "-alias", certificateAlias(),
                              "-keysize", m_keySizeSpinBox->text(),
                              "-validity", m_validitySpinBox->text(),
                              "-keypass", certificatePassword(),
                              "-dname", distinguishedNames});

    Process genKeyCertProc;
    genKeyCertProc.setTimeoutS(15);
    genKeyCertProc.setCommand(command);
    genKeyCertProc.runBlocking(EventLoopMode::On);

    if (genKeyCertProc.result() != ProcessResult::FinishedWithSuccess) {
        QMessageBox::critical(this, Tr::tr("Error"),
                              genKeyCertProc.exitMessage() + '\n' + genKeyCertProc.allOutput());
        return;
    }
    accept();
}

void AndroidCreateKeystoreCertificate::samePasswordStateChanged(int state)
{
    if (state == Qt::Checked) {
        m_certificatePassLineEdit->setDisabled(true);
        m_certificateRetypePassLineEdit->setDisabled(true);
        m_certificateShowPassCheckBox->setDisabled(true);
    }

    if (state == Qt::Unchecked) {
        m_certificatePassLineEdit->setEnabled(true);
        m_certificateRetypePassLineEdit->setEnabled(true);
        m_certificateShowPassCheckBox->setEnabled(true);
    }

    validateUserInput();
}

bool AndroidCreateKeystoreCertificate::validateUserInput()
{
    switch (checkKeystorePassword()) {
    case Invalid:
        m_keystorePassLineEdit->setFocus();
        return false;
    case NoMatch:
        m_keystoreRetypePassLineEdit->setFocus();
        return false;
    default:
        break;
    }

    if (!checkCertificateAlias()) {
        m_certificateAliasLineEdit->setFocus();
        return false;
    }

    switch (checkCertificatePassword()) {
    case Invalid:
        m_certificatePassLineEdit->setFocus();
        return false;
    case NoMatch:
        m_certificateRetypePassLineEdit->setFocus();
        return false;
    default:
        break;
    }

    if (!checkCountryCode()) {
        m_countryLineEdit->setFocus();
        return false;
    }

    return true;
}

} // Android::Internal
