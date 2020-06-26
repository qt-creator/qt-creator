/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cpptools_global.h"

#include "clangdiagnosticconfig.h"

#include <QVector>

namespace CppTools {

class CPPTOOLS_EXPORT ClangDiagnosticConfigsModel
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

    static QVector<Utils::Id> changedOrRemovedConfigs(const ClangDiagnosticConfigs &oldConfigs,
                                                     const ClangDiagnosticConfigs &newConfigs);
    static ClangDiagnosticConfig createCustomConfig(const ClangDiagnosticConfig &baseConfig,
                                                    const QString &displayName);
    static QStringList globalDiagnosticOptions();

private:
    ClangDiagnosticConfigs m_diagnosticConfigs;
};

} // namespace CppTools
