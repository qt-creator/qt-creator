// Copyright (C) 2016 Andre Hartmann.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customparserconfigdialog.h"

#include "projectexplorertr.h"

#include <utils/layoutbuilder.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTabWidget>

namespace ProjectExplorer::Internal {

CustomParserConfigDialog::CustomParserConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    resize(516, 480);
    setWindowTitle(Tr::tr("Custom Parser"));

    m_errorPattern = new QLineEdit;
    auto label_1 = new QLabel(Tr::tr("&Error message capture pattern:"));
    label_1->setBuddy(m_errorPattern);

    auto label = new QLabel(Tr::tr("&File name:"));
    auto label_2 = new QLabel(Tr::tr("&Line number:"));
    auto label_3 = new QLabel(Tr::tr("&Message:"));

    m_errorFileNameCap = new QSpinBox;
    m_errorFileNameCap->setMaximum(9);
    m_errorFileNameCap->setValue(1);

    m_errorLineNumberCap = new QSpinBox;
    m_errorLineNumberCap->setMaximum(9);
    m_errorLineNumberCap->setValue(2);

    m_errorMessageCap = new QSpinBox;
    m_errorMessageCap->setMaximum(9);
    m_errorMessageCap->setValue(3);

    label->setBuddy(m_errorFileNameCap);
    label_2->setBuddy(m_errorLineNumberCap);
    label_3->setBuddy(m_errorMessageCap);

    m_errorStdOutChannel = new QCheckBox(Tr::tr("Standard output"));
    m_errorStdErrChannel = new QCheckBox(Tr::tr("Standard error"));

    auto label_5 = new QLabel(Tr::tr("E&rror message:"));
    m_errorOutputMessage = new QLineEdit;
    m_errorFileNameTest = new QLabel;
    m_errorLineNumberTest = new QLabel;
    m_errorMessageTest = new QLabel;
    label_5->setBuddy(m_errorOutputMessage);


    m_warningPattern = new QLineEdit;

    auto label_11 = new QLabel(Tr::tr("&File name:"));
    auto label_12 = new QLabel(Tr::tr("&Line number:"));
    auto label_13 = new QLabel(Tr::tr("&Message:"));

    m_warningLineNumberCap = new QSpinBox;
    m_warningLineNumberCap->setMaximum(9);
    m_warningLineNumberCap->setValue(2);

    m_warningMessageCap = new QSpinBox;
    m_warningMessageCap->setMaximum(9);
    m_warningMessageCap->setValue(3);

    m_warningFileNameCap = new QSpinBox;
    m_warningFileNameCap->setMaximum(9);
    m_warningFileNameCap->setValue(1);

    label_11->setBuddy(m_warningFileNameCap);
    label_12->setBuddy(m_warningLineNumberCap);
    label_13->setBuddy(m_warningMessageCap);

    m_warningStdOutChannel = new QCheckBox(Tr::tr("Standard output"));
    m_warningStdErrChannel = new QCheckBox(Tr::tr("Standard error"));

    auto label_14 = new QLabel(Tr::tr("Warning message:"));
    m_warningOutputMessage = new QLineEdit;
    m_warningFileNameTest = new QLabel;
    m_warningLineNumberTest = new QLabel;
    m_warningMessageTest = new QLabel;
    label_14->setBuddy(m_warningOutputMessage);


    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    auto tabWarning = new QWidget;
    Column {
        Tr::tr("Warning message capture pattern:"),
        m_warningPattern,
        Group {
            title(Tr::tr("Capture Positions")),
            Grid {
                label_11, label_12, label_13, br,
                m_warningFileNameCap, m_warningLineNumberCap,  m_warningMessageCap
            }
        },
        Group {
            title(Tr::tr("Capture Output Channels")),
            Row { m_warningStdOutChannel, m_warningStdErrChannel }
        },
        Group {
            title(Tr::tr("Test")),
            Column {
                label_14,
                m_warningOutputMessage,
                Form {
                    Tr::tr("File name:"), m_warningFileNameTest, br,
                    Tr::tr("Line number:"), m_warningLineNumberTest, br,
                    Tr::tr("Message:"), m_warningMessageTest
                }
            }
        }
    }.attachTo(tabWarning);

    auto tabError = new QWidget;
    Column {
        label_1,
        m_errorPattern,
        Group {
            title(Tr::tr("Capture Positions")),
            Grid {
                label, label_2, label_3, br,
                m_errorFileNameCap, m_errorLineNumberCap, m_errorMessageCap
            }
        },
        Group {
            title(Tr::tr("Capture Output Channels")),
            Row { m_errorStdOutChannel, m_errorStdErrChannel }
        },
        Group {
            title(Tr::tr("Test")),
            Column {
                label_5,
                m_errorOutputMessage,
                Form {
                    Tr::tr("File name:"), m_errorFileNameTest, br,
                    Tr::tr("Line number:"), m_errorLineNumberTest, br,
                    Tr::tr("Message:"), m_errorMessageTest
                }
            }
        },
    }.attachTo(tabError);

    auto tabWidget = new QTabWidget(this);
    tabWidget->addTab(tabError, Tr::tr("Error"));
    tabWidget->addTab(tabWarning, Tr::tr("Warning"));
    tabWidget->setCurrentIndex(0);

    Column {
        tabWidget,
        st,
        buttonBox
    }.attachTo(this);

    connect(m_errorPattern, &QLineEdit::textChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_errorOutputMessage, &QLineEdit::textChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_errorFileNameCap, &QSpinBox::valueChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_errorLineNumberCap, &QSpinBox::valueChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_errorMessageCap, &QSpinBox::valueChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_warningPattern, &QLineEdit::textChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_warningOutputMessage, &QLineEdit::textChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_warningFileNameCap, &QSpinBox::valueChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_warningLineNumberCap, &QSpinBox::valueChanged,
            this, &CustomParserConfigDialog::changed);
    connect(m_warningMessageCap, &QSpinBox::valueChanged,
            this, &CustomParserConfigDialog::changed);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    changed();
    m_dirty = false;
}

CustomParserConfigDialog::~CustomParserConfigDialog() = default;

void CustomParserConfigDialog::setExampleSettings()
{
    setErrorPattern(QLatin1String("#error (.*):(\\d+): (.*)"));
    setErrorFileNameCap(1);
    setErrorLineNumberCap(2);
    setErrorMessageCap(3);
    setErrorChannel(CustomParserExpression::ParseBothChannels);
    setWarningPattern(QLatin1String("#warning (.*):(\\d+): (.*)"));
    setWarningFileNameCap(1);
    setWarningLineNumberCap(2);
    setWarningMessageCap(3);
    setWarningChannel(CustomParserExpression::ParseBothChannels);
    m_errorOutputMessage->setText(
                QLatin1String("#error /home/user/src/test.c:891: Unknown identifier `test`"));
    m_warningOutputMessage->setText(
                QLatin1String("#warning /home/user/src/test.c:49: Unreferenced variable `test`"));
}

void CustomParserConfigDialog::setSettings(const CustomParserSettings &settings)
{
    if (settings.error.pattern().isEmpty() && settings.warning.pattern().isEmpty()) {
        setExampleSettings();
        return;
    }

    setErrorPattern(settings.error.pattern());
    setErrorFileNameCap(settings.error.fileNameCap());
    setErrorLineNumberCap(settings.error.lineNumberCap());
    setErrorMessageCap(settings.error.messageCap());
    setErrorChannel(settings.error.channel());
    setErrorExample(settings.error.example());
    setWarningPattern(settings.warning.pattern());
    setWarningFileNameCap(settings.warning.fileNameCap());
    setWarningLineNumberCap(settings.warning.lineNumberCap());
    setWarningMessageCap(settings.warning.messageCap());
    setWarningChannel(settings.warning.channel());
    setWarningExample(settings.warning.example());
}

CustomParserSettings CustomParserConfigDialog::settings() const
{
    CustomParserSettings result;
    result.error.setPattern(errorPattern());
    result.error.setFileNameCap(errorFileNameCap());
    result.error.setLineNumberCap(errorLineNumberCap());
    result.error.setMessageCap(errorMessageCap());
    result.error.setChannel(errorChannel());
    result.error.setExample(errorExample());
    result.warning.setPattern(warningPattern());
    result.warning.setFileNameCap(warningFileNameCap());
    result.warning.setLineNumberCap(warningLineNumberCap());
    result.warning.setMessageCap(warningMessageCap());
    result.warning.setChannel(warningChannel());
    result.warning.setExample(warningExample());
    return result;
}

void CustomParserConfigDialog::setErrorPattern(const QString &errorPattern)
{
    m_errorPattern->setText(errorPattern);
}

QString CustomParserConfigDialog::errorPattern() const
{
    return m_errorPattern->text();
}

void CustomParserConfigDialog::setErrorFileNameCap(int fileNameCap)
{
    m_errorFileNameCap->setValue(fileNameCap);
}

int CustomParserConfigDialog::errorFileNameCap() const
{
    return m_errorFileNameCap->value();
}

void CustomParserConfigDialog::setErrorLineNumberCap(int lineNumberCap)
{
    m_errorLineNumberCap->setValue(lineNumberCap);
}

int CustomParserConfigDialog::errorLineNumberCap() const
{
    return m_errorLineNumberCap->value();
}

void CustomParserConfigDialog::setErrorMessageCap(int messageCap)
{
    m_errorMessageCap->setValue(messageCap);
}

int CustomParserConfigDialog::errorMessageCap() const
{
    return m_errorMessageCap->value();
}

void CustomParserConfigDialog::setErrorChannel(CustomParserExpression::CustomParserChannel errorChannel)
{
    m_errorStdErrChannel->setChecked(
                errorChannel & static_cast<int>(CustomParserExpression::ParseStdErrChannel));
    m_errorStdOutChannel->setChecked(
                errorChannel & static_cast<int>(CustomParserExpression::ParseStdOutChannel));
}

CustomParserExpression::CustomParserChannel CustomParserConfigDialog::errorChannel() const
{
    if (m_errorStdErrChannel->isChecked() && !m_errorStdOutChannel->isChecked())
        return CustomParserExpression::ParseStdErrChannel;
    if (m_errorStdOutChannel->isChecked() && !m_errorStdErrChannel->isChecked())
        return CustomParserExpression::ParseStdOutChannel;
    return CustomParserExpression::ParseBothChannels;
}

void CustomParserConfigDialog::setErrorExample(const QString &errorExample)
{
    m_errorOutputMessage->setText(errorExample);
}

QString CustomParserConfigDialog::errorExample() const
{
    return m_errorOutputMessage->text();
}

void CustomParserConfigDialog::setWarningPattern(const QString &warningPattern)
{
    m_warningPattern->setText(warningPattern);
}

QString CustomParserConfigDialog::warningPattern() const
{
    return m_warningPattern->text();
}

void CustomParserConfigDialog::setWarningFileNameCap(int warningFileNameCap)
{
    m_warningFileNameCap->setValue(warningFileNameCap);
}

int CustomParserConfigDialog::warningFileNameCap() const
{
    return m_warningFileNameCap->value();
}

void CustomParserConfigDialog::setWarningLineNumberCap(int warningLineNumberCap)
{
    m_warningLineNumberCap->setValue(warningLineNumberCap);
}

int CustomParserConfigDialog::warningLineNumberCap() const
{
    return m_warningLineNumberCap->value();
}

void CustomParserConfigDialog::setWarningMessageCap(int warningMessageCap)
{
    m_warningMessageCap->setValue(warningMessageCap);
}

int CustomParserConfigDialog::warningMessageCap() const
{
    return m_warningMessageCap->value();
}

void CustomParserConfigDialog::setWarningChannel(CustomParserExpression::CustomParserChannel warningChannel)
{
    m_warningStdErrChannel->setChecked(
                warningChannel & static_cast<int>(CustomParserExpression::ParseStdErrChannel));
    m_warningStdOutChannel->setChecked(
                warningChannel & static_cast<int>(CustomParserExpression::ParseStdOutChannel));
}

CustomParserExpression::CustomParserChannel CustomParserConfigDialog::warningChannel() const
{
    if (m_warningStdErrChannel->isChecked() && !m_warningStdOutChannel->isChecked())
        return CustomParserExpression::ParseStdErrChannel;
    if (m_warningStdOutChannel->isChecked() && !m_warningStdErrChannel->isChecked())
        return CustomParserExpression::ParseStdOutChannel;
    return CustomParserExpression::ParseBothChannels;
}

void CustomParserConfigDialog::setWarningExample(const QString &warningExample)
{
    m_warningOutputMessage->setText(warningExample);
}

QString CustomParserConfigDialog::warningExample() const
{
    return m_warningOutputMessage->text();
}

bool CustomParserConfigDialog::isDirty() const
{
    return m_dirty;
}

bool CustomParserConfigDialog::checkPattern(QLineEdit *pattern, const QString &outputText,
                                            QString *errorMessage, QRegularExpressionMatch *match)
{
    QRegularExpression rx;
    rx.setPattern(pattern->text());

    QPalette palette;
    palette.setColor(QPalette::Text,
                     Utils::creatorTheme()->color(rx.isValid() ? Utils::Theme::TextColorNormal
                                                               : Utils::Theme::TextColorError));
    pattern->setPalette(palette);
    pattern->setToolTip(rx.isValid() ? QString() : rx.errorString());

    if (rx.isValid())
        *match = rx.match(outputText);
    if (rx.pattern().isEmpty() || !rx.isValid() || !match->hasMatch()) {
        *errorMessage = QString::fromLatin1("<font color=\"%1\">%2 ").arg(
                    Utils::creatorTheme()->color(Utils::Theme::TextColorError).name(),
                    Tr::tr("Not applicable:"));
        if (rx.pattern().isEmpty())
            *errorMessage += Tr::tr("Pattern is empty.");
        else if (!rx.isValid())
            *errorMessage += rx.errorString();
        else if (outputText.isEmpty())
            *errorMessage += Tr::tr("No message given.");
        else
            *errorMessage += Tr::tr("Pattern does not match the message.");

        return false;
    }

    errorMessage->clear();
    return true;
}

void CustomParserConfigDialog::changed()
{
    QRegularExpressionMatch match;
    QString errorMessage;

    if (checkPattern(m_errorPattern, m_errorOutputMessage->text(), &errorMessage, &match)) {
        m_errorFileNameTest->setText(match.captured(m_errorFileNameCap->value()));
        m_errorLineNumberTest->setText(match.captured(m_errorLineNumberCap->value()));
        m_errorMessageTest->setText(match.captured(m_errorMessageCap->value()));
    } else {
        m_errorFileNameTest->setText(errorMessage);
        m_errorLineNumberTest->setText(errorMessage);
        m_errorMessageTest->setText(errorMessage);
    }

    if (checkPattern(m_warningPattern, m_warningOutputMessage->text(), &errorMessage, &match)) {
        m_warningFileNameTest->setText(match.captured(m_warningFileNameCap->value()));
        m_warningLineNumberTest->setText(match.captured(m_warningLineNumberCap->value()));
        m_warningMessageTest->setText(match.captured(m_warningMessageCap->value()));
    } else {
        m_warningFileNameTest->setText(errorMessage);
        m_warningLineNumberTest->setText(errorMessage);
        m_warningMessageTest->setText(errorMessage);
    }
    m_dirty = true;
}

} // ProjectExplorer::Internal
