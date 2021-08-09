/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <annotation.h>

#include <QWidget>
#include <QPointer>

#include <memory>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Ui {
class AnnotationCommentTab;
}

class RichTextEditor;
class DefaultAnnotationsModel;

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

    DefaultAnnotationsModel *defaultAnnotations() const;
    void setDefaultAnnotations(DefaultAnnotationsModel *);

signals:
    void titleChanged(const QString &text, QWidget *widget);

private:
    QString backupFile(const QString &filePath);
    void ensureDir(const QDir &dir);
    int compareFileChecksum(const QString &firstFile, const QString &secondFile);

private:
    std::unique_ptr<Ui::AnnotationCommentTab> ui;
    RichTextEditor *m_editor;

    Comment m_comment;
    QPointer<DefaultAnnotationsModel> m_defaults;
};

} //namespace QmlDesigner
