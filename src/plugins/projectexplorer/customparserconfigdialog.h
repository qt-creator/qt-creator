// Copyright (C) 2016 Andre Hartmann.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "customparser.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

namespace ProjectExplorer::Internal {

class CustomParserConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomParserConfigDialog(QWidget *parent = nullptr);
    ~CustomParserConfigDialog() override;

    void setExampleSettings();
    void setSettings(const CustomParserSettings &settings);
    CustomParserSettings settings() const;

    void setErrorPattern(const QString &errorPattern);
    QString errorPattern() const;
    void setErrorFileNameCap(int errorFileNameCap);
    int errorFileNameCap() const;
    void setErrorLineNumberCap(int errorLineNumberCap);
    int errorLineNumberCap() const;
    void setErrorMessageCap(int errorMessageCap);
    int errorMessageCap() const;
    void setErrorChannel(CustomParserExpression::CustomParserChannel errorChannel);
    CustomParserExpression::CustomParserChannel errorChannel() const;
    void setErrorExample(const QString &errorExample);
    QString errorExample() const;

    void setWarningPattern(const QString &warningPattern);
    QString warningPattern() const;
    void setWarningFileNameCap(int warningFileNameCap);
    int warningFileNameCap() const;
    void setWarningLineNumberCap(int warningLineNumberCap);
    int warningLineNumberCap() const;
    void setWarningMessageCap(int warningMessageCap);
    int warningMessageCap() const;
    void setWarningChannel(CustomParserExpression::CustomParserChannel warningChannel);
    CustomParserExpression::CustomParserChannel warningChannel() const;
    void setWarningExample(const QString &warningExample);
    QString warningExample() const;

    bool isDirty() const;

private:
    void changed();

    bool checkPattern(QLineEdit *pattern, const QString &outputText,
                      QString *errorMessage, QRegularExpressionMatch *match);

    QLineEdit *m_errorPattern;
    QSpinBox *m_errorFileNameCap;
    QSpinBox *m_errorLineNumberCap;
    QSpinBox *m_errorMessageCap;
    QCheckBox *m_errorStdOutChannel;
    QCheckBox *m_errorStdErrChannel;
    QLineEdit *m_errorOutputMessage;
    QLabel *m_errorFileNameTest;
    QLabel *m_errorLineNumberTest;
    QLabel *m_errorMessageTest;

    QLineEdit *m_warningPattern;
    QSpinBox *m_warningLineNumberCap;
    QSpinBox *m_warningMessageCap;
    QSpinBox *m_warningFileNameCap;
    QCheckBox *m_warningStdOutChannel;
    QCheckBox *m_warningStdErrChannel;
    QLineEdit *m_warningOutputMessage;
    QLabel *m_warningFileNameTest;
    QLabel *m_warningLineNumberTest;
    QLabel *m_warningMessageTest;

    bool m_dirty;
};

} // ProjectExplorer::Internal
