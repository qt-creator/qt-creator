// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "filepath.h"
#include "qtcprocess.h"

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

#include <chrono>
#include <functional>
#include <memory>
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
        using Callback = std::function<void(const std::optional<Data> &)>;

        Parameters(const CommandLine &cmdLine, const OutputParser &parser)
            : commandLine(cmdLine)
            , parser(parser)
        {}

        CommandLine commandLine;
        Environment environment = Environment::systemEnvironment();
        std::chrono::seconds timeout = std::chrono::seconds(10);
        OutputParser parser;
        ErrorHandler errorHandler;
        Callback callback;
        QList<ProcessResult> allowedResults{ProcessResult::FinishedWithSuccess};
    };

    // Use the first variant whenever possible.
    static void provideData(const Parameters &params);
    static std::optional<Data> getData(const Parameters &params);

private:
    using Key = std::tuple<FilePath, QStringList, QString>;
    using Value = std::pair<std::optional<Data>, QDateTime>;

    static std::optional<Data> getOrProvideData(const Parameters &params);
    static std::optional<Data> handleProcessFinished(const Parameters &params,
                                                     const QDateTime &exeTimestamp,
                                                     const Key &cacheKey,
                                                     const std::shared_ptr<Process> &process);

    static inline QHash<Key, Value> m_cache;
    static inline QMutex m_cacheMutex;
};

template<typename Data>
inline void DataFromProcess<Data>::provideData(const Parameters &params)
{
    QTC_ASSERT(params.callback, return);
    getOrProvideData(params);
}

template<typename Data>
inline std::optional<Data> DataFromProcess<Data>::getData(const Parameters &params)
{
    QTC_ASSERT(!params.callback, return {});
    return getOrProvideData(params);
}

template<typename Data>
inline std::optional<Data> DataFromProcess<Data>::getOrProvideData(const Parameters &params)
{
    if (params.commandLine.executable().isEmpty()) {
        if (params.callback)
            params.callback({});
        return {};
    }

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

    const auto outputRetriever = std::make_shared<Process>();
    outputRetriever->setCommand(params.commandLine);
    if (params.callback) {
        QObject::connect(outputRetriever.get(),
                         &Process::done,
                         [params, exeTimestamp, key, outputRetriever] {
                             handleProcessFinished(params, exeTimestamp, key, outputRetriever);
                         });
        outputRetriever->start();
        return {};
    }

    outputRetriever->runBlocking(params.timeout);
    return handleProcessFinished(params, exeTimestamp, key, outputRetriever);
}

template<typename Data>
inline std::optional<Data> DataFromProcess<Data>::handleProcessFinished(
    const Parameters &params,
    const QDateTime &exeTimestamp,
    const Key &cacheKey,
    const std::shared_ptr<Process> &process)
{
    // Do not store into cache: The next call might succeed.
    if (process->result() == ProcessResult::Canceled) {
        if (params.callback)
            params.callback({});
        return {};
    }

    std::optional<Data> data;
    if (params.allowedResults.contains(process->result()))
        data = params.parser(process->cleanedStdOut());
    else if (params.errorHandler)
        params.errorHandler(*process);
    QMutexLocker<QMutex> cacheLocker(&m_cacheMutex);
    m_cache.insert(cacheKey, std::make_pair(data, exeTimestamp));
    if (params.callback) {
        params.callback(data);
        return {};
    }
    return data;
}

} // namespace Utils
