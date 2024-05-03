// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidsdkmanager.h"

#include <utils/qtcprocess.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLineEdit;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class OutputFormatter; }

namespace Android::Internal {

class AndroidSdkManager;
class AndroidSdkModel;

class OptionsDialog : public QDialog
{
public:
    OptionsDialog(AndroidSdkManager *sdkManager, const QStringList &args, QWidget *parent = nullptr);

    QStringList sdkManagerArguments() const;

private:
    QPlainTextEdit *m_argumentDetailsEdit = nullptr;
    QLineEdit *m_argumentsEdit = nullptr;
    Utils::Process m_process;
};

class AndroidSdkManagerDialog : public QDialog
{
public:
    AndroidSdkManagerDialog(AndroidSdkManager *sdkManager, QWidget *parent = nullptr);

private:
    AndroidSdkManager *m_sdkManager = nullptr;
    AndroidSdkModel *m_sdkModel = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};

} // Android::Internal
