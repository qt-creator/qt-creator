// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Python::Internal {

class PyProjectTomlTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCorrectPyProjectParsing();
    void testNonExistingPyProjectParsing();
    void testEmptyPyProjectParsing();
    void testFilesBlankPyProjectParsing();
    void testFilesWrongTypePyProjectParsing();
    void testFileWrongTypePyProjectParsing();
    void testFileMissingPyProjectParsing();
    void testProjectEmptyPyProjectParsing();
    void testProjectMissingPyProjectParsing();
    void testProjectWrongTypePyProjectParsing();
    void testPySideEmptyPyProjectParsing();
    void testPySideMissingPyProjectParsing();
    void testPySideWrongTypePyProjectParsing();
    void testToolEmptyPyProjectParsing();
    void testToolMissingPyProjectParsing();
    void testToolWrongTypePyProjectParsing();

    void testUpdatePyProject();
};

QObject *createPyProjectTomlTest();

} // namespace Python::Internal
