#include "codestylesettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"
#include <texteditor/codestylepreferencesmanager.h>
#include <texteditor/icodestylepreferencesfactory.h>

#include <QtCore/QTextCodec>

using namespace TextEditor;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

QString CodeStyleSettingsPanelFactory::id() const
{
    return QLatin1String(CODESTYLESETTINGS_PANEL_ID);
}

QString CodeStyleSettingsPanelFactory::displayName() const
{
    return QCoreApplication::translate("CodeStyleSettingsPanelFactory", "Code Style Settings");
}

bool CodeStyleSettingsPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

PropertiesPanel *CodeStyleSettingsPanelFactory::createPanel(Project *project)
{
    PropertiesPanel *panel = new PropertiesPanel;
    panel->setWidget(new CodeStyleSettingsWidget(project));
    panel->setIcon(QIcon(":/projectexplorer/images/CodeStyleSettings.png"));
    panel->setDisplayName(QCoreApplication::translate("CodeStyleSettingsPanel", "Code Style Settings"));
    return panel;
}

CodeStyleSettingsWidget::CodeStyleSettingsWidget(Project *project) : QWidget(), m_project(project)
{
    m_ui.setupUi(this);

    const EditorConfiguration *config = m_project->editorConfiguration();
    CodeStylePreferencesManager *manager =
            CodeStylePreferencesManager::instance();

    QList<ICodeStylePreferencesFactory *> factories = manager->factories();
    for (int i = 0; i < factories.count(); i++) {
        ICodeStylePreferencesFactory *factory = factories.at(i);
        const QString languageId = factory->languageId();
        TabPreferences *tabPreferences = config->tabPreferences(languageId);
        IFallbackPreferences *codeStylePreferences = config->codeStylePreferences(languageId);

        QWidget *widget = factory->createEditor(codeStylePreferences, tabPreferences, m_ui.stackedWidget);
        m_ui.stackedWidget->addWidget(widget);
        m_ui.languageComboBox->addItem(factory->displayName());
    }

    connect(m_ui.languageComboBox, SIGNAL(currentIndexChanged(int)),
            m_ui.stackedWidget, SLOT(setCurrentIndex(int)));
}


