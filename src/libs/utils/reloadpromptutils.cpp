/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "reloadpromptutils.h"

#include <QtGui/QMessageBox>

using namespace Core;
using namespace Core::Utils;

QWORKBENCH_UTILS_EXPORT Core::Utils::ReloadPromptAnswer
    Core::Utils::reloadPrompt(const QString &fileName, QWidget *parent)
{
    return reloadPrompt(QObject::tr("File Changed"),
                        QObject::tr("The file %1 has changed outside Qt Creator. Do you want to reload it?").arg(fileName),
                        parent);
}

QWORKBENCH_UTILS_EXPORT Core::Utils::ReloadPromptAnswer
    Core::Utils::reloadPrompt(const QString &title, const QString &prompt, QWidget *parent)
{
    switch (QMessageBox::question(parent, title, prompt,
                                  QMessageBox::Yes|QMessageBox::YesToAll|QMessageBox::No|QMessageBox::NoToAll,
                                  QMessageBox::YesToAll)) {
    case QMessageBox::Yes:
        return  ReloadCurrent;
    case QMessageBox::YesToAll:
        return ReloadAll;
    case QMessageBox::No:
        return ReloadSkipCurrent;
    default:
        break;
    }
    return ReloadNone;
}
