#include "cppcodestylesettingsfactory.h"
#include "cppcodestylesettings.h"
#include "cppcodestylesettingspage.h"
#include "cppcodestylepreferences.h"
#include "cpptoolsconstants.h"
#include <texteditor/tabpreferences.h>
#include <texteditor/tabsettings.h>
#include <QtGui/QLayout>

using namespace CppTools;

CppCodeStylePreferencesFactory::CppCodeStylePreferencesFactory()
{
}

QString CppCodeStylePreferencesFactory::languageId()
{
    return Constants::CPP_SETTINGS_ID;
}

QString CppCodeStylePreferencesFactory::displayName()
{
    return Constants::CPP_SETTINGS_NAME;
}

TextEditor::IFallbackPreferences *CppCodeStylePreferencesFactory::createPreferences(
    const QList<TextEditor::IFallbackPreferences *> &fallbacks) const
{
    return new CppCodeStylePreferences(fallbacks);
}

QWidget *CppCodeStylePreferencesFactory::createEditor(TextEditor::IFallbackPreferences *preferences,
                                                           TextEditor::TabPreferences *tabPreferences,
                                                           QWidget *parent) const
{
    CppCodeStylePreferences *cppPreferences = qobject_cast<CppCodeStylePreferences *>(preferences);
    if (!cppPreferences)
        return 0;
    Internal::CppCodeStylePreferencesWidget *widget = new Internal::CppCodeStylePreferencesWidget(parent);
    widget->layout()->setMargin(0);
    widget->setCppCodeStylePreferences(cppPreferences);
    widget->setTabPreferences(tabPreferences);
    return widget;
}

