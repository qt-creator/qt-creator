/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "pathchooserdelegate.h"

#include <utils/pathchooser.h>

#include <QLineEdit>

using namespace Qnx;
using namespace Qnx::Internal;

PathChooserDelegate::PathChooserDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_kind(Utils::PathChooser::ExistingDirectory)
{
}

void PathChooserDelegate::setExpectedKind(Utils::PathChooser::Kind kind)
{
    m_kind = kind;
}

void PathChooserDelegate::setPromptDialogFilter(const QString &filter)
{
    m_filter = filter;
}

QWidget *PathChooserDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    Utils::PathChooser *editor = new Utils::PathChooser(parent);

    editor->setAutoFillBackground(true); // To hide the text beneath the editor widget
    editor->lineEdit()->setMinimumWidth(0);

    connect(editor, SIGNAL(browsingFinished()), this, SLOT(emitCommitData()));

    return editor;
}

void PathChooserDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();

    Utils::PathChooser *pathChooser = qobject_cast<Utils::PathChooser *>(editor);
    if (!pathChooser)
        return;

    pathChooser->setExpectedKind(m_kind);
    pathChooser->setPromptDialogFilter(m_filter);
    pathChooser->setPath(value);
}

void PathChooserDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Utils::PathChooser *pathChooser = qobject_cast<Utils::PathChooser *>(editor);
    if (!pathChooser)
        return;

    model->setData(index, pathChooser->path(), Qt::EditRole);
}

void PathChooserDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);

    editor->setGeometry(option.rect);
}

void PathChooserDelegate::emitCommitData()
{
    emit commitData(qobject_cast<QWidget*>(sender()));
}
