// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "basefilefilter.h"

#include <functional>

namespace Core {
namespace Internal {

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
    void reset();

    QString m_command;
    QString m_arguments;
    QString m_caseSensitiveArguments;
};

} // Internal
} // Core
