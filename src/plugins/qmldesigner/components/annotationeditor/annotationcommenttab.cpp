/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "annotationcommenttab.h"
#include "ui_annotationcommenttab.h"

#include "richtexteditor/richtexteditor.h"

namespace QmlDesigner {

AnnotationCommentTab::AnnotationCommentTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AnnotationCommentTab)
{
    ui->setupUi(this);

    m_editor = new RichTextEditor;
    ui->formLayout->setWidget(3, QFormLayout::FieldRole, m_editor);

    connect(ui->titleEdit, &QLineEdit::textEdited,
            this, &AnnotationCommentTab::commentTitleChanged);
}

AnnotationCommentTab::~AnnotationCommentTab()
{
    delete ui;
}

Comment AnnotationCommentTab::currentComment() const
{
    Comment result;

    result.setTitle(ui->titleEdit->text().trimmed());
    result.setAuthor(ui->authorEdit->text().trimmed());
    result.setText(m_editor->richText().trimmed());

    if (m_comment.sameContent(result))
        result.setTimestamp(m_comment.timestamp());
    else
        result.updateTimestamp();

    return result;
}

Comment AnnotationCommentTab::originalComment() const
{
    return m_comment;
}

void AnnotationCommentTab::setComment(const Comment &comment)
{
    m_comment = comment;
    resetUI();
}

void AnnotationCommentTab::resetUI()
{
    ui->titleEdit->setText(m_comment.title());
    ui->authorEdit->setText(m_comment.author());
    m_editor->setRichText(m_comment.deescapedText());

    if (m_comment.timestamp() > 0)
        ui->timeLabel->setText(m_comment.timestampStr());
    else
        ui->timeLabel->setText("");
}

void AnnotationCommentTab::resetComment()
{
    m_comment = currentComment();
}

void AnnotationCommentTab::commentTitleChanged(const QString &text)
{
    emit titleChanged(text, this);
}

} //namespace QmlDesigner
