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
class DefaultAnnotationsModel;

class BasicAnnotationEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BasicAnnotationEditorDialog(QWidget *parent);
    ~BasicAnnotationEditorDialog();

    Annotation const &annotation() const;
    void setAnnotation(const Annotation &annotation);

    void loadDefaultAnnotations(QString const &filename);

    DefaultAnnotationsModel *defaultAnnotations() const;

signals:
    void acceptedDialog(); //use instead of QDialog::accepted

protected:
    virtual void fillFields() = 0;
    virtual void acceptedClicked() = 0;

    Annotation m_annotation;
    std::unique_ptr<DefaultAnnotationsModel> m_defaults;
};

class AnnotationEditorDialog : public BasicAnnotationEditorDialog
{
    Q_OBJECT

public:
    explicit AnnotationEditorDialog(QWidget *parent,
                                    const QString &targetId,
                                    const QString &customId);
    ~AnnotationEditorDialog();

    void setCustomId(const QString &customId);
    QString customId() const;

private slots:
    void acceptedClicked() override;

private:
    void fillFields() override;
    void updateAnnotation();

    void addComment(const Comment &comment);
    void removeComment(int index);

private:
    const QString annotationEditorTitle = {tr("Annotation Editor")};

    Ui::AnnotationEditorDialog *ui;

    QString m_customId;
};

} //namespace QmlDesigner
