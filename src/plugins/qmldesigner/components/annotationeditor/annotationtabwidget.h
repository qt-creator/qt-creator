// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QPointer>
#include <QTabWidget>

#include "annotation.h"
#include "defaultannotations.h"

namespace QmlDesigner {
class AnnotationCommentTab;

class AnnotationTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    AnnotationTabWidget(QWidget *parent = nullptr);
    ~AnnotationTabWidget() = default;

    QVector<Comment> fetchComments() const;
    void setupComments(const QVector<Comment> &comments);

    DefaultAnnotationsModel *defaultAnnotations() const;
    void setDefaultAnnotations(DefaultAnnotationsModel *);

public slots:
    void addCommentTab(const QmlDesigner::Comment &comment = {});
    void deleteAllTabs();

private slots:
    void onCommentTitleChanged(const QString &text, QWidget *tab);

private:
    const QString defaultTabName = {tr("Annotation")};

    QPointer<DefaultAnnotationsModel> m_defaults;
};
} // namespace QmlDesigner
