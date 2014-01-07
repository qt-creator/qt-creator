/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stateseditorwidget.h"
#include "stateseditormodel.h"
#include "stateseditorview.h"
#include "stateseditorimageprovider.h"

#include <invalidqmlsourceexception.h>

#include <qapplication.h>

#include <QBoxLayout>

#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

enum {
    debug = false
};

namespace QmlDesigner {

int StatesEditorWidget::currentStateInternalId() const
{
    Q_ASSERT(m_quickView->rootObject());
    Q_ASSERT(m_quickView->rootObject()->property("currentStateInternalId").isValid());

    return m_quickView->rootObject()->property("currentStateInternalId").toInt();
}

void StatesEditorWidget::setCurrentStateInternalId(int internalId)
{
    m_quickView->rootObject()->setProperty("currentStateInternalId", internalId);
}

void StatesEditorWidget::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    m_imageProvider->setNodeInstanceView(nodeInstanceView);
}

void StatesEditorWidget::showAddNewStatesButton(bool showAddNewStatesButton)
{
    m_quickView->rootContext()->setContextProperty("canAddNewStates", showAddNewStatesButton);
}

StatesEditorWidget::StatesEditorWidget(StatesEditorView *statesEditorView, StatesEditorModel *statesEditorModel):
        QWidget(),
    m_quickView(new QQuickView()),
    m_statesEditorView(statesEditorView),
    m_imageProvider(0)
{
    m_imageProvider = new Internal::StatesEditorImageProvider;
    m_imageProvider->setNodeInstanceView(statesEditorView->nodeInstanceView());
    m_quickView->engine()->addImageProvider(
            QLatin1String("qmldesigner_stateseditor"), m_imageProvider);

    //m_quickView->setAcceptDrops(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    setMinimumHeight(160);
    layout->setMargin(0);
    layout->setSpacing(0);
    QWidget *container = createWindowContainer(m_quickView.data());
    layout->addWidget(container);

    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);

    m_quickView->rootContext()->setContextProperty(QLatin1String("statesEditorModel"), statesEditorModel);
    QColor highlightColor = palette().highlight().color();
    if (0.5*highlightColor.saturationF()+0.75-highlightColor.valueF() < 0)
        highlightColor.setHsvF(highlightColor.hsvHueF(),0.1 + highlightColor.saturationF()*2.0, highlightColor.valueF());
    m_quickView->rootContext()->setContextProperty(QLatin1String("highlightColor"), highlightColor);

    m_quickView->rootContext()->setContextProperty("canAddNewStates", true);

    // Work around ASSERT in the internal QGraphicsScene that happens when
    // the scene is created + items set dirty in one event loop run (BAUHAUS-459)
    //QApplication::processEvents();

    m_quickView->setSource(QUrl("qrc:/stateseditor/stateslist.qml"));

    if (!m_quickView->rootObject())
        throw InvalidQmlSourceException(__LINE__, __FUNCTION__, __FILE__);


    QEvent event(QEvent::WindowActivate);
    QApplication::sendEvent(m_quickView.data(), &event);


    connect(m_quickView->rootObject(), SIGNAL(currentStateInternalIdChanged()), statesEditorView, SLOT(synchonizeCurrentStateFromWidget()));
    connect(m_quickView->rootObject(), SIGNAL(createNewState()), statesEditorView, SLOT(createNewState()));
    connect(m_quickView->rootObject(), SIGNAL(deleteState(int)), statesEditorView, SLOT(removeState(int)));

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

    setWindowTitle(tr("States", "Title of Editor widget"));
}

StatesEditorWidget::~StatesEditorWidget()
{
}


QSize StatesEditorWidget::sizeHint() const
{
    return QSize(9999, 159);
}

}
