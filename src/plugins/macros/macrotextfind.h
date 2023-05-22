// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/find/ifindsupport.h>

#include <QPointer>

namespace Macros {
namespace Internal {

class MacroTextFind : public Core::IFindSupport
{
    Q_OBJECT

public:
    MacroTextFind(Core::IFindSupport *currentFind);

    bool supportsReplace() const override;
    Utils::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override;
    void clearHighlights() override;
    QString currentFindString() const override;
    QString completedFindString() const override;

    void highlightAll(const QString &txt, Utils::FindFlags findFlags) override;
    Core::IFindSupport::Result findIncremental(const QString &txt, Utils::FindFlags findFlags) override;
    Core::IFindSupport::Result findStep(const QString &txt, Utils::FindFlags findFlags) override;
    void replace(const QString &before, const QString &after, Utils::FindFlags findFlags) override;
    bool replaceStep(const QString &before, const QString &after, Utils::FindFlags findFlags) override;
    int replaceAll(const QString &before, const QString &after, Utils::FindFlags findFlags) override;

    void defineFindScope() override;
    void clearFindScope() override;

signals:
    void incrementalSearchReseted();
    void incrementalFound(const QString &txt, Utils::FindFlags findFlags);
    void stepFound(const QString &txt, Utils::FindFlags findFlags);
    void replaced(const QString &before, const QString &after, Utils::FindFlags findFlags);
    void stepReplaced(const QString &before, const QString &after, Utils::FindFlags findFlags);
    void allReplaced(const QString &before, const QString &after, Utils::FindFlags findFlags);

private:
    QPointer<Core::IFindSupport> m_currentFind;
};

} // namespace Internal
} // namespace Macros
