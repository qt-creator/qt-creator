#ifndef QMLJSCODESTYLESETTINGSFACTORY_H
#define QMLJSCODESTYLESETTINGSFACTORY_H

#include <texteditor/icodestylepreferencesfactory.h>

namespace QmlJSTools {

class QmlJSCodeStylePreferencesFactory : public TextEditor::ICodeStylePreferencesFactory
{
public:
    QmlJSCodeStylePreferencesFactory();

    virtual QString languageId();
    virtual QString displayName();
    virtual TextEditor::IFallbackPreferences *createPreferences(const QList<TextEditor::IFallbackPreferences *> &fallbacks) const;
    virtual QWidget *createEditor(TextEditor::IFallbackPreferences *settings,
                                          TextEditor::TabPreferences *tabSettings,
                                          QWidget *parent) const;

};

} // namespace QmlJSTools

#endif // QMLJSCODESTYLESETTINGSFACTORY_H
