// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
                                  const QString &wizardDir, const QString &projectDir,
                                  QString *errorMessage) override;
private:
    Core::GeneratedFiles scan(const QString &dir, const QDir &base);
    bool matchesSubdirectoryPattern(const QString &path);

    QString m_binaryPattern;
    QList<QRegularExpression> m_subDirectoryExpressions;
};

} // namespace Internal
} // namespace ProjectExplorer
