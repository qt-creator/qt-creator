/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "editorview.h"
#include "editormanager.h"
#include "coreimpl.h"

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


//================EditorModel====================
int EditorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

int EditorModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_editors.count();
    return 0;
}

void EditorModel::addEditor(IEditor *editor)
{
    int index = 0;

    QString fileName = editor->file()->fileName();
    for (index = 0; index < m_editors.count(); ++index)
        if (fileName < m_editors.at(index)->file()->fileName())
            break;

    beginInsertRows(QModelIndex(), index, index);
    m_editors.insert(index, editor);
    connect(editor, SIGNAL(changed()), this, SLOT(itemChanged()));
    endInsertRows();
}

void EditorModel::removeEditor(IEditor *editor)
{
    int idx = m_editors.indexOf(editor);
    if (idx < 0)
        return;
    beginRemoveRows(QModelIndex(), idx, idx);
    m_editors.removeAt(idx);
    endRemoveRows();
    disconnect(editor, SIGNAL(changed()), this, SLOT(itemChanged()));
}


void EditorModel::emitDataChanged(IEditor *editor)
{
    int idx = m_editors.indexOf(editor);
    if (idx < 0)
        return;
    QModelIndex mindex = index(idx, 0);
    emit dataChanged(mindex, mindex);
}

QModelIndex EditorModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (column != 0 || row < 0 || row >= m_editors.count())
        return QModelIndex();
    return createIndex(row, column);
}

QVariant EditorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    IEditor *editor = m_editors.at(index.row());
    QTC_ASSERT(editor, return QVariant());
    switch (role) {
    case Qt::DisplayRole:
        return editor->file()->isModified()
                ? editor->displayName() + QLatin1String("*")
                : editor->displayName();
    case Qt::DecorationRole:
        return editor->file()->isReadOnly()
                ? QIcon(QLatin1String(":/core/images/locked.png"))
                : QIcon();
    case Qt::ToolTipRole:
        return editor->file()->fileName().isEmpty()
                ? editor->displayName()
                : QDir::toNativeSeparators(editor->file()->fileName());
    case Qt::UserRole:
        return qVariantFromValue(editor);
    default:
        return QVariant();
    }
    return QVariant();
}

QModelIndex EditorModel::indexOf(IEditor *editor) const
{
    int idx = m_editors.indexOf(editor);
    if (idx < 0)
        return indexOf(editor->file()->fileName());
    return createIndex(idx, 0);
}

QModelIndex EditorModel::indexOf(const QString &fileName) const
{
    for (int i = 0; i < m_editors.count(); ++i)
        if (m_editors.at(i)->file()->fileName() == fileName)
            return createIndex(i, 0);
    return QModelIndex();
}


void EditorModel::itemChanged()
{
    emitDataChanged(qobject_cast<IEditor*>(sender()));
}



//================EditorView====================

EditorView::EditorView(EditorModel *model, QWidget *parent) :
    QWidget(parent),
    m_toolBar(new QWidget),
    m_container(new QStackedWidget(this)),
    m_editorList(new QComboBox),
    m_closeButton(new QToolButton),
    m_lockButton(new QToolButton),
    m_defaultToolBar(new QToolBar(this)),
    m_infoWidget(new QFrame(this)),
    m_editorForInfoWidget(0)
{
    QVBoxLayout *tl = new QVBoxLayout(this);
    tl->setSpacing(0);
    tl->setMargin(0);
    {
        m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_editorList->setMinimumContentsLength(20);
        m_editorList->setModel(model);
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

}

void EditorView::showEditorInfoBar(const QString &kind,
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

void EditorView::hideEditorInfoBar(const QString &kind)
{
    if (kind == m_infoWidgetKind)
        m_infoWidget->setVisible(false);
}


EditorView::~EditorView()
{
}

void EditorView::focusInEvent(QFocusEvent *e)
{
    if (m_container->count() > 0) {
        setEditorFocus(m_container->currentIndex());
    } else {
        QWidget::focusInEvent(e);
    }
}

void EditorView::setEditorFocus(int index)
{
    QWidget *w = m_container->widget(index);
    w->setFocus();
}

void EditorView::addEditor(IEditor *editor)
{
    insertEditor(editorCount(), editor);
}

void EditorView::insertEditor(int index, IEditor *editor)
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

//    emit editorAdded(editor);
}

bool EditorView::hasEditor(IEditor *editor) const
{
    return (m_container->indexOf(editor->widget()) != -1);
}

void EditorView::sendCloseRequest()
{
    emit closeRequested(currentEditor());
}

void EditorView::removeEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return);
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
    }
}

IEditor *EditorView::currentEditor() const
{
    if (m_container->count() > 0)
        return m_widgetEditorMap.value(m_container->currentWidget());
    return 0;
}

void EditorView::setCurrentEditor(IEditor *editor)
{
    if (!editor || m_container->count() <= 0
        || m_container->indexOf(editor->widget()) == -1)
        return;
    const int idx = m_container->indexOf(editor->widget());
    QTC_ASSERT(idx >= 0, return);
    if (m_container->currentIndex() != idx) {
        m_container->setCurrentIndex(idx);
        disconnect(m_editorList, SIGNAL(currentIndexChanged(int)), this, SLOT(listSelectionChanged(int)));
        m_editorList->setCurrentIndex(qobject_cast<EditorModel*>(m_editorList->model())->indexOf(editor->file()->fileName()).row());
        connect(m_editorList, SIGNAL(currentIndexChanged(int)), this, SLOT(listSelectionChanged(int)));
    }
    setEditorFocus(idx);

    updateEditorStatus(editor);
    updateToolBar(editor);
    if (editor != m_editorForInfoWidget) {
        m_infoWidget->hide();
        m_editorForInfoWidget = 0;
    }
}

void EditorView::checkEditorStatus()
{
        IEditor *editor = qobject_cast<IEditor *>(sender());
        if (editor == currentEditor())
            updateEditorStatus(editor);
}

void EditorView::updateEditorStatus(IEditor *editor)
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
        m_editorList->setToolTip(
                editor->file()->fileName().isEmpty()
                ? editor->displayName()
                    : QDir::toNativeSeparators(editor->file()->fileName())
                    );

}

void EditorView::updateToolBar(IEditor *editor)
{
    QToolBar *toolBar = editor->toolBar();
    if (!toolBar)
        toolBar = m_defaultToolBar;
    if (m_activeToolBar == toolBar)
        return;
    toolBar->setVisible(true);
    m_activeToolBar->setVisible(false);
    m_activeToolBar = toolBar;
}

int EditorView::editorCount() const
{
    return m_container->count();
}

QList<IEditor *> EditorView::editors() const
{
    return m_widgetEditorMap.values();
}


void EditorView::makeEditorWritable()
{
    CoreImpl::instance()->editorManager()->makeEditorWritable(currentEditor());
}

void EditorView::listSelectionChanged(int index)
{
    QAbstractItemModel *model = m_editorList->model();
    setCurrentEditor(model->data(model->index(index, 0), Qt::UserRole).value<IEditor*>());
}


