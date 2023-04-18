// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

#include <QTimer>

#include <atomic>
#include <memory>

QT_BEGIN_NAMESPACE
class QJSEngine;
QT_END_NAMESPACE

class JavaScriptEngine;

namespace Core {
namespace Internal {

class JavaScriptFilter : public Core::ILocatorFilter
{
    Q_OBJECT
public:
    JavaScriptFilter();
    ~JavaScriptFilter() override;

    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
private:
    LocatorMatcherTasks matchers() final;
    void setupEngine();

    mutable std::unique_ptr<QJSEngine> m_engine;
    QTimer m_abortTimer;
    std::atomic_bool m_aborted = false;
    std::unique_ptr<JavaScriptEngine> m_javaScriptEngine;
};

} // namespace Internal
} // namespace Core
