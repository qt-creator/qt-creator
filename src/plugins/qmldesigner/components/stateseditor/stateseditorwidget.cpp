/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
#include <utils/stylehelper.h>

#include <QApplication>

#include <QFileInfo>
#include <QShortcut>
#include <QBoxLayout>
#include <QKeySequence>

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

enum {
    debug = false
};

namespace QmlDesigner {

int StatesEditorWidget::currentStateInternalId() const
{
    Q_ASSERT(rootObject());
    Q_ASSERT(rootObject()->property("currentStateInternalId").isValid());

    return rootObject()->property("currentStateInternalId").toInt();
}

void StatesEditorWidget::setCurrentStateInternalId(int internalId)
{
    rootObject()->setProperty("currentStateInternalId", internalId);
}

void StatesEditorWidget::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    m_imageProvider->setNodeInstanceView(nodeInstanceView);
}

void StatesEditorWidget::showAddNewStatesButton(bool showAddNewStatesButton)
{
    rootContext()->setContextProperty(QLatin1String("canAddNewStates"), showAddNewStatesButton);
}

StatesEditorWidget::StatesEditorWidget(StatesEditorView *statesEditorView, StatesEditorModel *statesEditorModel)
    : QQuickWidget(),
      m_statesEditorView(statesEditorView),
      m_imageProvider(0),
      m_qmlSourceUpdateShortcut(0)
{
    m_imageProvider = new Internal::StatesEditorImageProvider;
    m_imageProvider->setNodeInstanceView(statesEditorView->nodeInstanceView());

    engine()->addImageProvider(QStringLiteral("qmldesigner_stateseditor"), m_imageProvider);
    engine()->addImportPath(qmlSourcesPath());

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F4), this);
    connect(m_qmlSourceUpdateShortcut, SIGNAL(activated()), this, SLOT(reloadQmlSource()));

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    rootContext()->setContextProperty(QStringLiteral("statesEditorModel"), statesEditorModel);
    rootContext()->setContextProperty(QStringLiteral("highlightColor"), Utils::StyleHelper::notTooBrightHighlightColor());


    rootContext()->setContextProperty(QLatin1String("canAddNewStates"), true);

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
    engine()->clearComponentCache();
    setSource(QUrl::fromLocalFile(statesListQmlFilePath));

    QTC_ASSERT(rootObject(), return);
    connect(rootObject(), SIGNAL(currentStateInternalIdChanged()), m_statesEditorView.data(), SLOT(synchonizeCurrentStateFromWidget()));
    connect(rootObject(), SIGNAL(createNewState()), m_statesEditorView.data(), SLOT(createNewState()));
    connect(rootObject(), SIGNAL(deleteState(int)), m_statesEditorView.data(), SLOT(removeState(int)));
    m_statesEditorView.data()->synchonizeCurrentStateFromWidget();
    setFixedHeight(initialSize().height());

    connect(rootObject(), SIGNAL(expandedChanged()), this, SLOT(changeHeight()));
}

void StatesEditorWidget::changeHeight()
{
    setFixedHeight(rootObject()->height());
}
}
