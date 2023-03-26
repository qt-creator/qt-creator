// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonwrapper.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QUuid>

namespace {
template<typename First>
void impl_option_cat(QStringList &list, const First &first)
{
    list.append(first);
}

template<typename First, typename... T>
void impl_option_cat(QStringList &list, const First &first, const T &...args)
{
    impl_option_cat(list, first);
    impl_option_cat(list, args...);
}

template<typename... T>
QStringList options_cat(const T &...args)
{
    QStringList result;
    impl_option_cat(result, args...);
    return result;
}

} // namespace
namespace MesonProjectManager {
namespace Internal {

Command MesonWrapper::setup(const Utils::FilePath &sourceDirectory,
                            const Utils::FilePath &buildDirectory,
                            const QStringList &options) const
{
    return {m_exe,
            sourceDirectory,
            options_cat("setup", options, sourceDirectory.toString(), buildDirectory.toString())};
}

Command MesonWrapper::configure(const Utils::FilePath &sourceDirectory,
                                const Utils::FilePath &buildDirectory,
                                const QStringList &options) const
{
    if (!isSetup(buildDirectory))
        return setup(sourceDirectory, buildDirectory, options);
    return {m_exe, buildDirectory, options_cat("configure", options, buildDirectory.toString())};
}

Command MesonWrapper::regenerate(const Utils::FilePath &sourceDirectory,
                                 const Utils::FilePath &buildDirectory) const
{
    return {m_exe,
            buildDirectory,
            options_cat("--internal",
                        "regenerate",
                        sourceDirectory.toString(),
                        buildDirectory.toString(),
                        "--backend",
                        "ninja")};
}

Command MesonWrapper::introspect(const Utils::FilePath &sourceDirectory) const
{
    return {m_exe,
            sourceDirectory,
            {"introspect", "--all", QString("%1/meson.build").arg(sourceDirectory.toString())}};
}

} // namespace Internal
} // namespace MesonProjectManager
