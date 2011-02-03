#include "codestylepreferencesmanager.h"
#include "icodestylepreferencesfactory.h"

using namespace TextEditor;

CodeStylePreferencesManager *CodeStylePreferencesManager::m_instance = 0;

namespace TextEditor {
namespace Internal {

class CodeStylePreferencesManagerPrivate
{
public:
    QMap<QString, ICodeStylePreferencesFactory *> m_idToFactory;
    QList<ICodeStylePreferencesFactory *> m_factories;
};

} // namespace Internal
} // namespace TextEditor

CodeStylePreferencesManager::CodeStylePreferencesManager()
    : QObject(),
      d(new Internal::CodeStylePreferencesManagerPrivate())
{
}

CodeStylePreferencesManager::~CodeStylePreferencesManager()
{
    delete d;
}

CodeStylePreferencesManager *CodeStylePreferencesManager::instance()
{
    if (!m_instance)
        m_instance = new CodeStylePreferencesManager();
    return m_instance;
}

void CodeStylePreferencesManager::registerFactory(ICodeStylePreferencesFactory *settings)
{
    d->m_idToFactory.insert(settings->languageId(), settings);
    d->m_factories = d->m_idToFactory.values();
}

QList<ICodeStylePreferencesFactory *> CodeStylePreferencesManager::factories() const
{
    return d->m_factories;
}

ICodeStylePreferencesFactory *CodeStylePreferencesManager::factory(const QString &languageId) const
{
    return d->m_idToFactory.value(languageId);
}

