// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationlistwidget.h"

#include "annotationeditorwidget.h"
#include "annotationlist.h"

#include <QHBoxLayout>

namespace QmlDesigner {

AnnotationListWidget::AnnotationListWidget(ModelNode rootNode, QWidget *parent)
    : QWidget(parent)
    , m_listView(new AnnotationListView(rootNode, this))
    , m_editor(new AnnotationEditorWidget(this))
{
    createUI();
    connect(m_listView, &Utils::ListView::activated,
            this, &AnnotationListWidget::changeAnnotation);

    if (validateListSize())
        m_listView->selectRow(0);
}

void AnnotationListWidget::setRootNode(ModelNode rootNode)
{
    m_listView->setRootNode(rootNode);

    m_currentItem = -1;

    if (validateListSize())
        m_listView->selectRow(0);
}

void AnnotationListWidget::saveAllChanges()
{
    //first commit currently opened item
    if (m_currentItem != -1) {
        m_editor->updateAnnotation();
        const QString oldCustomId = m_editor->customId();
        const Annotation oldAnno = m_editor->annotation();

        m_listView->storeChangesInModel(m_currentItem, oldCustomId, oldAnno);
    }

    //save all
    m_listView->saveChangesFromModel();
}

void AnnotationListWidget::createUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    const QSizePolicy policy = m_listView->sizePolicy();
    m_listView->setSizePolicy(QSizePolicy::Minimum, policy.verticalPolicy());

    layout->addWidget(m_listView);
    layout->addWidget(m_editor);
}

bool AnnotationListWidget::validateListSize()
{
    const bool isFilled = m_listView->rowCount() > 0;

    m_editor->setEnabled(isFilled);

    return isFilled;
}

void AnnotationListWidget::changeAnnotation(const QModelIndex &index)
{
    //store previous data
    if (m_currentItem != -1) {
        m_editor->updateAnnotation();
        const QString &oldCustomId = m_editor->customId();
        const Annotation &oldAnno = m_editor->annotation();

        m_listView->storeChangesInModel(m_currentItem, oldCustomId, oldAnno);
    }

    //show new data
    if (!index.isValid() || index.row() < 0 || index.row() >= m_listView->rowCount())
        return;

    const auto annotationData = m_listView->getStoredAnnotationById(index.row());

    m_editor->setTargetId(annotationData.id);
    m_editor->setCustomId(annotationData.annotationName);
    m_editor->setAnnotation(annotationData.annotation);

    m_currentItem = index.row();
}

}
