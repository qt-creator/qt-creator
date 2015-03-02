/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
#ifndef CLANGSTATICANALYZERPROJECTSETTINGSWIDGET_H
#define CLANGSTATICANALYZERPROJECTSETTINGSWIDGET_H

#include <QWidget>

namespace ProjectExplorer { class Project; }

namespace ClangStaticAnalyzer {
namespace Internal {
class ProjectSettings;

namespace Ui { class ProjectSettingsWidget; }

class ProjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectSettingsWidget(ProjectExplorer::Project *project, QWidget *parent = 0);
    ~ProjectSettingsWidget();

private:
    void updateButtonStates();
    void updateButtonStateRemoveSelected();
    void updateButtonStateRemoveAll();
    void removeSelected();

    Ui::ProjectSettingsWidget * const m_ui;
    ProjectSettings * const m_projectSettings;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // Include guard.
