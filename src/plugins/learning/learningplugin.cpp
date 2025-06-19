// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "overviewwelcomepage.h"
#include "qtacademywelcomepage.h"

#include <extensionsystem/iplugin.h>

namespace Learning::Internal {

class LearningPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Learning.json")

public:
    void initialize() final
    {
        setupQtAcademyWelcomePage(this);
        setupOverviewWelcomePage(this);
#ifdef WITH_TESTS
        addTest<LearningTest>();
#endif // WITH_TESTS
    }
};

} // namespace Learning::Internal

#include "learningplugin.moc"
