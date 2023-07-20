// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectstorage/projectstorageinfotypes.h>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>

template<typename Matcher>
auto IsVersionNumber(const Matcher &matcher)
{
    return Field(&QmlDesigner::Storage::VersionNumber::value, matcher);
}

template<typename Matcher>
auto IsMinorVersion(const Matcher &matcher)
{
    return Field(&QmlDesigner::Storage::Version::minor, matcher);
}

template<typename Matcher>
auto IsMajorVersion(const Matcher &matcher)
{
    return Field(&QmlDesigner::Storage::Version::major, matcher);
}

template<typename MajorMatcher, typename MinorMatcher>
auto IsVersion(const MajorMatcher &majorMatcher, const MinorMatcher &minorMatcher)
{
    return AllOf(IsMajorVersion(IsVersionNumber(majorMatcher)),
                 IsMinorVersion(IsVersionNumber(minorMatcher)));
}
