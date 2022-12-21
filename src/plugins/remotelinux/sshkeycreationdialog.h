// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QPushButton;
class QRadioButton;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace RemoteLinux {

class SshKeyCreationDialog : public QDialog
{
    Q_OBJECT
public:
    SshKeyCreationDialog(QWidget *parent = nullptr);
    ~SshKeyCreationDialog();

    Utils::FilePath privateKeyFilePath() const;
    Utils::FilePath publicKeyFilePath() const;

private:
    void keyTypeChanged();
    void generateKeys();
    void handleBrowseButtonClicked();
    void setPrivateKeyFile(const Utils::FilePath &filePath);
    void showError(const QString &details);

private:
    QComboBox *m_comboBox;
    QLabel *m_privateKeyFileValueLabel;
    QLabel *m_publicKeyFileLabel;
    QPushButton *m_generateButton;
    QRadioButton *m_rsa;
    QRadioButton *m_ecdsa;
};

} // namespace RemoteLinux
