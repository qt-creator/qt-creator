// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model_widgets_ui/propertiesviewmview.h"

namespace qmt { class ProjectController; }

namespace Utils { class PathChooser; }

namespace ModelEditor {
namespace Internal {

class ExtPropertiesMView : public qmt::PropertiesView::MView
{
    Q_OBJECT

public:
    ExtPropertiesMView(qmt::PropertiesView *view);
    ~ExtPropertiesMView();

    void setProjectController(qmt::ProjectController *projectController);

    void visitMPackage(const qmt::MPackage *package) override;

protected:
    void visitMObjectBehind(const qmt::MObject *object) override;
    void visitDObjectBefore(const qmt::DObject *object) override;

private:
    void onConfigPathChanged(const QString &path);
    void onFileLinkPathChanged(const QString &path);
    void onImagePathChanged(const QString &path);

private:
    qmt::ProjectController *m_projectController = nullptr;
    Utils::PathChooser *m_filelinkPathChooser = nullptr;
    Utils::PathChooser *m_configPath = nullptr;
    QLabel *m_configPathInfo = nullptr;
    Utils::PathChooser *m_imagePathChooser = nullptr;
};

} // namespace Interal
} // namespace ModelEditor
