#ifndef CODESTYLESETTINGSPROPERTIESPAGE_H
#define CODESTYLESETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"
#include "ui_codestylesettingspropertiespage.h"

namespace ProjectExplorer {

class EditorConfiguration;

namespace Internal {

const char * const CODESTYLESETTINGS_PANEL_ID("ProjectExplorer.CodeStyleSettingsPanel");

class CodeStyleSettingsPanelFactory : public IProjectPanelFactory
{
public:
    QString id() const;
    QString displayName() const;
    PropertiesPanel *createPanel(Project *project);
    bool supports(Project *project);
};

class CodeStyleSettingsWidget;

class CodeStyleSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    CodeStyleSettingsWidget(Project *project);

private:
    ProjectExplorer::Ui::CodeStyleSettingsPropertiesPage m_ui;
    Project *m_project;
};

} // namespace Internal
} // namespace ProjectExplorer


#endif // CODESTYLESETTINGSPROPERTIESPAGE_H
