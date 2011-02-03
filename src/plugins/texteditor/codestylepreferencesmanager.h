#ifndef CODESTYLEPREFERENCESMANAGER_H
#define CODESTYLEPREFERENCESMANAGER_H

#include "texteditor_global.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace TextEditor {

namespace Internal {
class CodeStylePreferencesManagerPrivate;
}

class ICodeStylePreferencesFactory;

class TEXTEDITOR_EXPORT CodeStylePreferencesManager : public QObject
{
    Q_OBJECT
public:
    static CodeStylePreferencesManager *instance();

    void registerFactory(ICodeStylePreferencesFactory *settings);
    QList<ICodeStylePreferencesFactory *> factories() const;
    ICodeStylePreferencesFactory *factory(const QString &languageId) const;

private:
    CodeStylePreferencesManager();
    ~CodeStylePreferencesManager();
    Internal::CodeStylePreferencesManagerPrivate *d;
    static CodeStylePreferencesManager *m_instance;
};

} // namespace TextEditor

#endif // CODESTYLEPREFERENCESMANAGER_H
