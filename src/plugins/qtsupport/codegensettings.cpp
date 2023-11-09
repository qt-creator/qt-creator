// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegensettings.h"

#include "qtsupportconstants.h"
#include "qtsupporttr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditortr.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace QtSupport {

CodeGenSettings &codeGenSettings()
{
    static CodeGenSettings theCodeGenSettings;
    return theCodeGenSettings;
}

CodeGenSettings::CodeGenSettings()
{
    setSettingsGroup("FormClassWizardPage");

    embedding.setSettingsKey("Embedding");
    embedding.addOption(Tr::tr("Aggregation as a pointer member"));
    embedding.addOption(Tr::tr("Aggregation"));
    embedding.addOption(Tr::tr("Multiple inheritance"));
    embedding.setDefaultValue(CodeGenSettings::PointerAggregatedUiClass);

    retranslationSupport.setSettingsKey("RetranslationSupport");
    retranslationSupport.setLabelText(Tr::tr("Support for changing languages at runtime"));

    includeQtModule.setSettingsKey("IncludeQtModule");
    includeQtModule.setLabelText(Tr::tr("Use Qt module name in #include-directive"));

    addQtVersionCheck.setSettingsKey("AddQtVersionCheck");
    addQtVersionCheck.setLabelText(Tr::tr("Add Qt version #ifdef for module names"));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Embedding of the UI Class")),
                Column {
                    embedding,
                }
            },
            Group {
                title(Tr::tr("Code Generation")),
                Column {
                    retranslationSupport,
                    includeQtModule,
                    addQtVersionCheck
                }
            },
            st
        };
    });


    readSettings();
    addQtVersionCheck.setEnabler(&includeQtModule);
}

class CodeGenSettingsPage final : public Core::IOptionsPage
{
public:
    CodeGenSettingsPage()
    {
        setId(Constants::CODEGEN_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("Qt Class Generation"));
        setCategory(CppEditor::Constants::CPP_SETTINGS_CATEGORY);
        setDisplayCategory(::CppEditor::Tr::tr(CppEditor::Constants::CPP_SETTINGS_NAME));
        setCategoryIconPath(":/projectexplorer/images/settingscategory_cpp.png");
        setSettingsProvider([] { return &codeGenSettings(); });
    }
};

const CodeGenSettingsPage settingsPage;

} // QtSupport
