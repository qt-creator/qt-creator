// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionspage.h"

#include "haskellconstants.h"
#include "haskellmanager.h"
#include "haskelltr.h"

#include <utils/pathchooser.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Haskell::Internal {

class HaskellOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    HaskellOptionsPageWidget()
    {
        auto stackPath = new PathChooser;
        stackPath->setExpectedKind(PathChooser::ExistingCommand);
        stackPath->setPromptDialogTitle(Tr::tr("Choose Stack Executable"));
        stackPath->setFilePath(HaskellManager::stackExecutable());
        stackPath->setCommandVersionArguments({"--version"});

        setOnApply([stackPath] { HaskellManager::setStackExecutable(stackPath->rawFilePath()); });

        using namespace Layouting;
        Column {
            Group {
                title(Tr::tr("General")),
                Row { Tr::tr("Stack executable:"), stackPath }
            },
            st,
        }.attachTo(this);
    }
};

OptionsPage::OptionsPage()
{
    setId(Constants::OPTIONS_GENERAL);
    setDisplayName(Tr::tr("General"));
    setCategory("J.Z.Haskell");
    setDisplayCategory(Tr::tr("Haskell"));
    setCategoryIconPath(":/haskell/images/settingscategory_haskell.png");
    setWidgetCreator([] { return new HaskellOptionsPageWidget; });
}

} // Haskell::Internal
