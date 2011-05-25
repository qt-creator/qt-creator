#include "qmljscodestylesettingsfactory.h"
#include "qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include <texteditor/tabpreferences.h>
#include <texteditor/tabsettings.h>
#include <QtGui/QLayout>

using namespace QmlJSTools;

QmlJSCodeStylePreferencesFactory::QmlJSCodeStylePreferencesFactory()
{
}

QString QmlJSCodeStylePreferencesFactory::languageId()
{
    return Constants::QML_JS_SETTINGS_ID;
}

QString QmlJSCodeStylePreferencesFactory::displayName()
{
    return Constants::QML_JS_SETTINGS_NAME;
}

TextEditor::IFallbackPreferences *QmlJSCodeStylePreferencesFactory::createPreferences(
    const QList<TextEditor::IFallbackPreferences *> &fallbacks) const
{
    return 0;
}

QWidget *QmlJSCodeStylePreferencesFactory::createEditor(TextEditor::IFallbackPreferences *preferences,
                                                           TextEditor::TabPreferences *tabPreferences,
                                                           QWidget *parent) const
{
    Q_UNUSED(preferences)

    Internal::QmlJSCodeStylePreferencesWidget *widget = new Internal::QmlJSCodeStylePreferencesWidget(parent);
    widget->layout()->setMargin(0);
    widget->setTabPreferences(tabPreferences);
    return widget;
}

