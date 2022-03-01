/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "mcuabstractpackage.h"

#include <gmock/gmock.h>
#include <utils/filepath.h>

namespace McuSupport::Internal {

class PackageMock : public McuAbstractPackage
{
public:
    MOCK_METHOD(Utils::FilePath, basePath, (), (const));
    MOCK_METHOD(Utils::FilePath, path, (), (const));
    MOCK_METHOD(QString, label, (), (const));
    MOCK_METHOD(Utils::FilePath, defaultPath, (), (const));
    MOCK_METHOD(Utils::FilePath, detectionPath, (), (const));
    MOCK_METHOD(QString, statusText, (), (const));
    MOCK_METHOD(void, updateStatus, ());
    MOCK_METHOD(QString, settingsKey, (), (const));

    MOCK_METHOD(Status, status, (), (const));
    MOCK_METHOD(bool, isValidStatus, (), (const));
    MOCK_METHOD(const QString &, environmentVariableName, (), (const));
    MOCK_METHOD(bool, isAddToSystemPath, (), (const));
    MOCK_METHOD(bool, writeToSettings, (), (const));
    MOCK_METHOD(void, setVersions, (const QStringList &) );

    MOCK_METHOD(QWidget *, widget, ());
}; // class PackageMock
} // namespace McuSupport::Internal
