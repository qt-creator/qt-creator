// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace Fossil {
namespace Internal {

namespace Ui { class PullOrPushDialog; }

class PullOrPushDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        PullMode,
        PushMode
    };

    explicit PullOrPushDialog(Mode mode, QWidget *parent = nullptr);
    ~PullOrPushDialog() final;

    // Common parameters and options
    QString remoteLocation() const;
    bool isRememberOptionEnabled() const;
    bool isPrivateOptionEnabled() const;
    void setDefaultRemoteLocation(const QString &url);
    void setLocalBaseDirectory(const QString &dir);
    // Pull-specific options
    // Push-specific options

protected:
    void changeEvent(QEvent *e) final;

private:
    Mode m_mode;
    Ui::PullOrPushDialog *m_ui;
};

} // namespace Internal
} // namespace Fossil
