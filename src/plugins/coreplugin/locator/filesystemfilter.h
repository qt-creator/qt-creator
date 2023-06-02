// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

namespace Core::Internal {

class FileSystemFilter : public ILocatorFilter
{
public:
    FileSystemFilter();
    void restoreState(const QByteArray &state) final;
    bool openConfigDialog(QWidget *parent, bool &needsRefresh) final;

protected:
    void saveState(QJsonObject &object) const final;
    void restoreState(const QJsonObject &object) final;

private:
    LocatorMatcherTasks matchers() final;

    static const bool s_includeHiddenDefault = true;
    bool m_includeHidden = s_includeHiddenDefault;
};

} // namespace Core::Internal
