// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace Fossil::Internal {

class PullOrPushDialog : public QDialog
{
public:
    enum Mode {
        PullMode,
        PushMode
    };

    explicit PullOrPushDialog(Mode mode, QWidget *parent = nullptr);

    // Common parameters and options
    QString remoteLocation() const;
    bool isRememberOptionEnabled() const;
    bool isPrivateOptionEnabled() const;
    void setDefaultRemoteLocation(const QString &url);
    void setLocalBaseDirectory(const Utils::FilePath &dir);
    // Pull-specific options
    // Push-specific options

private:
    QRadioButton *m_defaultButton;
    QRadioButton *m_localButton;
    Utils::PathChooser *m_localPathChooser;
    QRadioButton *m_urlButton;
    QLineEdit *m_urlLineEdit;
    QCheckBox *m_rememberCheckBox;
    QCheckBox *m_privateCheckBox;
};

} // Fossil::Internal
