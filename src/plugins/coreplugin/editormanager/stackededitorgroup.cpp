/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "stackededitorgroup.h"
#include "editormanager.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>

#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QStackedWidget>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#ifdef Q_WS_MAC
#include <qmacstyle_mac.h>
#endif

Q_DECLARE_METATYPE(Core::IEditor *)

using namespace Core;
using namespace Core::Internal;

StackedEditorGroup::StackedEditorGroup(QWidget *parent) :
    EditorGroup(parent),
    m_toplevel(new QWidget),
    m_toolBar(new QWidget),
    m_container(new QStackedWidget(this)),
    m_editorList(new QComboBox),
    m_closeButton(new QToolButton),
    m_lockButton(new QToolButton),
    m_defaultToolBar(new QToolBar(this)),
    m_infoWidget(new QFrame(this)),
    m_editorForInfoWidget(0)
{
    QVBoxLayout *tl = new QVBoxLayout(m_toplevel);
    tl->setSpacing(0);
    tl->setMargin(0);
    {
        m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_editorList->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_editorList->setMinimumContentsLength(20);
        m_proxyModel.setSourceModel(model());
        m_proxyModel.sort(0);
        m_editorList->setModel(&m_proxyModel);
        m_editorList->setMaxVisibleItems(40);

        QToolBar *editorListToolBar = new QToolBar;

        editorListToolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
        editorListToolBar->addWidget(m_editorList);

        m_defaultToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        m_activeToolBar = m_defaultToolBar;

        QHBoxLayout *toolBarLayout = new QHBoxLayout;
        toolBarLayout->setMargin(0);
        toolBarLayout->setSpacing(0);
        toolBarLayout->addWidget(m_defaultToolBar);
        m_toolBar->setLayout(toolBarLayout);

        m_lockButton->setAutoRaise(true);
        m_lockButton->setProperty("type", QLatin1String("dockbutton"));

        m_closeButton->setAutoRaise(true);
        m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
        m_closeButton->setProperty("type", QLatin1String("dockbutton"));

        QToolBar *rightToolBar = new QToolBar;
        rightToolBar->setLayoutDirection(Qt::RightToLeft);
        rightToolBar->addWidget(m_closeButton);
        rightToolBar->addWidget(m_lockButton);

        QHBoxLayout *toplayout = new QHBoxLayout;
        toplayout->setSpacing(0);
        toplayout->setMargin(0);
        toplayout->addWidget(editorListToolBar);
        toplayout->addWidget(m_toolBar, 1); // Custom toolbar stretches
        toplayout->addWidget(rightToolBar);

        QWidget *top = new QWidget;
        QVBoxLayout *vlayout = new QVBoxLayout(top);
        vlayout->setSpacing(0);
        vlayout->setMargin(0);
        vlayout->addLayout(toplayout);
        tl->addWidget(top);

        connect(m_editorList, SIGNAL(currentIndexChanged(int)), this, SLOT(listSelectionChanged(int)));
        connect(m_lockButton, SIGNAL(clicked()), this, SLOT(makeEditorWritable()));
        connect(m_closeButton, SIGNAL(clicked()), this, SLOT(sendCloseRequest()));
    }
    {
        m_infoWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
        m_infoWidget->setLineWidth(1);
        m_infoWidget->setForegroundRole(QPalette::ToolTipText);
        m_infoWidget->setBackgroundRole(QPalette::ToolTipBase);
        m_infoWidget->setAutoFillBackground(true);


        QHBoxLayout *hbox = new QHBoxLayout(m_infoWidget);
        hbox->setMargin(2);
        m_infoWidgetLabel = new QLabel("Placeholder");
        m_infoWidgetLabel->setForegroundRole(QPalette::ToolTipText);
        hbox->addWidget(m_infoWidgetLabel);
        hbox->addStretch(1);

        m_infoWidgetButton = new QToolButton;
        m_infoWidgetButton->setText(tr("Placeholder"));
        hbox->addWidget(m_infoWidgetButton);

        QToolButton *closeButton = new QToolButton;
        closeButton->setAutoRaise(true);
        closeButton->setIcon(QIcon(":/core/images/clear.png"));
        closeButton->setToolTip(tr("Close"));
        connect(closeButton, SIGNAL(clicked()), m_infoWidget, SLOT(hide()));

        hbox->addWidget(closeButton);


        m_infoWidget->setVisible(false);
        tl->addWidget(m_infoWidget);
    }
    tl->addWidget(m_container);

    QHBoxLayout *l = new QHBoxLayout;
    l->setSpacing(0);
    l->setMargin(0);
    l->addWidget(m_toplevel);
    setLayout(l);
}

void StackedEditorGroup::showEditorInfoBar(const QString &kind,
                                           const QString &infoText,
                                           const QString &buttonText,
                                           QObject *object, const char *member)
{
    m_infoWidgetKind = kind;
    m_infoWidgetLabel->setText(infoText);
    m_infoWidgetButton->setText(buttonText);
    m_infoWidgetButton->disconnect();
    if (object && member)
        connect(m_infoWidgetButton, SIGNAL(clicked()), object, member);
    m_infoWidget->setVisible(true);
    m_editorForInfoWidget = currentEditor();
}

void StackedEditorGroup::hideEditorInfoBar(const QString &kind)
{
    if (kind == m_infoWidgetKind)
        m_infoWidget->setVisible(false);
}


StackedEditorGroup::~StackedEditorGroup()
{
}

void StackedEditorGroup::focusInEvent(QFocusEvent *e)
{
    if (m_container->count() > 0) {
        setEditorFocus(m_container->currentIndex());
    } else {
        EditorGroup::focusInEvent(e);
    }
}

void StackedEditorGroup::setEditorFocus(int index)
{
    QWidget *w = m_container->widget(index);
    w->setFocus();
}

void StackedEditorGroup::addEditor(IEditor *editor)
{
    insertEditor(editorCount(), editor);
}

void StackedEditorGroup::insertEditor(int index, IEditor *editor)
{
    if (m_container->indexOf(editor->widget()) != -1)
        return;

    m_container->insertWidget(index, editor->widget());
    m_widgetEditorMap.insert(editor->widget(), editor);

    QToolBar *toolBar = editor->toolBar();
    if (toolBar) {
        toolBar->setVisible(false); // will be made visible in setCurrentEditor
        m_toolBar->layout()->addWidget(toolBar);
    }
    connect(editor, SIGNAL(changed()), this, SLOT(checkEditorStatus()));

    bool block = m_editorList->blockSignals(true);
    EditorGroup::insertEditor(index, editor);
    m_editorList->blockSignals(block);
    emit editorAdded(editor);
}

void StackedEditorGroup::sendCloseRequest()
{
    emit closeRequested(currentEditor());
}

void StackedEditorGroup::removeEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    EditorGroup::removeEditor(editor);
    const int index = m_container->indexOf(editor->widget());
    if (index != -1) {
        m_container->removeWidget(editor->widget());
        m_widgetEditorMap.remove(editor->widget());
        editor->widget()->setParent(0);
        disconnect(editor, SIGNAL(changed()), this, SLOT(updateEditorStatus()));
        QToolBar *toolBar = editor->toolBar();
        if (toolBar != 0) {
            if (m_activeToolBar == toolBar) {
                m_activeToolBar = m_defaultToolBar;
                m_activeToolBar->setVisible(true);
            }
            m_toolBar->layout()->removeWidget(toolBar);
            toolBar->setVisible(false);
            toolBar->setParent(0);
        }
        emit editorRemoved(editor);
    }
}

IEditor *StackedEditorGroup::currentEditor() const
{
    if (m_container->count() > 0)
        return m_widgetEditorMap.value(m_container->currentWidget());
    return 0;
}

void StackedEditorGroup::setCurrentEditor(IEditor *editor)
{
    if (!editor || m_container->count() <= 0
        || m_container->indexOf(editor->widget()) == -1)
        return;
    const int idx = m_container->indexOf(editor->widget());
    QTC_ASSERT(idx >= 0, return);
    if (m_container->currentIndex() != idx) {
        m_container->setCurrentIndex(idx);

        const bool block = m_editorList->blockSignals(true);
        m_editorList->setCurrentIndex(indexOf(editor));
        m_editorList->blockSignals(block);
    }
    setEditorFocus(idx);

    updateEditorStatus(editor);
    updateToolBar(editor);
    if (editor != m_editorForInfoWidget) {
        m_infoWidget->hide();
        m_editorForInfoWidget = 0;
    }
}

void StackedEditorGroup::checkEditorStatus()
{
    IEditor *editor = qobject_cast<IEditor *>(sender());
    if (editor == currentEditor())
        updateEditorStatus(editor);
}

void StackedEditorGroup::updateEditorStatus(IEditor *editor)
{
    static const QIcon lockedIcon(QLatin1String(":/core/images/locked.png"));
    static const QIcon unlockedIcon(QLatin1String(":/core/images/unlocked.png"));

    if (editor->file()->isReadOnly()) {
        m_lockButton->setIcon(lockedIcon);
        m_lockButton->setEnabled(!editor->file()->fileName().isEmpty());
        m_lockButton->setToolTip(tr("Make writable"));
    } else {
        m_lockButton->setIcon(unlockedIcon);
        m_lockButton->setEnabled(false);
        m_lockButton->setToolTip(tr("File is writable"));
    }
    if (currentEditor() == editor)
        m_editorList->setToolTip(model()->data(model()->indexOf(editor), Qt::ToolTipRole).toString());
    model()->emitDataChanged(editor);
}

void StackedEditorGroup::updateToolBar(IEditor *editor)
{
    QToolBar *toolBar = editor->toolBar();
    if (!toolBar)
        toolBar = m_defaultToolBar;
    if (m_activeToolBar == toolBar)
        return;
    m_activeToolBar->setVisible(false);
    toolBar->setVisible(true);
    m_activeToolBar = toolBar;
}

int StackedEditorGroup::editorCount() const
{
    return model()->editors().count();
}

QList<IEditor *> StackedEditorGroup::editors() const
{
    QAbstractItemModel *model = m_editorList->model();
    QList<IEditor*> output;
    int rows = model->rowCount();
    for (int i = 0; i < rows; ++i)
        output.append(model->data(model->index(i, 0), Qt::UserRole).value<IEditor*>());
    return output;
}

QList<IEditor *> StackedEditorGroup::editorsInNaturalOrder() const
{
    return model()->editors();
}

void StackedEditorGroup::makeEditorWritable()
{
    EditorManager::instance()->makeEditorWritable(currentEditor());
}

void StackedEditorGroup::listSelectionChanged(int index)
{
    QAbstractItemModel *model = m_editorList->model();
    setCurrentEditor(model->data(model->index(index, 0), Qt::UserRole).value<IEditor*>());
}

int StackedEditorGroup::indexOf(IEditor *editor)
{
    QAbstractItemModel *model = m_editorList->model();
    int rows = model->rowCount();
    for (int i = 0; i < rows; ++i) {
        if (editor == model->data(model->index(i, 0), Qt::UserRole).value<IEditor*>())
            return i;
    }
    QTC_ASSERT(false, /**/);
    return 0;
}
