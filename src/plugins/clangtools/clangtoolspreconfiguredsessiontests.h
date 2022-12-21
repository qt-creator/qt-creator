// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QString>

namespace ProjectExplorer {
class Project;
class Target;
}

namespace ClangTools {
namespace Internal {

class PreconfiguredSessionTests: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testPreconfiguredSession();
    void testPreconfiguredSession_data();

private:
    bool switchToProjectAndTarget(ProjectExplorer::Project *project,
                                  ProjectExplorer::Target *target);
};

} // namespace Internal
} // namespace ClangTools
