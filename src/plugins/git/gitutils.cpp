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

#include "gitutils.h"

#include <QInputDialog>
#include <QLineEdit>

namespace Git {
namespace Internal {

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
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setWindowTitle(title);
    dialog.setLabelText(prompt);
    dialog.setTextValue(*s);
    // Nasty hack:
    if (QLineEdit *le = dialog.findChild<QLineEdit*>())
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

} // namespace Internal
} // namespace Git
