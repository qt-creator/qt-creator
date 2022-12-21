// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "clangdiagnosticconfig.h"

#include <QVector>

namespace CppEditor {

class CPPEDITOR_EXPORT ClangDiagnosticConfigsModel
{
public:
    ClangDiagnosticConfigsModel() = default;
    explicit ClangDiagnosticConfigsModel(const ClangDiagnosticConfigs &configs);

    int size() const;
    const ClangDiagnosticConfig &at(int index) const;

    void appendOrUpdate(const ClangDiagnosticConfig &config);
    void removeConfigWithId(const Utils::Id &id);

    ClangDiagnosticConfigs allConfigs() const;
    ClangDiagnosticConfigs customConfigs() const;

    bool hasConfigWithId(const Utils::Id &id) const;
    const ClangDiagnosticConfig &configWithId(const Utils::Id &id) const;
    int indexOfConfig(const Utils::Id &id) const;

    static ClangDiagnosticConfig createCustomConfig(const ClangDiagnosticConfig &baseConfig,
                                                    const QString &displayName);
    static QStringList globalDiagnosticOptions();

private:
    ClangDiagnosticConfigs m_diagnosticConfigs;
};

} // namespace CppEditor
