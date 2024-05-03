// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidsdkmanager.h"

#include <QDialog>

namespace Android::Internal {

class AndroidSdkManager;
class AndroidSdkModel;

class AndroidSdkManagerDialog : public QDialog
{
public:
    AndroidSdkManagerDialog(AndroidSdkManager *sdkManager, QWidget *parent = nullptr);

private:
    AndroidSdkManager *m_sdkManager = nullptr;
    AndroidSdkModel *m_sdkModel = nullptr;
};

} // Android::Internal
