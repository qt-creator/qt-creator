/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "reloadpromptutils.h"

#include <QtGui/QMessageBox>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

using namespace Utils;

QTCREATOR_UTILS_EXPORT Utils::ReloadPromptAnswer
    Utils::reloadPrompt(const QString &fileName, bool modified, QWidget *parent)
{

    const QString title = QCoreApplication::translate("Utils::reloadPrompt", "File Changed");
    QString msg;

    if (modified)
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                                          "The unsaved file %1 has been changed outside Qt Creator. Do you want to reload it and discard your changes?").arg(QDir::toNativeSeparators(fileName));
    else
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                                          "The file %1 has changed outside Qt Creator. Do you want to reload it?").arg(QDir::toNativeSeparators(fileName));
    return reloadPrompt(title, msg, parent);
}

QTCREATOR_UTILS_EXPORT Utils::ReloadPromptAnswer
    Utils::reloadPrompt(const QString &title, const QString &prompt, QWidget *parent)
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
