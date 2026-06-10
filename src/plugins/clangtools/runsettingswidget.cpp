// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runsettingswidget.h"

#include "clangtoolsconstants.h"
#include "clangtoolssettings.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"
#include "diagnosticconfigswidget.h"
#include "executableinfo.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <cppeditor/clangdiagnosticconfigswidget.h>

#include <utils/filepath.h>

using namespace CppEditor;
using namespace Utils;

namespace ClangTools::Internal {

class ClangToolsOptionsPage final : public Core::IOptionsPage
{
public:
    ClangToolsOptionsPage()
    {
        setId(Constants::SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("Clang Tools"));
        setCategory("T.Analyzer");
        setSettingsProvider([] { return ClangToolsSettings::instance(); });
    }
};

void setupClangToolsOptionsPage()
{
    static ClangToolsOptionsPage theClangToolsOptionsPage;
    ClangToolsSettings *s = ClangToolsSettings::instance();
    s->diagnosticConfigId.setEditWidgetFactory(
        [s](const ClangDiagnosticConfigs &configs, const Id &id)
            -> ClangDiagnosticConfigsWidget * {
            FilePath tidy = FilePath::fromUserInput(s->clangTidyExecutable.volatileValue());
            tidy = tidy.isEmpty() ? toolFallbackExecutable(ClangToolType::Tidy) : fullPath(tidy);
            FilePath clazy = FilePath::fromUserInput(s->clazyStandaloneExecutable.volatileValue());
            clazy = clazy.isEmpty() ? toolFallbackExecutable(ClangToolType::Clazy) : fullPath(clazy);
            return new DiagnosticConfigsWidget(configs, id,
                                               ClangTidyInfo(tidy),
                                               ClazyStandaloneInfo(clazy));
        });
}

} // namespace ClangTools::Internal
