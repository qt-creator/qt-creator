/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "macrolocatorfilter.h"
#include "macromanager.h"
#include "macro.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QPixmap>

using namespace Macros;
using namespace Macros::Internal;

MacroLocatorFilter::MacroLocatorFilter():
    m_icon(QPixmap(QLatin1String(":/macros/images/macro.png")))
{
    setShortcutString(QLatin1String("rm"));
}

MacroLocatorFilter::~MacroLocatorFilter()
{
}

QList<Locator::FilterEntry> MacroLocatorFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    QList<Locator::FilterEntry> result;

    const QMap<QString, Macro*> &macros = MacroManager::instance()->macros();
    QMapIterator<QString, Macro*> it(macros);

    while (it.hasNext()) {
        it.next();
        QString name = it.key();

        if (name.contains(entry)) {
            QVariant id;
            Locator::FilterEntry entry(this, it.key(), id, m_icon);
            entry.extraInfo = it.value()->description();
            result.append(entry);
        }
    }
    return result;
}

void MacroLocatorFilter::accept(Locator::FilterEntry selection) const
{
    // Give the focus back to the editor
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    if (editor)
        editor->widget()->setFocus(Qt::OtherFocusReason);

    MacroManager::instance()->executeMacro(selection.displayName);
}

void MacroLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}
