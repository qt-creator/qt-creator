/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "reloadpromptutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QAbstractButton>

using namespace Utils;

QTCREATOR_UTILS_EXPORT Utils::ReloadPromptAnswer
    Utils::reloadPrompt(const QString &fileName, bool modified, QWidget *parent)
{

    const QString title = QCoreApplication::translate("Utils::reloadPrompt", "File Changed");
    QString msg;

    if (modified)
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                                          "The unsaved file <i>%1</i> has been changed outside Qt Creator. Do you want to reload it and discard your changes?");
    else
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                                          "The file <i>%1</i> has changed outside Qt Creator. Do you want to reload it?");
    msg = msg.arg(QFileInfo(fileName).fileName());
    return reloadPrompt(title, msg, QDir::toNativeSeparators(fileName), parent);
}

QTCREATOR_UTILS_EXPORT Utils::ReloadPromptAnswer
    Utils::reloadPrompt(const QString &title, const QString &prompt, const QString &details, QWidget *parent)
{
    QMessageBox msg(parent);
    msg.setStandardButtons(QMessageBox::Yes|QMessageBox::YesToAll|QMessageBox::Close|QMessageBox::No|QMessageBox::NoToAll);
    msg.setDefaultButton(QMessageBox::YesToAll);
    msg.setWindowTitle(title);
    msg.setText(prompt);
    msg.setDetailedText(details);

    switch (msg.exec()) {
    case QMessageBox::Yes:
        return  ReloadCurrent;
    case QMessageBox::YesToAll:
        return ReloadAll;
    case QMessageBox::No:
        return ReloadSkipCurrent;
    case QMessageBox::Close:
        return CloseCurrent;
    default:
        break;
    }
    return ReloadNone;
}

QTCREATOR_UTILS_EXPORT Utils::FileDeletedPromptAnswer
        Utils::fileDeletedPrompt(const QString &fileName, bool triggerExternally, QWidget *parent)
{
    const QString title = QCoreApplication::translate("Utils::fileDeletedPrompt", "File has been removed");
    QString msg;
    if (triggerExternally)
        msg = QCoreApplication::translate("Utils::fileDeletedPrompt",
                                          "The file %1 has been removed outside Qt Creator. Do you want to save it under a different name, or close the editor?").arg(QDir::toNativeSeparators(fileName));
    else
        msg = QCoreApplication::translate("Utils::fileDeletedPrompt",
                                          "The file %1 was removed. Do you want to save it under a different name, or close the editor?").arg(QDir::toNativeSeparators(fileName));
    QMessageBox box(QMessageBox::Question, title, msg, QMessageBox::NoButton, parent);
    QPushButton *close = box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "&Close"), QMessageBox::RejectRole);
    QPushButton *saveas = box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "Save &as..."), QMessageBox::ActionRole);
    QPushButton *save = box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "&Save"), QMessageBox::AcceptRole);
    box.setDefaultButton(saveas);
    box.exec();
    QAbstractButton *clickedbutton = box.clickedButton();
    if (clickedbutton == close)
        return FileDeletedClose;
    else if (clickedbutton == saveas)
        return FileDeletedSaveAs;
    else if (clickedbutton == save)
        return FileDeletedSave;
    return FileDeletedClose;
}
