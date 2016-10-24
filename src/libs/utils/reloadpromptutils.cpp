/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "fileutils.h"
#include "reloadpromptutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>

namespace Utils {

QTCREATOR_UTILS_EXPORT ReloadPromptAnswer reloadPrompt(const FileName &fileName,
                                                       bool modified,
                                                       bool enableDiffOption,
                                                       QWidget *parent)
{

    const QString title = QCoreApplication::translate("Utils::reloadPrompt", "File Changed");
    QString msg;

    if (modified) {
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                "The unsaved file <i>%1</i> has changed outside Qt Creator. "
                "Do you want to reload it and discard your changes?");
    } else {
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                "The file <i>%1</i> has changed outside Qt Creator. Do you want to reload it?");
    }
    msg = msg.arg(fileName.fileName());
    return reloadPrompt(title, msg, fileName.toUserOutput(), enableDiffOption, parent);
}

QTCREATOR_UTILS_EXPORT ReloadPromptAnswer reloadPrompt(const QString &title,
                                                       const QString &prompt,
                                                       const QString &details,
                                                       bool enableDiffOption,
                                                       QWidget *parent)
{
    QMessageBox msg(parent);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Close
                           | QMessageBox::No | QMessageBox::NoToAll);
    msg.setDefaultButton(QMessageBox::YesToAll);
    msg.setWindowTitle(title);
    msg.setText(prompt);
    msg.setDetailedText(details);

    msg.button(QMessageBox::Close)->setText(QCoreApplication::translate("Utils::reloadPrompt",
                                                                        "&Close"));

    QPushButton *diffButton = nullptr;
    if (enableDiffOption) {
        diffButton = msg.addButton(QCoreApplication::translate(
                                   "Utils::reloadPrompt", "No to All && &Diff"),
                                   QMessageBox::NoRole);
    }

    const int result = msg.exec();

    if (msg.clickedButton() == diffButton)
        return ReloadNoneAndDiff;

    switch (result) {
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

QTCREATOR_UTILS_EXPORT FileDeletedPromptAnswer
        fileDeletedPrompt(const QString &fileName, bool triggerExternally, QWidget *parent)
{
    const QString title = QCoreApplication::translate("Utils::fileDeletedPrompt",
                                                      "File has been removed");
    QString msg;
    if (triggerExternally) {
        msg = QCoreApplication::translate("Utils::fileDeletedPrompt",
                                          "The file %1 has been removed outside Qt Creator. "
                                          "Do you want to save it under a different name, or close "
                                          "the editor?").arg(QDir::toNativeSeparators(fileName));
    } else {
        msg = QCoreApplication::translate("Utils::fileDeletedPrompt",
                                          "The file %1 was removed. "
                                          "Do you want to save it under a different name, or close "
                                          "the editor?").arg(QDir::toNativeSeparators(fileName));
    }
    QMessageBox box(QMessageBox::Question, title, msg, QMessageBox::NoButton, parent);
    QPushButton *close =
            box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "&Close"),
                          QMessageBox::RejectRole);
    QPushButton *closeAll =
            box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "C&lose All"),
                          QMessageBox::RejectRole);
    QPushButton *saveas =
            box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "Save &as..."),
                          QMessageBox::ActionRole);
    QPushButton *save =
            box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "&Save"),
                          QMessageBox::AcceptRole);
    box.setDefaultButton(saveas);
    box.exec();
    QAbstractButton *clickedbutton = box.clickedButton();
    if (clickedbutton == close)
        return FileDeletedClose;
    else if (clickedbutton == closeAll)
        return FileDeletedCloseAll;
    else if (clickedbutton == saveas)
        return FileDeletedSaveAs;
    else if (clickedbutton == save)
        return FileDeletedSave;
    return FileDeletedClose;
}

} // namespace Utils
