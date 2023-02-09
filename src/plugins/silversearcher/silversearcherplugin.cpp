// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "silversearcherplugin.h"
#include "findinfilessilversearcher.h"
#include "outputparser_test.h"

namespace SilverSearcher::Internal {

void SilverSearcherPlugin::initialize()
{
    new FindInFilesSilverSearcher(this);

#ifdef WITH_TESTS
    addTest<OutputParserTest>();
#endif
}

} // SilverSearcher::Internal
