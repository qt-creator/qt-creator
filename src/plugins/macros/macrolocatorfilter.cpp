/**************************************************************************
**
** Copyright (C) 2015 Nicolas Arnaud-Cormos
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
    setId("Macros");
    setDisplayName(tr("Text Editing Macros"));
    setShortcutString(QLatin1String("rm"));
}

MacroLocatorFilter::~MacroLocatorFilter()
{
}

QList<Core::LocatorFilterEntry> MacroLocatorFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    QList<Core::LocatorFilterEntry> goodEntries;
    QList<Core::LocatorFilterEntry> betterEntries;

    const Qt::CaseSensitivity caseSensitivity_ = caseSensitivity(entry);

    const QMap<QString, Macro*> &macros = MacroManager::macros();
    QMapIterator<QString, Macro*> it(macros);

    while (it.hasNext()) {
        it.next();
        QString name = it.key();

        QList<Core::LocatorFilterEntry> *category = 0;
        if (name.startsWith(entry, caseSensitivity_))
            category = &betterEntries;
        else if (name.contains(entry, caseSensitivity_))
            category = &goodEntries;

        if (category) {
            QVariant id;
            Core::LocatorFilterEntry entry(this, it.key(), id, m_icon);
            entry.extraInfo = it.value()->description();
            category->append(entry);
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void MacroLocatorFilter::accept(Core::LocatorFilterEntry selection) const
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
