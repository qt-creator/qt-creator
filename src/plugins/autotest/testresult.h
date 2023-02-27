// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "autotestconstants.h"

#include <utils/filepath.h>

#include <QColor>

#include <optional>

namespace Autotest {

class ITestTreeItem;

enum class ResultType {
    // result types (have icon, color, short text)
    Pass, FIRST_TYPE = Pass,
    Fail,
    ExpectedFail,
    UnexpectedPass,
    Skip,
    BlacklistedPass,
    BlacklistedFail,
    BlacklistedXPass,
    BlacklistedXFail,

    // special (message) types (have icon, color, short text)
    Benchmark,
    MessageDebug,
    MessageInfo,
    MessageWarn,
    MessageFatal,
    MessageSystem,
    MessageError,

    // special message - get's icon (but no color/short text) from parent
    MessageLocation,
    // anything below is an internal message (or a pure message without icon)
    MessageInternal, INTERNAL_MESSAGES_BEGIN = MessageInternal,
    // start item (get icon/short text depending on children)
    TestStart,
    // usually no icon/short text - more or less an indicator (and can contain test duration)
    TestEnd,
    // special global (temporary) message
    MessageCurrentTest, INTERNAL_MESSAGES_END = MessageCurrentTest,

    Application,        // special.. not to be used outside of testresultmodel
    Invalid,            // indicator for unknown result items
    LAST_TYPE = Invalid
};

inline auto qHash(const ResultType &result)
{
    return QT_PREPEND_NAMESPACE(qHash(int(result)));
}

class TestResult;

struct ResultHooks
{
    using OutputStringHook = std::function<QString(const TestResult &, bool)>;
    using FindTestItemHook = std::function<ITestTreeItem *(const TestResult &)>;
    using DirectParentHook = std::function<bool(const TestResult &, const TestResult &, bool *)>;
    using IntermediateHook = std::function<bool(const TestResult &, const TestResult &)>;
    using CreateResultHook = std::function<TestResult(const TestResult &)>;
    QVariant extraData;
    OutputStringHook outputString = {};
    FindTestItemHook findTestItem = {};
    DirectParentHook directParent = {};
    IntermediateHook intermediate = {};
    CreateResultHook createResult = {};
};

class TestResult
{
public:
    TestResult() = default;
    TestResult(const QString &id, const QString &name, const ResultHooks &hooks = {});
    virtual ~TestResult() {}

    bool isValid() const;
    const QString outputString(bool selected) const;
    const ITestTreeItem *findTestTreeItem() const;

    QString id() const { return m_id.value_or(QString()); }
    QString name() const { return m_name; }
    ResultType result() const { return m_result; }
    QString description() const { return m_description; }
    Utils::FilePath fileName() const { return m_file; }
    int line() const { return m_line; }
    QVariant extraData() const { return m_hooks.extraData; }

    void setDescription(const QString &description) { m_description = description; }
    void setFileName(const Utils::FilePath &fileName) { m_file = fileName; }
    void setLine(int line) { m_line = line; }
    void setResult(ResultType type) { m_result = type; }

    static ResultType resultFromString(const QString &resultString);
    static ResultType toResultType(int rt);
    static QString resultToString(const ResultType type);
    static QColor colorForType(const ResultType type);

    bool isDirectParentOf(const TestResult &other, bool *needsIntermediate) const;
    bool isIntermediateFor(const TestResult &other) const;
    TestResult intermediateResult() const;

private:
    std::optional<QString> m_id = {};
    QString m_name;
    ResultType m_result = ResultType::Invalid;  // the real result..
    QString m_description;
    Utils::FilePath m_file;
    int m_line = 0;
    ResultHooks m_hooks = {};
};

} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::TestResult)
Q_DECLARE_METATYPE(Autotest::ResultType)
