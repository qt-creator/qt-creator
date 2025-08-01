// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectstorage/projectstorageinfotypes.h>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>
#include <utils/google-using-declarations.h>

auto IsVersionNumber(const auto &matcher)
{
    return Field("QmlDesigner::Storage::VersionNumber::value", &QmlDesigner::Storage::VersionNumber::value, matcher);
}

inline auto HasNoVersionNumber()
{
    return IsVersionNumber(QmlDesigner::Storage::VersionNumber::noVersion);
}

template<typename Matcher>
auto IsMinorVersion(const Matcher &matcher)
{
    return Field("Version::minor", &QmlDesigner::Storage::Version::minor, matcher);
}

auto IsMajorVersion(const auto &matcher)
{
    return Field("Version::major", &QmlDesigner::Storage::Version::major, matcher);
}

auto IsVersion(const auto &majorMatcher, const auto &minorMatcher)
{
    return AllOf(IsMajorVersion(IsVersionNumber(majorMatcher)),
                 IsMinorVersion(IsVersionNumber(minorMatcher)));
}

auto IsVersion(const auto &majorMatcher)
{
    return AllOf(IsMajorVersion(IsVersionNumber(majorMatcher)), IsMinorVersion(HasNoVersionNumber()));
}

inline auto HasNoVersion()
{
    return AllOf(IsMajorVersion(HasNoVersionNumber()), IsMinorVersion(HasNoVersionNumber()));
}
