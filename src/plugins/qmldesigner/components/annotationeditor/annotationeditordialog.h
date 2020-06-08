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

#include <QDialog>

#include "annotation.h"

namespace QmlDesigner {

namespace Ui {
class AnnotationEditorDialog;
}

class AnnotationEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnnotationEditorDialog(QWidget *parent, const QString &targetId, const QString &customId, const Annotation &annotation);
    ~AnnotationEditorDialog();

    void setAnnotation(const Annotation &annotation);
    Annotation annotation() const;

    void setCustomId(const QString &customId);
    QString customId() const;

signals:
    void accepted();

private slots:
    void acceptedClicked();
    void tabChanged(int index);
    void commentTitleChanged(const QString &text, QWidget *tab);

private:
    void fillFields();
    void setupComments();
    void addComment(const Comment &comment);
    void removeComment(int index);

    void addCommentTab(const Comment &comment);
    void removeCommentTab(int index);
    void deleteAllTabs();

private:
    const QString annotationEditorTitle = {tr("Annotation Editor")};
    const QString defaultTabName = {tr("Annotation")};
    Ui::AnnotationEditorDialog *ui;

    QString m_customId;
    Annotation m_annotation;
};

} //namespace QmlDesigner
