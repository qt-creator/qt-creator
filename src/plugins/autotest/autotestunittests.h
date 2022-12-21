// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor { namespace Tests { class TemporaryCopiedDir; } }
namespace ProjectExplorer { class Kit; }

namespace Autotest {

class TestTreeModel;

namespace Internal {

class AutoTestUnitTests : public QObject
{
    Q_OBJECT
public:
    explicit AutoTestUnitTests(TestTreeModel *model, QObject *parent = nullptr);

signals:

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCodeParser();
    void testCodeParser_data();
    void testCodeParserSwitchStartup();
    void testCodeParserSwitchStartup_data();
    void testCodeParserGTest();
    void testCodeParserGTest_data();
    void testCodeParserBoostTest();
    void testCodeParserBoostTest_data();
    void testModelManagerInterface();

private:
    TestTreeModel *m_model = nullptr;
    CppEditor::Tests::TemporaryCopiedDir *m_tmpDir = nullptr;
    bool m_isQt4 = false;
    bool m_checkBoost = false;
    ProjectExplorer::Kit *m_kit = nullptr;
};

} // namespace Internal
} // namespace Autotest
