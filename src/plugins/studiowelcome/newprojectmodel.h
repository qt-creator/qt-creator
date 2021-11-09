/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QAbstractListModel>
#include <QUrl>

#include <utils/filepath.h>
#include <utils/optional.h>

namespace Utils {
class Wizard;
}

namespace StudioWelcome {

struct ProjectItem
{
    QString name;
    QString categoryId;
    QString description;
    QUrl qmlPath;
    QString fontIconCode;
    std::function<Utils::Wizard *(const Utils::FilePath &path)> create;
};

inline QDebug &operator<<(QDebug &d, const ProjectItem &item)
{
    d << "name=" << item.name;
    d << "; category = " << item.categoryId;

    return d;
}

struct ProjectCategory
{
    QString id;
    QString name;
    std::vector<ProjectItem> items;
};

inline QDebug &operator<<(QDebug &d, const ProjectCategory &cat)
{
    d << "id=" << cat.id;
    d << "; name=" << cat.name;
    d << "; items=" << cat.items;

    return d;
}

using ProjectsByCategory = std::map<QString, ProjectCategory>;

/****************** BaseNewProjectModel ******************/

class BaseNewProjectModel : public QAbstractListModel
{
    using ProjectItems = std::vector<std::vector<ProjectItem>>;
    using Categories = std::vector<QString>;

public:
    explicit BaseNewProjectModel(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    void setProjects(const ProjectsByCategory &projects);

protected:
    const ProjectItems &projects() const { return m_projects; }
    const Categories &categories() const { return m_categories; }

private:
    ProjectItems m_projects;
    Categories m_categories;
};

/****************** NewProjectCategoryModel ******************/

class NewProjectCategoryModel : public BaseNewProjectModel
{
public:
    explicit NewProjectCategoryModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};

/****************** NewProjectModel ******************/

class NewProjectModel : public BaseNewProjectModel
{
    Q_OBJECT
public:
    explicit NewProjectModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void setPage(int index); // called from QML when view's header item is clicked
    Q_INVOKABLE QString fontIconCode(int index) const;

    int page() const { return static_cast<int>(m_page); }

    Utils::optional<ProjectItem> project(size_t selection) const
    {
        if (projects().empty())
            return {};

        if (m_page < projects().size()) {
            const std::vector<ProjectItem> projectsOfCategory = projects().at(m_page);
            if (selection < projectsOfCategory.size())
                return projects().at(m_page).at(selection);
        }
        return {};
    }

    bool empty() const { return projects().empty(); }

private:
    const std::vector<ProjectItem> projectsOfCurrentCategory() const
    { return projects().at(m_page); }

private:
    size_t m_page = 0;
};

} // namespace StudioWelcome
