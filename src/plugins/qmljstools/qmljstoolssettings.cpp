#include "qmljstoolssettings.h"
#include "qmljstoolsconstants.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/tabpreferences.h>

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <QtCore/QSettings>

static const char *idKey = "QmlJSGlobal";

using namespace QmlJSTools;

namespace QmlJSTools {
namespace Internal {

class QmlJSToolsSettingsPrivate
{
public:
    TextEditor::TabPreferences *m_tabPreferences;
};

} // namespace Internal
} // namespace QmlJSTools

QmlJSToolsSettings *QmlJSToolsSettings::m_instance = 0;

QmlJSToolsSettings::QmlJSToolsSettings(QObject *parent)
    : QObject(parent)
    , m_d(new Internal::QmlJSToolsSettingsPrivate)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    if (const QSettings *s = Core::ICore::instance()->settings()) {
        TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
        m_d->m_tabPreferences
                = new TextEditor::TabPreferences(QList<TextEditor::IFallbackPreferences *>()
                                                 << textEditorSettings->tabPreferences(), this);
        m_d->m_tabPreferences->setCurrentFallback(textEditorSettings->tabPreferences());
        m_d->m_tabPreferences->fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
        m_d->m_tabPreferences->setDisplayName(tr("global QML"));
        m_d->m_tabPreferences->setId(idKey);
        textEditorSettings->registerLanguageTabPreferences(QmlJSTools::Constants::QML_JS_SETTINGS_ID, m_d->m_tabPreferences);
    }
}

QmlJSToolsSettings::~QmlJSToolsSettings()
{
    delete m_d;

    m_instance = 0;
}

QmlJSToolsSettings *QmlJSToolsSettings::instance()
{
    return m_instance;
}

TextEditor::TabPreferences *QmlJSToolsSettings::tabPreferences() const
{
    return m_d->m_tabPreferences;
}


