// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangutils.h"

#include <extensionsystem/iplugin.h>

#include <utils/parameteraction.h>

#include <QFutureWatcher>

namespace ClangCodeModel {
namespace Internal {

class ClangCodeModelPlugin final: public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClangCodeModel.json")

public:
    ~ClangCodeModelPlugin() override;
    bool initialize(const QStringList &arguments, QString *errorMessage) override;

private:
    void maybeHandleBatchFileAndExit() const;

private:
    void generateCompilationDB();
    void createCompilationDBAction();

    Utils::ParameterAction *m_generateCompilationDBAction = nullptr;
    QFutureWatcher<GenerateCompilationDbResult> m_generatorWatcher;
#ifdef WITH_TESTS
    QVector<QObject *> createTestObjects() const override;
#endif
};

} // namespace Internal
} // namespace Clang
