// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor { namespace Tests { class TemporaryCopiedDir; } }
namespace ProjectExplorer { class Kit; }

namespace Autotest {

class TestTreeModel;

namespace Internal {

class LoadProjectScenario : public QObject
{
    Q_OBJECT
public:
    explicit LoadProjectScenario(TestTreeModel *model, QObject *parent = nullptr);
    ~LoadProjectScenario();

    bool operator()();

private:
    bool init();
    bool loadProject();

    TestTreeModel *m_model = nullptr;
    CppEditor::Tests::TemporaryCopiedDir *m_tmpDir = nullptr;
    ProjectExplorer::Kit *m_kit = nullptr;
};

} // namespace Internal
} // namespace Autotest
