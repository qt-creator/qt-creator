// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "annotation.h"
#include "modelnode.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QDialogButtonBox;
class QAbstractButton;
QT_END_NAMESPACE

namespace QmlDesigner {

class DefaultAnnotationsModel;
class AnnotationEditorWidget;
class AnnotationListWidget;

class GlobalAnnotationDialog : public QDialog
{
    Q_OBJECT
public:
    enum class ViewMode { GlobalAnnotation,
                          List };

    explicit GlobalAnnotationDialog(ModelNode rootNode, QWidget *parent);
    ~GlobalAnnotationDialog();

    const Annotation &annotation() const;
    void setAnnotation(const Annotation &annotation);

    DefaultAnnotationsModel *defaultAnnotations() const;
    void loadDefaultAnnotations(const QString &filename);

    GlobalAnnotationStatus globalStatus() const;
    void setStatus(GlobalAnnotationStatus status);

    ViewMode viewMode() const;

    void saveAnnotationListChanges();

private slots:
    void buttonClicked(QAbstractButton *button);

    void acceptedClicked();
    void appliedClicked();

signals:
    void acceptedDialog(); //use instead of QDialog::accepted
    void appliedDialog();
    void globalChanged();

private:
    void updateAnnotation();
    void setupUI();

private:
    GlobalAnnotationStatus m_globalStatus = GlobalAnnotationStatus::NoStatus;
    bool m_statusIsActive = false;
    Annotation m_annotation;
    std::unique_ptr<DefaultAnnotationsModel> m_defaults;
    AnnotationEditorWidget *m_editorWidget = nullptr;
    AnnotationListWidget *m_annotationListWidget = nullptr;

    QTabWidget *m_tabWidget = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};


} //namespace QmlDesigner
