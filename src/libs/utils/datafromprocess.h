// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "filepath.h"
#include "process.h"

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

#include <chrono>
#include <functional>
#include <optional>
#include <utility>

namespace Utils {

// Use this facility for cached retrieval of data from a tool that always returns the same
// output for the same parameters and is side effect free.
// A prime example is version info via a --version switch.
template<typename Data> class DataFromProcess
{
public:
    class Parameters
    {
    public:
        using OutputParser = std::function<std::optional<Data>(const QString &)>;
        using ErrorHandler = std::function<void(const Process &)>;

        Parameters(const CommandLine &cmdLine, const OutputParser &parser)
            : commandLine(cmdLine)
            , parser(parser)
        {}

        CommandLine commandLine;
        Environment environment = Environment::systemEnvironment();
        std::chrono::seconds timeout = std::chrono::seconds(10);
        OutputParser parser;
        ErrorHandler errorHandler;
        QList<ProcessResult> allowedResults{ProcessResult::FinishedWithSuccess};
    };

    static std::optional<Data> getData(const Parameters &params);

    // TODO: async variant.

private:
    using Key = std::tuple<FilePath, QStringList, QString>;
    using Value = std::pair<std::optional<Data>, QDateTime>;
    static inline QHash<Key, Value> m_cache;
    static inline QMutex m_cacheMutex;
};

template<typename Data>
inline std::optional<Data> DataFromProcess<Data>::getData(const Parameters &params)
{
    if (params.commandLine.executable().isEmpty())
        return {};

    const auto key = std::make_tuple(params.commandLine.executable(),
                                     params.environment.toStringList(),
                                     params.commandLine.arguments());
    const QDateTime exeTimestamp = params.commandLine.executable().lastModified();
    {
        QMutexLocker<QMutex> cacheLocker(&m_cacheMutex);
        const auto it = m_cache.constFind(key);
        if (it != m_cache.constEnd() && it.value().second == exeTimestamp)
            return it.value().first;
    }

    Process outputRetriever;
    outputRetriever.setCommand(params.commandLine);
    outputRetriever.runBlocking(params.timeout);

    // Do not store into cache: The next call might succeed.
    if (outputRetriever.result() == ProcessResult::Canceled)
        return {};

    std::optional<Data> data;
    if (params.allowedResults.contains(outputRetriever.result()))
        data = params.parser(outputRetriever.cleanedStdOut());
    else if (params.errorHandler)
        params.errorHandler(outputRetriever);
    QMutexLocker<QMutex> cacheLocker(&m_cacheMutex);
    m_cache.insert(key, std::make_pair(data, exeTimestamp));
    return data;
}

} // namespace Utils
