/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cdbsymbolpathlisteditor.h"

#include <QtGui/QFileDialog>
#include <QtGui/QAction>

namespace Debugger {
namespace Internal {

CdbSymbolPathListEditor::CdbSymbolPathListEditor(QWidget *parent) :
    Utils::PathListEditor(parent)
{
    //! Add Microsoft Symbol server connection
    QAction *action = insertAction(lastAddActionIndex() + 1, tr("Symbol Server..."), this, SLOT(addSymbolServer()));
    action->setToolTip(tr("Adds the Microsoft symbol server providing symbols for operating system libraries."
                      "Requires specifying a local cache directory."));
}

void CdbSymbolPathListEditor::addSymbolServer()
{
    const QString title = tr("Pick a local cache directory");
    const QString cacheDir = QFileDialog::getExistingDirectory(this, title);
    if (!cacheDir.isEmpty()) {
        const QString path = QString::fromLatin1("symsrv*symsrv.dll*%1*http://msdl.microsoft.com/download/symbols").
                             arg(QDir::toNativeSeparators(cacheDir));
        insertPathAtCursor(path);
    }
}

} // namespace Internal
} // namespace Debugger
