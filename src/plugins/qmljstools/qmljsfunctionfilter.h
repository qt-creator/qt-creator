// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace QmlJSTools {
namespace Internal {

class LocatorData;

class FunctionFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    explicit FunctionFilter(LocatorData *data, QObject *parent = nullptr);

    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
private:
    Core::LocatorMatcherTasks matchers() final;

    LocatorData *m_data = nullptr;
};

} // namespace Internal
} // namespace QmlJSTools
