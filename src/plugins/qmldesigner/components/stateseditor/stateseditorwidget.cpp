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

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QApplication>

#include <QFileInfo>
#include <QShortcut>
#include <QBoxLayout>
#include <QKeySequence>

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

StatesEditorWidget::StatesEditorWidget(StatesEditorView *statesEditorView, StatesEditorModel *statesEditorModel)
    : QWidget(),
      m_quickView(new QQuickView()),
      m_statesEditorView(statesEditorView),
      m_imageProvider(0),
      m_qmlSourceUpdateShortcut(0)
{
    m_imageProvider = new Internal::StatesEditorImageProvider;
    m_imageProvider->setNodeInstanceView(statesEditorView->nodeInstanceView());

    m_quickView->engine()->addImageProvider(QStringLiteral("qmldesigner_stateseditor"), m_imageProvider);
    m_quickView->engine()->addImportPath(qmlSourcesPath());

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F4), this);
    connect(m_qmlSourceUpdateShortcut, SIGNAL(activated()), this, SLOT(reloadQmlSource()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    QWidget *container = createWindowContainer(m_quickView.data());
    layout->addWidget(container);
    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_quickView->rootContext()->setContextProperty(QStringLiteral("statesEditorModel"), statesEditorModel);
    QColor highlightColor = palette().highlight().color();
    if (0.5*highlightColor.saturationF()+0.75-highlightColor.valueF() < 0)
        highlightColor.setHsvF(highlightColor.hsvHueF(),0.1 + highlightColor.saturationF()*2.0, highlightColor.valueF());
    m_quickView->rootContext()->setContextProperty(QStringLiteral("highlightColor"), highlightColor);

    m_quickView->rootContext()->setContextProperty("canAddNewStates", true);

    setWindowTitle(tr("States", "Title of Editor widget"));

    // init the first load of the QML UI elements
    reloadQmlSource();
}

StatesEditorWidget::~StatesEditorWidget()
{
}

QString StatesEditorWidget::qmlSourcesPath() {
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/statesEditorQmlSources");
}

void StatesEditorWidget::reloadQmlSource()
{
    QString statesListQmlFilePath = qmlSourcesPath() + QStringLiteral("/StatesList.qml");
    QTC_ASSERT(QFileInfo::exists(statesListQmlFilePath), return);
    m_quickView->engine()->clearComponentCache();
    m_quickView->setSource(QUrl::fromLocalFile(statesListQmlFilePath));

    QTC_ASSERT(m_quickView->rootObject(), return);
    connect(m_quickView->rootObject(), SIGNAL(currentStateInternalIdChanged()), m_statesEditorView.data(), SLOT(synchonizeCurrentStateFromWidget()));
    connect(m_quickView->rootObject(), SIGNAL(createNewState()), m_statesEditorView.data(), SLOT(createNewState()));
    connect(m_quickView->rootObject(), SIGNAL(deleteState(int)), m_statesEditorView.data(), SLOT(removeState(int)));
    m_statesEditorView.data()->synchonizeCurrentStateFromWidget();
    setFixedHeight(m_quickView->initialSize().height());

    connect(m_quickView->rootObject(), SIGNAL(expandedChanged()), this, SLOT(changeHeight()));
}

void StatesEditorWidget::changeHeight()
{
    setFixedHeight(m_quickView->rootObject()->height());
}
}
