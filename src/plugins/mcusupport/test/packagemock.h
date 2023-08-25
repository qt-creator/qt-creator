// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcuabstractpackage.h"

#include <gmock/gmock.h>

#include <utils/filepath.h>
#include <utils/storekey.h>

namespace McuSupport::Internal {

class PackageMock : public McuAbstractPackage
{
public:
    MOCK_METHOD(Utils::FilePath, basePath, (), (const));
    MOCK_METHOD(Utils::FilePath, path, (), (const));
    MOCK_METHOD(void, setPath, (const Utils::FilePath &) );
    MOCK_METHOD(QString, label, (), (const));
    MOCK_METHOD(Utils::FilePath, defaultPath, (), (const));
    MOCK_METHOD(Utils::FilePath, detectionPath, (), (const));
    MOCK_METHOD(QString, statusText, (), (const));
    MOCK_METHOD(void, updateStatus, ());
    MOCK_METHOD(Utils::Key, settingsKey, (), (const));

    MOCK_METHOD(Status, status, (), (const));
    MOCK_METHOD(bool, isValidStatus, (), (const));
    MOCK_METHOD(QString, cmakeVariableName, (), (const));
    MOCK_METHOD(QString, environmentVariableName, (), (const));
    MOCK_METHOD(bool, isAddToSystemPath, (), (const));
    MOCK_METHOD(bool, writeToSettings, (), (const));
    MOCK_METHOD(void, readFromSettings, ());
    MOCK_METHOD(QStringList, versions, (), (const));

    MOCK_METHOD(QWidget *, widget, ());
    MOCK_METHOD(const McuPackageVersionDetector *, getVersionDetector, (), (const));
};

} // namespace McuSupport::Internal
