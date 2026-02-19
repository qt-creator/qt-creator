// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "filepath.h"
#include "qtcprocess.h"
#include "synchronizedvalue.h"
#include "threadutils.h"
#include "shutdownguard.h"
#include "utils_global.h"

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

class QTCREATOR_UTILS_EXPORT DataFromProcessSettingsCache
{
public:
    struct ProcessOutput
    {
        static std::optional<ProcessOutput> fromVariant(const QVariant &var);
        QVariant toVariant() const;

        QString stdOut;
        QString stdErr;
    };

    static void writeToSettings();

    static std::optional<ProcessOutput> value(const QString &key);
    static void setValue(const QString &key, const ProcessOutput &value);

private:
    static SynchronizedValue<QHash<QString, QVariant>> m_newValuesCache;
};

// Use this facility for cached retrieval of data from a tool that always returns the same
// output for the same parameters and is side effect free.
// A prime example is version info via a --version switch.
template<typename Data> class DataFromProcess
{
public:
    class Parameters
    {
    public:
        using OutputParser = std::function<
            std::optional<Data>(const QString & /* stdOut */, const QString & /* stdErr */)>;
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
        Callback cachedValueChangedCallback;
        bool persistValue = true;
        QList<ProcessResult> allowedResults{ProcessResult::FinishedWithSuccess};
        bool disableUnixTerminal = false;
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
                                                     const Process *process);

    static inline SynchronizedValue<QHash<Key, Value>> m_cache;
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

    const auto cachedValue = m_cache.get(
        [&key, &exeTimestamp](const QHash<Key, Value> &cache) -> std::optional<Data> {
            const auto it = cache.constFind(key);
            if (it != cache.constEnd() && it.value().second == exeTimestamp)
                return it.value().first;
            return std::nullopt;
        });

    if (cachedValue) {
        if (params.callback)
            params.callback(cachedValue);
        return cachedValue;
    }

    const auto outputRetriever = new Process();
    outputRetriever->setCommand(params.commandLine);
    outputRetriever->setEnvironment(params.environment);
    if (params.disableUnixTerminal)
        outputRetriever->setDisableUnixTerminal();

    if (Utils::isMainThread()) {
        outputRetriever->setParent(Utils::shutdownGuard());
        if (params.persistValue && !params.callback) {
            const QChar separator = params.commandLine.executable().pathListSeparator();
            const QString stringKey = params.commandLine.executable().toUrlishString() + separator
                                      + params.commandLine.arguments() + separator
                                      + params.environment.toStringList().join(separator);
            const std::optional<DataFromProcessSettingsCache::ProcessOutput> output
                = DataFromProcessSettingsCache::value(stringKey);
            if (output) {
                std::optional<Data> data = params.parser(output->stdOut, output->stdErr);

                m_cache.writeLocked()->insert(key, std::make_pair(data, exeTimestamp));

                QObject::connect(
                    outputRetriever,
                    &Process::done,
                    Utils::shutdownGuard(),
                    [params, exeTimestamp, key, outputRetriever] {
                        handleProcessFinished(params, exeTimestamp, key, outputRetriever);
                        outputRetriever->deleteLater();
                    });
                outputRetriever->start();
                return data;
            }
        }
        if (params.callback) {
            QObject::connect(
                outputRetriever,
                &Process::done,
                Utils::shutdownGuard(),
                [params, exeTimestamp, key, outputRetriever] {
                    handleProcessFinished(params, exeTimestamp, key, outputRetriever);
                    outputRetriever->deleteLater();
                });
            outputRetriever->start();
            return {};
        }
    }

    outputRetriever->runBlocking(params.timeout);
    const std::optional<Data> result
        = handleProcessFinished(params, exeTimestamp, key, outputRetriever);
    delete outputRetriever;
    return result;
}

template<typename Data>
inline std::optional<Data> DataFromProcess<Data>::handleProcessFinished(
    const Parameters &params,
    const QDateTime &exeTimestamp,
    const Key &cacheKey,
    const Process *process)
{
    // Do not store into cache: The next call might succeed.
    if (process->result() == ProcessResult::Canceled) {
        if (params.callback)
            params.callback({});
        return {};
    }

    std::optional<Data> data;
    if (params.allowedResults.contains(process->result())) {
        if (params.persistValue) {
            const QChar separator = params.commandLine.executable().pathListSeparator();
            const QString stringKey = params.commandLine.executable().toUrlishString() + separator
                                      + params.commandLine.arguments() + separator
                                      + params.environment.toStringList().join(separator);
            const DataFromProcessSettingsCache::ProcessOutput
                output{process->cleanedStdOut(), process->cleanedStdErr()};
            DataFromProcessSettingsCache::setValue(stringKey, output);
        }
        data = params.parser(process->cleanedStdOut(), process->cleanedStdErr());
    } else if (params.errorHandler) {
        params.errorHandler(*process);
    }

    if (params.cachedValueChangedCallback) {
        const bool valueChanged = m_cache.get(
            [&cacheKey, &exeTimestamp, &data](const auto &cache) {
                const auto it = cache.constFind(cacheKey);
                if (it != cache.constEnd() && it.value().second == exeTimestamp) {
                    if (it.value().first != data)
                        return true;
                }
                return false;
            });

        if (valueChanged)
            params.cachedValueChangedCallback(data);
    }

    m_cache.writeLocked()->insert(cacheKey, std::make_pair(data, exeTimestamp));

    if (params.callback) {
        params.callback(data);
        return {};
    }
    return data;
}

} // namespace Utils
