// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitutils.h"

#include <QInputDialog>
#include <QLineEdit>

namespace Git::Internal {

void Stash::clear()
{
    name.clear();
    branch.clear();
    message.clear();
}

/* Parse a stash line in its 2 manifestations (with message/without message containing
 * <base_sha1>+subject):
\code
stash@{1}: WIP on <branch>: <base_sha1> <subject_base_sha1>
stash@{2}: On <branch>: <message>
\endcode */

bool Stash::parseStashLine(const QString &l)
{
    const QChar colon = ':';
    const int branchPos = l.indexOf(colon);
    if (branchPos < 0)
        return false;
    const int messagePos = l.indexOf(colon, branchPos + 1);
    if (messagePos < 0)
        return false;
    // Branch spec
    const int onIndex = l.indexOf("on ", branchPos + 2, Qt::CaseInsensitive);
    if (onIndex == -1 || onIndex >= messagePos)
        return false;
    // Happy!
    name = l.left(branchPos);
    branch = l.mid(onIndex + 3, messagePos - onIndex - 3);
    message = l.mid(messagePos + 2); // skip blank
    return true;
}

// Make QInputDialog  play nicely, widen it a bit.
bool inputText(QWidget *parent, const QString &title, const QString &prompt, QString *s)
{
    QInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setLabelText(prompt);
    dialog.setTextValue(*s);
    // Nasty hack:
    if (auto le = dialog.findChild<QLineEdit*>())
        le->setMinimumWidth(500);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    *s = dialog.textValue();
    return true;
}

static inline QString versionPart(unsigned part)
{
    return QString::number(part & 0xff, 16);
}

QString versionString(unsigned ver)
{
    return QString::fromLatin1("%1.%2.%3")
            .arg(versionPart(ver >> 16))
            .arg(versionPart(ver >> 8))
            .arg(versionPart(ver));
}

} // Git::Internal
