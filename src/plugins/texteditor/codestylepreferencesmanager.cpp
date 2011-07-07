/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

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

