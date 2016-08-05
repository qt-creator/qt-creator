/****************************************************************************
**
** Copyright (C) 2016 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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

#include <coreplugin/inavigationwidgetfactory.h>
#include <utils/treemodel.h>

#include <QSet>

namespace CppEditor {
namespace Internal {

class CppIncludeHierarchyItem;

class CppIncludeHierarchyModel : public Utils::TreeModel<CppIncludeHierarchyItem>
{
    Q_OBJECT
    typedef Utils::TreeModel<CppIncludeHierarchyItem> base_type;

public:
    CppIncludeHierarchyModel();

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    void buildHierarchy(const QString &filePath);
    QString editorFilePath() const { return m_editorFilePath; }
    void setSearching(bool on);
    QString toString() const;

#if WITH_TESTS
    using base_type::canFetchMore;
    using base_type::fetchMore;
#endif

private:
    friend class CppIncludeHierarchyItem;
    QString m_editorFilePath;
    QSet<QString> m_seen;
    bool m_searching = false;
};

class CppIncludeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    CppIncludeHierarchyFactory();

    Core::NavigationView createWidget() override;
};

} // namespace Internal
} // namespace CppEditor
