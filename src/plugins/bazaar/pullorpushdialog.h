// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace Bazaar {
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
    ~PullOrPushDialog() override;

    // Common parameters and options
    QString branchLocation() const;
    bool isRememberOptionEnabled() const;
    bool isOverwriteOptionEnabled() const;
    QString revision() const;

    // Pull-specific options
    bool isLocalOptionEnabled() const;

    // Push-specific options
    bool isUseExistingDirectoryOptionEnabled() const;
    bool isCreatePrefixOptionEnabled() const;

protected:
    void changeEvent(QEvent *e) override;

private:
    Mode m_mode;
    Ui::PullOrPushDialog *m_ui;
};

} // namespace Internal
} // namespace Bazaar
