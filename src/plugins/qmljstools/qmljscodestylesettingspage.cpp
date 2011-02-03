#include "qmljscodestylesettingspage.h"
#include "ui_qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljsindenter.h"
#include "qmljsqtstylecodeformatter.h"

#include <texteditor/snippets/isnippetprovider.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/tabpreferences.h>
#include <texteditor/displaysettings.h>
#include <texteditor/texteditorsettings.h>
#include <extensionsystem/pluginmanager.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <coreplugin/icore.h>

#include <QtCore/QTextStream>

using namespace TextEditor;

namespace QmlJSTools {
namespace Internal {

// ------------------ CppCodeStyleSettingsWidget

QmlJSCodeStyleSettingsWidget::QmlJSCodeStyleSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_tabPreferences(0),
    m_ui(new ::Ui::QmlJSCodeStyleSettingsPage)
{
    m_ui->setupUi(this);

    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::instance()->getObjects<ISnippetProvider>();
    foreach (ISnippetProvider *provider, providers) {
        if (provider->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID)) {
            provider->decorateEditor(m_ui->previewTextEdit);
            break;
        }
    }
    TextEditor::TextEditorSettings *settings = TextEditorSettings::instance();
    setFontSettings(settings->fontSettings());
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
       this, SLOT(setFontSettings(TextEditor::FontSettings)));

    setVisualizeWhitespace(true);

    updatePreview();
}

QmlJSCodeStyleSettingsWidget::~QmlJSCodeStyleSettingsWidget()
{
    delete m_ui;
}

void QmlJSCodeStyleSettingsWidget::setTabPreferences(TextEditor::TabPreferences *tabPreferences)
{
    m_tabPreferences = tabPreferences;
    m_ui->tabPreferencesWidget->setTabPreferences(tabPreferences);
    connect(m_tabPreferences, SIGNAL(currentSettingsChanged(TextEditor::TabSettings)),
            this, SLOT(slotTabSettingsChanged()));
    updatePreview();
}


QString QmlJSCodeStyleSettingsWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
       << sep << m_ui->tabPreferencesWidget->searchKeywords()
          ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

void QmlJSCodeStyleSettingsWidget::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    m_ui->previewTextEdit->setFont(fontSettings.font());
}

void QmlJSCodeStyleSettingsWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettings displaySettings = m_ui->previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_ui->previewTextEdit->setDisplaySettings(displaySettings);
}

void QmlJSCodeStyleSettingsWidget::slotTabSettingsChanged()
{
    updatePreview();
}

void QmlJSCodeStyleSettingsWidget::updatePreview()
{
    QTextDocument *doc = m_ui->previewTextEdit->document();

    const TextEditor::TabSettings &ts = m_tabPreferences
            ? m_tabPreferences->currentSettings() : TextEditorSettings::instance()->tabPreferences()->settings();
    m_ui->previewTextEdit->setTabSettings(ts);
    QtStyleCodeFormatter formatter(ts);
    formatter.invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_ui->previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        int depth = formatter.indentFor(block);
        ts.indentLine(block, depth);
        formatter.updateLineStateChange(block);

        block = block.next();
    }
    tc.endEditBlock();
}

// ------------------ CppCodeStyleSettingsPage

QmlJSCodeStyleSettingsPage::QmlJSCodeStyleSettingsPage(/*QSharedPointer<CppFileSettings> &settings,*/
                     QWidget *parent) :
    Core::IOptionsPage(parent)/*,
    m_settings(settings)*/
{
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        TextEditor::TabSettings tabSettings;
        // read it from old global settings for the first time?
        tabSettings.fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
//        TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
//        textEditorSettings->setTabSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID,
//                                           tabSettings);
    }
}

QmlJSCodeStyleSettingsPage::~QmlJSCodeStyleSettingsPage()
{
}

QString QmlJSCodeStyleSettingsPage::id() const
{
    return QLatin1String(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
}

QString QmlJSCodeStyleSettingsPage::displayName() const
{
    return QCoreApplication::translate("QmlJSTools", Constants::QML_JS_CODE_STYLE_SETTINGS_NAME);
}

QString QmlJSCodeStyleSettingsPage::category() const
{
    return QLatin1String("Qt Quick");
}

QString QmlJSCodeStyleSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Qt Quick", "Qt Quick");
}

QIcon QmlJSCodeStyleSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(QmlDesigner::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *QmlJSCodeStyleSettingsPage::createPage(QWidget *parent)
{
    m_widget = new QmlJSCodeStyleSettingsWidget(parent);
    m_widget->setTabPreferences(
                TextEditorSettings::instance()->tabPreferences(
                    QmlJSTools::Constants::QML_JS_SETTINGS_ID));

    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void QmlJSCodeStyleSettingsPage::apply()
{
//    if (m_widget) {
//        const TabSettings newTabSettings = m_widget->tabSettings();
//        TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
//        if (newTabSettings != textEditorSettings->tabSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID)) {
//            textEditorSettings->setTabSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID,
//                                               newTabSettings);
//            if (QSettings *s = Core::ICore::instance()->settings()) {
//                newTabSettings.toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
//            }
//        }
//    }
}

bool QmlJSCodeStyleSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace CppTools
