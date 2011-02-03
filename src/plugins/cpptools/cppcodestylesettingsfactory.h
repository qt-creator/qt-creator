#ifndef CPPCODESTYLESETTINGSFACTORY_H
#define CPPCODESTYLESETTINGSFACTORY_H

#include <texteditor/icodestylepreferencesfactory.h>

namespace CppTools {

class CppCodeStylePreferencesFactory : public TextEditor::ICodeStylePreferencesFactory
{
public:
    CppCodeStylePreferencesFactory();

    virtual QString languageId();
    virtual QString displayName();
    virtual TextEditor::IFallbackPreferences *createPreferences(const QList<TextEditor::IFallbackPreferences *> &fallbacks) const;
    virtual QWidget *createEditor(TextEditor::IFallbackPreferences *settings,
                                          TextEditor::TabPreferences *tabSettings,
                                          QWidget *parent) const;

};

} // namespace CppTools

#endif // CPPCODESTYLESETTINGSFACTORY_H
