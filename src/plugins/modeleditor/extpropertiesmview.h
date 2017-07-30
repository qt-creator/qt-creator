/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

private:
    void onConfigPathChanged(const QString &path);

private:
    qmt::ProjectController *m_projectController = nullptr;
    Utils::PathChooser *m_configPath = nullptr;
    QLabel *m_configPathInfo = nullptr;
};

} // namespace Interal
} // namespace ModelEditor
