// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "basefilefilter.h"

#include <functional>

namespace Core {
namespace Internal {

// TODO: Don't derive from BaseFileFilter, flatten the hierarchy
class SpotlightLocatorFilter : public BaseFileFilter
{
    Q_OBJECT
public:
    SpotlightLocatorFilter();

    void prepareSearch(const QString &entry) override;

    using ILocatorFilter::openConfigDialog;
    bool openConfigDialog(QWidget *parent, bool &needsRefresh) final;

protected:
    void saveState(QJsonObject &obj) const final;
    void restoreState(const QJsonObject &obj) final;

private:
    LocatorMatcherTasks matchers() final;
    void reset();

    QString m_command;
    QString m_arguments;
    QString m_caseSensitiveArguments;
};

} // Internal
} // Core
