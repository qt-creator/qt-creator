// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonwizardgeneratorfactory.h"

#include <QDir>
#include <QRegularExpression>
#include <QVariant>

namespace ProjectExplorer {
namespace Internal {

// Documentation inside.
class JsonWizardScannerGenerator : public JsonWizardGenerator
{
public:
    bool setup(const QVariant &data, QString *errorMessage);

    Core::GeneratedFiles fileList(Utils::MacroExpander *expander,
                                  const Utils::FilePath &wizardDir, const Utils::FilePath &projectDir,
                                  QString *errorMessage) override;
private:
    Core::GeneratedFiles scan(const Utils::FilePath &dir, const Utils::FilePath &base);
    bool matchesSubdirectoryPattern(const Utils::FilePath &path);

    QString m_binaryPattern;
    QList<QRegularExpression> m_subDirectoryExpressions;
};

} // namespace Internal
} // namespace ProjectExplorer
