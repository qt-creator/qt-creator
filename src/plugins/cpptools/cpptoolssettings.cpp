#include "cpptoolssettings.h"
#include "cpptoolsconstants.h"
#include "cppcodestylepreferences.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/tabpreferences.h>

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <QtCore/QSettings>

static const char *idKey = "CppGlobal";

using namespace CppTools;

namespace CppTools {
namespace Internal {

class CppToolsSettingsPrivate
{
public:
    CppCodeStylePreferences *m_cppCodeStylePreferences;
    TextEditor::TabPreferences *m_tabPreferences;
};

} // namespace Internal
} // namespace CppTools

CppToolsSettings *CppToolsSettings::m_instance = 0;

CppToolsSettings::CppToolsSettings(QObject *parent)
    : QObject(parent)
    , m_d(new Internal::CppToolsSettingsPrivate)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    if (const QSettings *s = Core::ICore::instance()->settings()) {
        TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
        m_d->m_tabPreferences
                = new TextEditor::TabPreferences(QList<TextEditor::IFallbackPreferences *>()
                                                 << textEditorSettings->tabPreferences(), this);
        m_d->m_tabPreferences->setCurrentFallback(textEditorSettings->tabPreferences());
        m_d->m_tabPreferences->fromSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        m_d->m_tabPreferences->setDisplayName(tr("global C++"));
        m_d->m_tabPreferences->setId(idKey);
        textEditorSettings->registerLanguageTabPreferences(CppTools::Constants::CPP_SETTINGS_ID, m_d->m_tabPreferences);

        m_d->m_cppCodeStylePreferences
                = new CppCodeStylePreferences(QList<TextEditor::IFallbackPreferences *>(), this);
        m_d->m_cppCodeStylePreferences->fromSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        m_d->m_cppCodeStylePreferences->setDisplayName(tr("global C++"));
        m_d->m_cppCodeStylePreferences->setId(idKey);
        textEditorSettings->registerLanguageCodeStylePreferences(CppTools::Constants::CPP_SETTINGS_ID, m_d->m_cppCodeStylePreferences);
    }
}

CppToolsSettings::~CppToolsSettings()
{
    delete m_d;

    m_instance = 0;
}

CppToolsSettings *CppToolsSettings::instance()
{
    return m_instance;
}

CppCodeStylePreferences *CppToolsSettings::cppCodeStylePreferences() const
{
    return m_d->m_cppCodeStylePreferences;
}

TextEditor::TabPreferences *CppToolsSettings::tabPreferences() const
{
    return m_d->m_tabPreferences;
}


