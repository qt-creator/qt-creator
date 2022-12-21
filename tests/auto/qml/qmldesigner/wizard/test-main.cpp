// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "test-utilities.h"

#include <utils/temporarydirectory.h>

class Environment : public testing::Environment
{
public:
    void SetUp() override
    {
        const QString temporayDirectoryPath = QDir::tempPath() + "/QtCreator-UnitTests-XXXXXX";
        Utils::TemporaryDirectory::setMasterTemporaryDirectory(temporayDirectoryPath);
        qputenv("TMPDIR", Utils::TemporaryDirectory::masterDirectoryPath().toUtf8());
        qputenv("TEMP", Utils::TemporaryDirectory::masterDirectoryPath().toUtf8());
    }

    void TearDown() override {}
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    auto environment = std::make_unique<Environment>();
    testing::AddGlobalTestEnvironment(environment.release());

    return RUN_ALL_TESTS();
}

