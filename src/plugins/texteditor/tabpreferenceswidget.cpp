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
    m_ui->fallbackWidget->setLabelText(tr("Tab settings:"));
    m_ui->tabSettingsWidget->setEnabled(false);
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
        disconnect(m_tabPreferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
                m_ui->tabSettingsWidget, SLOT(setSettings(TextEditor::TabSettings)));
        disconnect(m_tabPreferences, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences*)),
                this, SLOT(slotCurrentPreferencesChanged(TextEditor::IFallbackPreferences*)));
        disconnect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
    }
    m_tabPreferences = tabPreferences;
    m_ui->fallbackWidget->setFallbackPreferences(tabPreferences);
    // fillup new
    if (m_tabPreferences) {
        slotCurrentPreferencesChanged(m_tabPreferences->currentPreferences());
        m_ui->tabSettingsWidget->setSettings(m_tabPreferences->currentSettings());

        connect(m_tabPreferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
                m_ui->tabSettingsWidget, SLOT(setSettings(TextEditor::TabSettings)));
        connect(m_tabPreferences, SIGNAL(currentPreferencesChanged(TextEditor::IFallbackPreferences*)),
                this, SLOT(slotCurrentPreferencesChanged(TextEditor::IFallbackPreferences*)));
        connect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
                this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
    } else {
        m_ui->tabSettingsWidget->setEnabled(false);
    }
}

void TabPreferencesWidget::slotCurrentPreferencesChanged(TextEditor::IFallbackPreferences *preferences)
{
    m_ui->tabSettingsWidget->setEnabled(!preferences->isReadOnly() && m_tabPreferences->isFallbackEnabled(m_tabPreferences->currentFallback()));
}

void TabPreferencesWidget::slotTabSettingsChanged(const TextEditor::TabSettings &settings)
{
    if (!m_tabPreferences)
        return;

    TabPreferences *current = qobject_cast<TabPreferences *>(m_tabPreferences->currentPreferences());
    if (!current)
        return;

    current->setSettings(settings);
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
