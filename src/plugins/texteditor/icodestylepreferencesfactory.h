#ifndef ICODESTYLEPREFERENCESFACTORY_H
#define ICODESTYLEPREFERENCESFACTORY_H

#include "texteditor_global.h"

#include <QtCore/QObject>

namespace TextEditor {

class IFallbackPreferences;
class TabPreferences;

class TEXTEDITOR_EXPORT ICodeStylePreferencesFactory : public QObject
{
    Q_OBJECT
public:
    explicit ICodeStylePreferencesFactory(QObject *parent = 0);

    virtual QString languageId() = 0;
    virtual QString displayName() = 0;
    virtual IFallbackPreferences *createPreferences(const QList<IFallbackPreferences *> &fallbacks) const = 0;
    virtual QWidget *createEditor(IFallbackPreferences *preferences, TabPreferences *tabSettings, QWidget *parent) const = 0;
};

} // namespace TextEditor

#endif // ICODESTYLEPREFERENCESFACTORY_H
