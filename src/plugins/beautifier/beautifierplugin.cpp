// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "beautifierconstants.h"
#include "beautifiertr.h"

#include "artisticstyle/artisticstyle.h"
#include "clangformat/clangformat.h"
#include "uncrustify/uncrustify.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <extensionsystem/iplugin.h>

using namespace Core;

namespace Beautifier::Internal {

class BeautifierPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Beautifier.json")

    void initialize() final
    {
        IOptionsPage::registerCategory(
            Constants::OPTION_CATEGORY,
            Tr::tr("Beautifier"),
            ":/beautifier/images/settingscategory_beautifier.png");

        MenuBuilder(Constants::MENU_ID)
            .setTitle(Tr::tr("Bea&utifier"))
            .setOnAllDisabledBehavior(ActionContainer::Show)
            .addToContainer(Core::Constants::M_TOOLS);

        setupArtisticStyle();
        setupClangFormat();
        setupUncrustify();
    }
};

} // Beautifier::Internal

#include "beautifierplugin.moc"
