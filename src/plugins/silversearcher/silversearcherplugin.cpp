// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinfilessilversearcher.h"
#include "silversearcherparser_test.h"

#include <extensionsystem/iplugin.h>

namespace SilverSearcher::Internal {

class SilverSearcherPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SilverSearcher.json")

    void initialize() final
    {
        new FindInFilesSilverSearcher(this);

#ifdef WITH_TESTS
        addTest<OutputParserTest>();
#endif
    }
};

} // SilverSearcher::Internal

#include "silversearcherplugin.moc"
