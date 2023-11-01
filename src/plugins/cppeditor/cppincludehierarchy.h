// Copyright (C) 2016 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    using base_type = Utils::TreeModel<CppIncludeHierarchyItem>;

public:
    CppIncludeHierarchyModel();

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    void buildHierarchy(const Utils::FilePath &filePath);
    const Utils::FilePath &editorFilePath() const { return m_editorFilePath; }
    void setSearching(bool on);
    QString toString() const;

#if WITH_TESTS
    using base_type::canFetchMore;
    using base_type::fetchMore;
#endif

private:
    friend class CppIncludeHierarchyItem;
    Utils::FilePath m_editorFilePath;
    QSet<Utils::FilePath> m_seen;
    bool m_searching = false;
};

class CppIncludeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    CppIncludeHierarchyFactory();

    Core::NavigationView createWidget() override;
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
};

} // namespace Internal
} // namespace CppEditor
