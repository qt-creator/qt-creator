#include "tabpreferenceswidget.h"
#include "ui_tabpreferenceswidget.h"
#include "tabpreferences.h"

#include <QtCore/QTextStream>

namespace TextEditor {

TabPreferencesWidget::TabPreferencesWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::TabPreferencesWidget),
    m_tabPreferences(0)
{
    m_ui->setupUi(this);
}

TabPreferencesWidget::~TabPreferencesWidget()
{
    delete m_ui;
}

void TabPreferencesWidget::setTabPreferences(TabPreferences *tabPreferences)
{
    if (m_tabPreferences == tabPreferences)
        return; // nothing changes

    // cleanup old
    if (m_tabPreferences) {
        disconnect(m_tabPreferences, SIGNAL(settingsChanged(TabSettings)),
                m_ui->tabSettingsWidget, SLOT(setSettings(TabSettings)));
        disconnect(m_tabPreferences, SIGNAL(currentFallbackChanged(IFallbackPreferences*)),
                this, SLOT(slotCurrentFallbackChanged(IFallbackPreferences*)));
        disconnect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TabSettings)),
                m_tabPreferences, SLOT(setSettings(TabSettings)));
        m_ui->tabSettingsWidget->setEnabled(true);
    }
    m_tabPreferences = tabPreferences;
    m_ui->fallbackWidget->setFallbackPreferences(tabPreferences);
    // fillup new
    if (m_tabPreferences) {
        slotCurrentFallbackChanged(m_tabPreferences->currentFallback());

        connect(m_tabPreferences, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                m_ui->tabSettingsWidget, SLOT(setSettings(TextEditor::TabSettings)));
        connect(m_tabPreferences, SIGNAL(currentFallbackChanged(TextEditor::IFallbackPreferences*)),
                this, SLOT(slotCurrentFallbackChanged(TextEditor::IFallbackPreferences*)));
        connect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                m_tabPreferences, SLOT(setSettings(TextEditor::TabSettings)));

        m_ui->tabSettingsWidget->setSettings(m_tabPreferences->settings());
    }
}

void TabPreferencesWidget::slotCurrentFallbackChanged(TextEditor::IFallbackPreferences *fallback)
{
    m_ui->tabSettingsWidget->setEnabled(!fallback);
}

QString TabPreferencesWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << m_ui->fallbackWidget->searchKeywords()
            << sep << m_ui->tabSettingsWidget->searchKeywords()
               ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

void TabPreferencesWidget::setFallbacksVisible(bool on)
{
    m_ui->fallbackWidget->setFallbacksVisible(on);
}

void TabPreferencesWidget::setFlat(bool on)
{
     m_ui->tabSettingsWidget->setFlat(on);
}

void TabPreferencesWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace TextEditor
