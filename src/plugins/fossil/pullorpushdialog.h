// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace Fossil {
namespace Internal {

class PullOrPushDialog : public QDialog
{
    Q_OBJECT

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
    void setLocalBaseDirectory(const QString &dir);
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

} // namespace Internal
} // namespace Fossil
