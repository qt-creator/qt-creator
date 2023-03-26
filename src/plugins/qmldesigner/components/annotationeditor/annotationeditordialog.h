// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "annotation.h"

#include <QDialog>


QT_BEGIN_NAMESPACE
class QAbstractButton;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class DefaultAnnotationsModel;
class AnnotationEditorWidget;

class AnnotationEditorDialog : public QDialog
{
    Q_OBJECT
public:
    enum class ViewMode { TableView,
                          TabsView };

    explicit AnnotationEditorDialog(QWidget *parent,
                                    const QString &targetId = {},
                                    const QString &customId = {});
    ~AnnotationEditorDialog();

    const Annotation &annotation() const;
    void setAnnotation(const Annotation &annotation);

    const QString &customId() const;
    void setCustomId(const QString &customId);

    DefaultAnnotationsModel *defaultAnnotations() const;
    void loadDefaultAnnotations(const QString &filename);

private slots:
    void buttonClicked(QAbstractButton *button);

    void acceptedClicked();
    void appliedClicked();

signals:
    void acceptedDialog(); //use instead of QDialog::accepted
    void appliedDialog();

private:
    void updateAnnotation();

private:
    GlobalAnnotationStatus m_globalStatus = GlobalAnnotationStatus::NoStatus;
    Annotation m_annotation;
    QString m_customId;
    std::unique_ptr<DefaultAnnotationsModel> m_defaults;
    AnnotationEditorWidget *m_editorWidget;

    QDialogButtonBox *m_buttonBox;
};


} //namespace QmlDesigner
