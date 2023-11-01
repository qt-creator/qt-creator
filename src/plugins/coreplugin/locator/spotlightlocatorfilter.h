// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

namespace Core::Internal {

class SpotlightLocatorFilter : public ILocatorFilter
{
public:
    SpotlightLocatorFilter();

    bool openConfigDialog(QWidget *parent, bool &needsRefresh) final;

protected:
    void saveState(QJsonObject &obj) const final;
    void restoreState(const QJsonObject &obj) final;

private:
    LocatorMatcherTasks matchers() final;

    QString m_command;
    QString m_arguments;
    QString m_caseSensitiveArguments;
    bool m_sortResults = true;
};

} // namespace Core::Internal
