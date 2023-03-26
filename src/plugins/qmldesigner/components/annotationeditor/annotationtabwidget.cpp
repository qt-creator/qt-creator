// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationtabwidget.h"

#include "annotationcommenttab.h"

#include <timelineeditor/timelineicons.h>

#include <QAction>
#include <QMessageBox>
#include <QToolBar>

namespace QmlDesigner {
AnnotationTabWidget::AnnotationTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    auto *commentCornerWidget = new QToolBar;

    //Making it look similar to timeline editor button:
    commentCornerWidget->setStyleSheet("QToolBar { background-color: transparent; border-width: 1px; }");

    auto *commentAddAction = new QAction(TimelineIcons::ADD_TIMELINE.icon(),
                                         tr("Add Comment")); //timeline icons?
    auto *commentRemoveAction = new QAction(TimelineIcons::REMOVE_TIMELINE.icon(),
                                            tr("Remove Comment")); //timeline icons?
    connect(commentAddAction, &QAction::triggered, this, [this]() { addCommentTab(); });

    connect(commentRemoveAction, &QAction::triggered, this, [this]() {
        int currentIndex = this->currentIndex();
        QString currentTitle = tabText(currentIndex);
        if (QMessageBox::question(this,
                                  currentTitle,
                                  tr("Delete this comment?"))
            == QMessageBox::Yes) {
            removeTab(currentIndex);
            if (count() == 0) //lets be sure that tabWidget is never empty
                addCommentTab();
        }
    });

    commentCornerWidget->addAction(commentAddAction);
    commentCornerWidget->addAction(commentRemoveAction);
    setCornerWidget(commentCornerWidget, Qt::TopRightCorner);
}

QVector<Comment> AnnotationTabWidget::fetchComments() const
{
    QVector<Comment> comments;
    for (int i = 0; i < count(); i++) {
        auto *tab = qobject_cast<AnnotationCommentTab *>(widget(i));
        if (!tab)
            continue;

        Comment comment = tab->currentComment();

        if (!comment.isEmpty())
            comments.push_back(comment);
    }

    return comments;
}

void AnnotationTabWidget::setupComments(const QVector<Comment> &comments)
{
    setUpdatesEnabled(false);

    deleteAllTabs();
    if (comments.isEmpty())
        addCommentTab();

    for (const Comment &comment : comments)
        addCommentTab(comment);

    setUpdatesEnabled(true);
}

DefaultAnnotationsModel *AnnotationTabWidget::defaultAnnotations() const
{
    return m_defaults;
}

void AnnotationTabWidget::setDefaultAnnotations(DefaultAnnotationsModel *defaults)
{
    m_defaults = defaults;
    for (int i = 0; i < count(); i++) {
        // The tab widget might be contain regular QTabs initially, hence we need this qobject_cast test
        if (auto *tab = qobject_cast<AnnotationCommentTab *>(widget(i)))
            tab->setDefaultAnnotations(defaults);
    }
}

void AnnotationTabWidget::onCommentTitleChanged(const QString &text, QWidget *tab)
{
    int tabIndex = indexOf(tab);
    if (tabIndex >= 0)
        setTabText(tabIndex, text);

    if (text.isEmpty())
        setTabText(tabIndex, defaultTabName + " " + QString::number(tabIndex + 1));
}

void AnnotationTabWidget::addCommentTab(const QmlDesigner::Comment &comment)
{
    auto *commentTab = new AnnotationCommentTab();
    commentTab->setDefaultAnnotations(m_defaults);
    commentTab->setComment(comment);

    QString tabTitle(comment.title());
    int tabIndex = addTab(commentTab, tabTitle);
    setCurrentIndex(tabIndex);

    if (tabTitle.isEmpty()) {
        const QString appendix = ((tabIndex > 0) ? QString::number(tabIndex + 1) : "");
        tabTitle = QString("%1 %2").arg(defaultTabName).arg(appendix);
        setTabText(tabIndex, tabTitle);
    }
    connect(commentTab,
            &AnnotationCommentTab::titleChanged,
            this,
            &AnnotationTabWidget::onCommentTitleChanged);
}

void AnnotationTabWidget::deleteAllTabs()
{
    while (count() > 0) {
        QWidget *w = widget(0);
        removeTab(0);
        delete w;
    }
}

} // namespace QmlDesigner
