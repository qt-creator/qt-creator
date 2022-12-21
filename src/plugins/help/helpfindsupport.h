// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/find/ifindsupport.h>

namespace Help {
namespace Internal {

class HelpViewer;

class HelpViewerFindSupport : public Core::IFindSupport
{
    Q_OBJECT
public:
    HelpViewerFindSupport(HelpViewer *viewer);

    bool supportsReplace() const override { return false; }
    Core::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override {}
    void clearHighlights() override {}
    QString currentFindString() const override;
    QString completedFindString() const override { return QString(); }

    Result findIncremental(const QString &txt, Core::FindFlags findFlags) override;
    Result findStep(const QString &txt, Core::FindFlags findFlags) override;

private:
    bool find(const QString &ttf, Core::FindFlags findFlags, bool incremental);
    HelpViewer *m_viewer;
};

} // namespace Internal
} // namespace Help
