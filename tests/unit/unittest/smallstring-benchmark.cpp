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

#include <benchmark/benchmark.h>

#include <QByteArray>

#include <utils/smallstringvector.h>

namespace {

Utils::SmallStringView createText(int size)
{
    auto text = new char[size + 1];
    std::fill(text, text + size, 'x');
    text[size] = 0;

    return Utils::SmallStringView(text, size);
}

void CreateSmallStringLiteral(benchmark::State& state) {
    while (state.KeepRunning())
        constexpr auto string = Utils::SmallStringLiteral("very very very long long long text");
}
BENCHMARK(CreateSmallStringLiteral);

void CreateNullSmallString(benchmark::State& state) {
    while (state.KeepRunning())
        auto string = Utils::SmallString();
}
BENCHMARK(CreateNullSmallString);

void CreateSmallStringFromLiteral(benchmark::State& state) {
    while (state.KeepRunning())
        auto string = Utils::SmallString("very very very long long long text");
}
BENCHMARK(CreateSmallStringFromLiteral);

void CreateSmallString(benchmark::State& state) {
    auto text = createText(state.range_x());

    while (state.KeepRunning())
        auto string = Utils::SmallString(text.data(), state.range_x());
}
BENCHMARK(CreateSmallString)->Range(0, 1024);

void CreateQByteArray(benchmark::State& state) {
    auto text = createText(state.range_x());

    while (state.KeepRunning())
        QByteArray foo(text.data(), state.range_x());
}
BENCHMARK(CreateQByteArray)->Range(0, 1024);

void CreateQString(benchmark::State& state) {
    auto text = createText(state.range_x());

    while (state.KeepRunning())
        auto string = QString::fromUtf8(text.data(), state.range_x());
}
BENCHMARK(CreateQString)->Range(0, 1024);

void SmallStringAppend(benchmark::State& state) {
    auto text = createText(state.range_y());

    while (state.KeepRunning()) {
        auto string = Utils::SmallString();
        for (int i = 0; i < state.range_x(); ++i)
            string.append(text);
    }
}
BENCHMARK(SmallStringAppend)->RangePair(1, 64, 1, 4096);

void QByteArrayAppend(benchmark::State& state) {
    auto text = createText(state.range_y());

    while (state.KeepRunning()) {
        auto string = QByteArray();
        for (int i = 0; i < state.range_x(); ++i)
            string.append(text.data(), text.size());
    }
}
BENCHMARK(QByteArrayAppend)->RangePair(1, 64, 1, 4096);

void CompareCString(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsQuiteLong");

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text == "staticMetaObject");
}
BENCHMARK(CompareCString);

void CompareString(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsQuiteLong");

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text == Utils::SmallString("staticMetaObject"));
}
BENCHMARK(CompareString);

void CompareStringView(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsQuiteLong");

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text == Utils::SmallStringView("staticMetaObject"));
}
BENCHMARK(CompareStringView);

void CompareCStringPointer(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsQuiteLong");
    const char *cStringText = "staticMetaObject";

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text == cStringText);
}
BENCHMARK(CompareCStringPointer);

void StartsWith(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsQuiteLong");

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text.startsWith("qt_"));
}
BENCHMARK(StartsWith);

void ShortSize(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsQuiteLong");

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text.size());
}
BENCHMARK(ShortSize);

void LongSize(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsQuiteLong");

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text.size());
}
BENCHMARK(LongSize);

void Size(benchmark::State& state) {
    Utils::SmallString text("myFunctionIsMUCHMUCHMUCHMUCHMUCHLonger");

    while (state.KeepRunning())
        benchmark::DoNotOptimize(text.size());
}
BENCHMARK(LongSize);

void generateRandomString(char *data, const int length)
{
    static const char alphaNumericData[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < length; ++i) {
        data[i] = alphaNumericData[std::rand() % (sizeof(alphaNumericData) - 1)];
    }
}

uint entryCount=100000;

QByteArray generateRandomQByteArray()
{
    QByteArray text;
    text.resize(20);

    generateRandomString(text.data(), text.size());

    return text;
}

void Collection_CreateQByteArrays(benchmark::State& state)
{
    while (state.KeepRunning()) {
        QVector<QByteArray> values;
        values.reserve(entryCount);

        for (int i = 0; i < entryCount; i++)
            values.append(generateRandomQByteArray());
    }
}
BENCHMARK(Collection_CreateQByteArrays);

void Collection_SortQByteArrays(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();

        QVector<QByteArray> values;
        values.reserve(entryCount);

        for (int i = 0; i < entryCount; i++)
            values.append(generateRandomQByteArray());

        state.ResumeTiming();

        std::sort(values.begin(), values.end());
    }
}
BENCHMARK(Collection_SortQByteArrays);

void Collection_FilterQByteArrays(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();

        QVector<QByteArray> values;
        values.reserve(entryCount);
        QVector<QByteArray> filteredValues;
        filteredValues.reserve(entryCount);

        for (int i = 0; i < entryCount; i++)
            values.append(generateRandomQByteArray());

        auto startsWithA = [] (const QByteArray &byteArray) {
            return byteArray.startsWith('a');
        };

        state.ResumeTiming();

        std::copy_if(std::make_move_iterator(values.begin()),
                     std::make_move_iterator(values.end()),
                     std::back_inserter(filteredValues),
                     startsWithA);
    }
}
BENCHMARK(Collection_FilterQByteArrays);

Utils::SmallString generateRandomSmallString(int size = 20)
{
    Utils::SmallString text;
    text.resize(size);

    generateRandomString(text.data(), text.size());

    return text;
}

void Collection_CreateSmallStrings(benchmark::State& state)
{
    while (state.KeepRunning()) {
        Utils::SmallStringVector values;
        values.reserve(entryCount);

        for (int i = 0; i < entryCount; i++)
            values.push_back(generateRandomSmallString());
    }
}
BENCHMARK(Collection_CreateSmallStrings);

void Collection_SortSmallStrings(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();

        Utils::SmallStringVector values;
        values.reserve(entryCount);

        for (int i = 0; i < entryCount; i++)
            values.push_back(generateRandomSmallString());

        state.ResumeTiming();

        std::sort(values.begin(), values.end());
    }
}
BENCHMARK(Collection_SortSmallStrings);

void Collection_FilterSmallStrings(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();

        Utils::SmallStringVector values;
        values.reserve(entryCount);
        Utils::SmallStringVector filteredValues;
        filteredValues.reserve(entryCount);

        for (int i = 0; i < entryCount; i++)
            values.push_back(generateRandomSmallString());

        auto startsWithA = [] (const Utils::SmallString &byteArray) {
            return byteArray.startsWith('a');
        };

        state.ResumeTiming();

        std::copy_if(std::make_move_iterator(values.begin()),
                     std::make_move_iterator(values.end()),
                     std::back_inserter(filteredValues),
                     startsWithA);
    }
}
BENCHMARK(Collection_FilterSmallStrings);

void IterateOver(benchmark::State& state) {
    Utils::SmallString text = generateRandomSmallString(30);
    auto begin = std::next(text.begin(), state.range_x());
    auto end = std::next(text.begin(), state.range_y());

    while (state.KeepRunning()) {
        std::for_each(begin, end, [] (char x) {
            benchmark::DoNotOptimize(x);
        });
    }
}
BENCHMARK(IterateOver)->ArgPair(0, 8)
                      ->ArgPair(8, 16)
                      ->ArgPair(16, 24)
                      ->ArgPair(24, 31)
                      ->ArgPair(0, 8)
                      ->ArgPair(0, 16)
                      ->ArgPair(0, 24)
                      ->ArgPair(0, 31);


//void StringLiteralSize(benchmark::State& state) {
//    constexpr Utils::SmallStringLiteral text("myFunctionIsQuiteLong");

//    while (state.KeepRunning()) {
//        constexpr auto size = text.size(); // internal compile error
//        benchmark::DoNotOptimize(size);
//    }
//}
//BENCHMARK(StringLiteralSize);
}
