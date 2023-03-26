// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    int compareFileChecksum(const QString &firstFile, const QString &secondFile);

private:
    std::unique_ptr<Ui::AnnotationCommentTab> ui;
    RichTextEditor *m_editor;

    Comment m_comment;
    QPointer<DefaultAnnotationsModel> m_defaults;
};

} //namespace QmlDesigner
