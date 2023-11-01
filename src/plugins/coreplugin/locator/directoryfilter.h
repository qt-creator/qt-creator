// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"
#include "ilocatorfilter.h"

namespace Core {

class CORE_EXPORT DirectoryFilter : public ILocatorFilter
{
public:
    DirectoryFilter(Utils::Id id);
    void restoreState(const QByteArray &state) override;
    bool openConfigDialog(QWidget *parent, bool &needsRefresh) override;

protected:
    void setIsCustomFilter(bool value);
    void addDirectory(const Utils::FilePath &directory);
    void removeDirectory(const Utils::FilePath &directory);
    void setFilters(const QStringList &filters);
    void setExclusionFilters(const QStringList &exclusionFilters);

    void saveState(QJsonObject &object) const override;
    void restoreState(const QJsonObject &object) override;

private:
    LocatorMatcherTasks matchers() final { return {m_cache.matcher()}; }
    void setDirectories(const Utils::FilePaths &directories);
    void handleAddDirectory();
    void handleEditDirectory();
    void handleRemoveDirectory();
    void updateOptionButtons();

    Utils::FilePaths m_directories;
    QStringList m_filters;
    QStringList m_exclusionFilters;
    // Our config dialog, uses in addDirectory and editDirectory
    // to give their dialogs the right parent
    class DirectoryFilterOptions *m_dialog = nullptr;
    bool m_isCustomFilter = true;
    LocatorFileCache m_cache;
};

} // namespace Core
