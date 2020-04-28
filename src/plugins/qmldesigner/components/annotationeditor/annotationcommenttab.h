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

#pragma once

#include <QWidget>

#include "annotation.h"

namespace QmlDesigner {

namespace Ui {
class AnnotationCommentTab;
}

class RichTextEditor;

class AnnotationCommentTab : public QWidget
{
    Q_OBJECT

public:
    explicit AnnotationCommentTab(QWidget *parent = nullptr);
    ~AnnotationCommentTab();

    Comment currentComment() const;

    Comment originalComment() const;
    void setComment(const Comment &comment);

    void resetUI();
    void resetComment();

signals:
    void titleChanged(const QString &text, QWidget *widget);

private slots:
    void commentTitleChanged(const QString &text);

private:
    Ui::AnnotationCommentTab *ui;
    RichTextEditor *m_editor;

    Comment m_comment;
};

} //namespace QmlDesigner
