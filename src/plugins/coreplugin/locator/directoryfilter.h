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

#pragma once

#include "basefilefilter.h"

#include <coreplugin/core_global.h>

#include <QString>
#include <QByteArray>
#include <QFutureInterface>
#include <QMutex>

namespace Core {
namespace Internal {
namespace Ui {
class DirectoryFilterOptions;
} // namespace Ui
} // namespace Internal

class CORE_EXPORT DirectoryFilter : public BaseFileFilter
{
    Q_OBJECT

public:
    DirectoryFilter(Utils::Id id);
    QByteArray saveState() const override;
    void restoreState(const QByteArray &state) override;
    bool openConfigDialog(QWidget *parent, bool &needsRefresh) override;
    void refresh(QFutureInterface<void> &future) override;

    void setIsCustomFilter(bool value);
    void setDirectories(const QStringList &directories);
    void addDirectory(const QString &directory);
    void removeDirectory(const QString &directory);
    QStringList directories() const;
    void setFilters(const QStringList &filters);
    void setExclusionFilters(const QStringList &exclusionFilters);

    using ILocatorFilter::setDisplayName;

private:
    void handleAddDirectory();
    void handleEditDirectory();
    void handleRemoveDirectory();
    void updateOptionButtons();
    void updateFileIterator();

    QStringList m_directories;
    QStringList m_filters;
    QStringList m_exclusionFilters;
    // Our config dialog, uses in addDirectory and editDirectory
    // to give their dialogs the right parent
    QDialog *m_dialog = nullptr;
    Internal::Ui::DirectoryFilterOptions *m_ui = nullptr;
    mutable QMutex m_lock;
    Utils::FilePaths m_files;
    bool m_isCustomFilter = true;
};

} // namespace Core
