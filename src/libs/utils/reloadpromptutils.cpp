// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "reloadpromptutils.h"

#include "fileutils.h"
#include "hostosinfo.h"
#include "utilstr.h"

#include <QDir>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPushButton>

namespace Utils {

QTCREATOR_UTILS_EXPORT ReloadPromptAnswer reloadPrompt(const FilePath &fileName,
                                                       bool modified,
                                                       bool enableDiffOption,
                                                       QWidget *parent)
{

    const QString title = Tr::tr("File Changed");
    QString msg;

    if (modified) {
        msg = Tr::tr("The unsaved file <i>%1</i> has been changed on disk. "
                     "Do you want to reload it and discard your changes?");
    } else {
        msg = Tr::tr("The file <i>%1</i> has been changed on disk. Do you want to reload it?");
    }
    msg = "<p>" + msg.arg(fileName.fileName()) + "</p><p>";
    if (HostOsInfo::isMacHost()) {
        msg += Tr::tr("The default behavior can be set in %1 > Preferences > Environment > System.",
                      "macOS")
                   .arg(QGuiApplication::applicationDisplayName());
    } else {
        msg += Tr::tr("The default behavior can be set in Edit > Preferences > Environment > System.");
    }
    msg += "</p>";
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

    msg.button(QMessageBox::Close)->setText(Tr::tr("&Close"));

    QPushButton *diffButton = nullptr;
    if (enableDiffOption)
        diffButton = msg.addButton(Tr::tr("No to All && &Diff"), QMessageBox::NoRole);

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
        fileDeletedPrompt(const QString &fileName, QWidget *parent)
{
    const QString title = Tr::tr("File Has Been Removed");
    const QString msg = Tr::tr("The file %1 has been removed from disk. "
                               "Do you want to save it under a different name, or close "
                               "the editor?").arg(QDir::toNativeSeparators(fileName));
    QMessageBox box(QMessageBox::Question, title, msg, QMessageBox::NoButton, parent);
    QPushButton *close =
            box.addButton(Tr::tr("&Close"), QMessageBox::RejectRole);
    QPushButton *closeAll =
            box.addButton(Tr::tr("C&lose All"), QMessageBox::RejectRole);
    QPushButton *saveas =
            box.addButton(Tr::tr("Save &as..."), QMessageBox::ActionRole);
    QPushButton *save =
            box.addButton(Tr::tr("&Save"), QMessageBox::AcceptRole);
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
