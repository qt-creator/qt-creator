// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

#include <utils/pathchooser.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
QT_END_NAMESPACE

namespace Bazaar::Internal {

class PullOrPushDialog : public QDialog
{
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

private:
    Mode m_mode;

    QRadioButton *m_defaultButton;
    QRadioButton *m_localButton;
    Utils::PathChooser *m_localPathChooser;
    QLineEdit *m_urlLineEdit;
    QCheckBox *m_rememberCheckBox;
    QCheckBox *m_overwriteCheckBox;
    QCheckBox *m_useExistingDirCheckBox;
    QCheckBox *m_createPrefixCheckBox;
    QLineEdit *m_revisionLineEdit;
    QCheckBox *m_localCheckBox;
};

} // Bazaar::Internal
