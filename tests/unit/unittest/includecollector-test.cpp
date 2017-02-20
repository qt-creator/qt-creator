/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "googletest.h"

#include <includecollector.h>
#include <stringcache.h>

using testing::AllOf;
using testing::Contains;
using testing::Not;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

namespace {

class IncludeCollector : public ::testing::Test
{
protected:
    void SetUp();
    uint id(const Utils::SmallString &path);

protected:
    ClangBackEnd::StringCache<Utils::PathString> filePathCache;
    ClangBackEnd::IncludeCollector collector{filePathCache};
    ClangBackEnd::IncludeCollector emptyCollector{filePathCache};
    Utils::PathStringVector excludePaths = {TESTDATA_DIR "/includecollector_main.h",
                                            TESTDATA_DIR "/includecollector_main2.h",
                                            TESTDATA_DIR "/includecollector_header1.h",
                                            TESTDATA_DIR "/includecollector_header2.h",
                                            TESTDATA_DIR "/includecollector_generated_file.h"};
};

TEST_F(IncludeCollector, IncludesExternalHeader)
{
    collector.collectIncludes();

    ASSERT_THAT(collector.takeIncludeIds(),
                AllOf(Contains(id(TESTDATA_DIR "/includecollector_external1.h")),
                      Contains(id(TESTDATA_DIR "/includecollector_external2.h"))));
}

TEST_F(IncludeCollector, DoesNotIncludesInternalHeader)
{
    collector.collectIncludes();

    ASSERT_THAT(collector.takeIncludeIds(), Not(Contains(id(TESTDATA_DIR "/includecollector_header1.h"))));
}

TEST_F(IncludeCollector, NoDuplicate)
{
    collector.collectIncludes();

    ASSERT_THAT(collector.takeIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector_external1.h"),
                                     id(TESTDATA_DIR "/includecollector_external2.h"),
                                     id(TESTDATA_DIR "/includecollector_external3.h"))) << filePathCache.string(3);
}

TEST_F(IncludeCollector, IncludesAreSorted)
{
    collector.collectIncludes();

    ASSERT_THAT(collector.takeIncludeIds(),
                ElementsAre(0, 1, 2));
}

TEST_F(IncludeCollector, If)
{
    emptyCollector.addFile(TESTDATA_DIR, "includecollector_if.cpp", "", {"cc", "includecollector_if.cpp"});

    emptyCollector.collectIncludes();

    ASSERT_THAT(emptyCollector.takeIncludeIds(),
                ElementsAre(id(TESTDATA_DIR "/includecollector_true.h")));
}

TEST_F(IncludeCollector, LocalPath)
{
    emptyCollector.addFile(TESTDATA_DIR, "includecollector_main.cpp", "", {"cc", "includecollector_main.cpp"});

    emptyCollector.collectIncludes();

    ASSERT_THAT(emptyCollector.takeIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector_external1.h"),
                                     id(TESTDATA_DIR "/includecollector_external2.h"),
                                     id(TESTDATA_DIR "/includecollector_external3.h")));
}

TEST_F(IncludeCollector, IgnoreMissingFile)
{
    emptyCollector.addFile(TESTDATA_DIR, "includecollector_missingfile.cpp", "", {"cc", "includecollector_missingfile.cpp"});

    emptyCollector.collectIncludes();

    ASSERT_THAT(emptyCollector.takeIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector_external1.h")));
}

void IncludeCollector::SetUp()
{
    collector.addFile(TESTDATA_DIR, "includecollector_main.cpp", "", {"cc", "includecollector_main.cpp"});
    collector.addFile(TESTDATA_DIR, "includecollector_main2.cpp", "", {"cc", "includecollector_main2.cpp"});

    collector.addUnsavedFiles({{{TESTDATA_DIR, "includecollector_generated_file.h"}, "#pragma once", {}}});

    collector.setExcludedIncludes(excludePaths.clone());
    emptyCollector.setExcludedIncludes(excludePaths.clone());
}

uint IncludeCollector::id(const Utils::SmallString &path)
{
    return filePathCache.stringId(path);
}

}
