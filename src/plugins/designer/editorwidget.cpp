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

#include "editorwidget.h"
#include "formeditorw.h"

#include <coreplugin/minisplitter.h>
#include <utils/qtcassert.h>

#include <QtCore/QEvent>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>

using namespace Designer::Constants;

enum { ActionEditorTab, SignalSlotEditorTab };

enum { wantSignalSlotEditor = 0 };

namespace Designer {
namespace Internal {

SharedSubWindow::SharedSubWindow(QWidget *shared, QWidget *parent) :
   QWidget(parent),
   m_shared(shared),
   m_layout(new QVBoxLayout)
{
    QTC_ASSERT(m_shared, /**/);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void SharedSubWindow::activate()
{
    // Take the widget off the other parent
    QTC_ASSERT(m_shared, return);
    QWidget *currentParent = m_shared->parentWidget();
    if (currentParent == this)
        return;

    if (currentParent) {
        QVBoxLayout *lt = qobject_cast<QVBoxLayout *>(currentParent->layout());
        QTC_ASSERT(lt, return);
        m_shared->setParent(0);
        delete lt->takeAt(0);
    }
    m_layout->addWidget(m_shared);
    m_layout->invalidate();
}

SharedSubWindow::~SharedSubWindow()
{
    // Do not destroy the shared sub window if we currently own it
    if (m_layout->count()) {
        m_shared->setParent(0);
        delete m_layout->takeAt(0);
    }
}

// ---------- Global EditorState
Q_GLOBAL_STATIC(EditorWidgetState, editorWidgetState)

enum { Version = 1 };
// Simple conversion of an int list to QVariantList, size as leading element
static void intToVariantList(const QList<int> &il, QVariantList& vl)
{
    const int size = il.size();
    vl.push_back(size);
    if (size != 0) {
        const QList<int>::const_iterator cend = il.constEnd();
        for (QList<int>::const_iterator it = il.constBegin(); it != cend; ++it)
            vl.push_back(QVariant(*it));
    }
}
// Simple conversion of a QVariantList portion saved by the above function to int list
bool variantListToIntList(const QVariantList& vl, int &index, QList<int> &list)
{
    list.clear();
    if (index >= vl.size())
        return false;
    const int size = vl.at(index++).toInt();
    const int end = index + size;
    if (end > vl.size())
        return false;
    if (size != 0) {
        for ( ; index < end; index++)
            list.push_back(vl.at(index).toInt());
    }
    return true;
}

// ------------------ EditorWidgetState
QVariant EditorWidgetState::toVariant() const
{
    QVariantList rc;
    rc.push_back(Version);
    intToVariantList(horizontalSizes, rc);
    intToVariantList(centerVerticalSizes, rc);
    intToVariantList(rightVerticalSizes, rc);
    return QVariant(rc);
}

bool EditorWidgetState::fromVariant(const QVariant &v)
{
    // Restore state. The weird thing is that QSettings might return
    // a QStringList although it was saved as QVariantList<int>.
    if (v.type() != QVariant::List && v.type() != QVariant::StringList)
        return false;
    const QVariantList vl = v.toList();
    if (vl.empty())
        return false;
    int index = 0;
    const QVariant &versionV = vl.at(index++);
    if (versionV.type() != QVariant::Int && versionV.type() != QVariant::String)
        return false;
    if (versionV.toInt() > Version)
        return false;
    return variantListToIntList(vl, index, horizontalSizes) &&
           variantListToIntList(vl, index, centerVerticalSizes) &&
           variantListToIntList(vl, index, rightVerticalSizes);
}

// ---------- EditorWidget
EditorWidget::EditorWidget(QWidget *formWindow) :
    Core::MiniSplitter(Qt::Horizontal),
    m_centerVertSplitter(new Core::MiniSplitter(Qt::Vertical)),
    m_bottomTab(0),
    m_rightVertSplitter(new Core::MiniSplitter(Qt::Vertical))
{
    // Get shared sub windows from Form Editor
    FormEditorW *few = FormEditorW::instance();
    QWidget * const*subs = few->designerSubWindows();
    // Create shared sub windows except SignalSlotEditor
    qFill(m_designerSubWindows, m_designerSubWindows + DesignerSubWindowCount, static_cast<SharedSubWindow*>(0));
    for (int i=0; i < DesignerSubWindowCount; i++)
        if (wantSignalSlotEditor || i != SignalSlotEditorSubWindow)
            m_designerSubWindows[i] = new SharedSubWindow(subs[i]);
    // Create splitter
    addWidget(m_designerSubWindows[WidgetBoxSubWindow]);

    // center
    m_centerVertSplitter->addWidget(formWindow);

    if (wantSignalSlotEditor) {
        m_bottomTab = new QTabWidget;
        m_bottomTab->setTabPosition(QTabWidget::South);
        m_bottomTab->addTab(m_designerSubWindows[ActionEditorSubWindow], tr("Action editor"));
        m_bottomTab->addTab(m_designerSubWindows[SignalSlotEditorSubWindow], tr("Signals and slots editor"));
        m_centerVertSplitter->addWidget(m_bottomTab);
    } else {
        m_centerVertSplitter->addWidget(m_designerSubWindows[ActionEditorSubWindow]);
    }

    addWidget(m_centerVertSplitter);

    m_rightVertSplitter->addWidget(m_designerSubWindows[ObjectInspectorSubWindow]);
    m_rightVertSplitter->addWidget(m_designerSubWindows[PropertyEditorSubWindow]);
    addWidget(m_rightVertSplitter);
}

void EditorWidget::setInitialSizes()
{
    QList<int> sizes;
    // center vertical. Either the tab containing signal slot editor/
    // action editor or the action editor itself
    const QWidget *bottomWidget = m_bottomTab;
    if (!bottomWidget)
        bottomWidget = m_designerSubWindows[ActionEditorSubWindow];
    const int tabHeight = bottomWidget->sizeHint().height();
    sizes.push_back(height() - handleWidth() -  tabHeight);
    sizes.push_back( tabHeight);
    m_centerVertSplitter->setSizes(sizes);
    // right vert
    sizes.clear();
    sizes.push_back(height() /2 - (handleWidth() / 2));
    sizes.push_back(height() / 2 - (handleWidth() / 2));
    m_rightVertSplitter->setSizes(sizes);
    // horiz sizes
    sizes.clear();
    const int wboxWidth = m_designerSubWindows[WidgetBoxSubWindow]->sizeHint().width();
    const int vSplitterWidth = m_rightVertSplitter->sizeHint().width();
    sizes.push_back(wboxWidth);
    sizes.push_back(width() - 2 * handleWidth() -  wboxWidth - vSplitterWidth);
    sizes.push_back(vSplitterWidth);
    setSizes(sizes);
}

void EditorWidget::activate()
{
    for (int i=0; i < DesignerSubWindowCount; i++)
        if (SharedSubWindow *sw = m_designerSubWindows[i]) // Signal slot might be deactivated
            sw->activate();
    if (!restore(*editorWidgetState()))
        setInitialSizes();
}

bool EditorWidget::event(QEvent * e)
{
    if (e->type() == QEvent::Hide)
        *editorWidgetState() = save();
    return QSplitter::event(e);
}

EditorWidgetState EditorWidget::save() const
{
    EditorWidgetState rc;
    rc.horizontalSizes = sizes();
    rc.centerVerticalSizes = m_centerVertSplitter->sizes();
    rc.rightVerticalSizes = m_rightVertSplitter->sizes();
    return rc;
}

bool EditorWidget::restore(const EditorWidgetState &s)
{
    if (s.horizontalSizes.size() != count() ||
        s.centerVerticalSizes.size() != m_centerVertSplitter->count() ||
        s.rightVerticalSizes.size() != m_rightVertSplitter->count())
        return false;
    m_centerVertSplitter->setSizes(s.centerVerticalSizes);
    m_rightVertSplitter->setSizes(s.rightVerticalSizes);
    setSizes(s.horizontalSizes);
    return true;
}

void EditorWidget::toolChanged(int i)
{
    if (m_bottomTab)
        m_bottomTab->setCurrentIndex(i == EditModeSignalsSlotEditor ? SignalSlotEditorTab : ActionEditorTab);
}

EditorWidgetState EditorWidget::state()
{
    return *editorWidgetState();
}

void EditorWidget::setState(const EditorWidgetState& st)
{
    *editorWidgetState() = st;
}

} // namespace Internal
} // namespace Designer
