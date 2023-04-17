// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

namespace Core {

/* Command locators: Provides completion for a set of
 * Core::Command's by sub-string of their action's text. */
class Command;
struct CommandLocatorPrivate;

class CORE_EXPORT CommandLocator : public ILocatorFilter
{
    Q_OBJECT

public:
    CommandLocator(Utils::Id id, const QString &displayName,
                   const QString &shortCutString, QObject *parent = nullptr);
    ~CommandLocator() override;

    void appendCommand(Command *cmd);

    void prepareSearch(const QString &entry) override;
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override;
private:
    CommandLocatorPrivate *d = nullptr;
};

} // namespace Core
