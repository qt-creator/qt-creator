/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "stateseditorwidget.h"
#include "stateseditormodel.h"
#include "stateseditorview.h"
#include "stateseditorimageprovider.h"

#include <designersettings.h>
#include <theme.h>

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
    Q_ASSERT(rootObject());
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
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &StatesEditorWidget::reloadQmlSource);

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    rootContext()->setContextProperty(QStringLiteral("statesEditorModel"), statesEditorModel);

    rootContext()->setContextProperty(QLatin1String("canAddNewStates"), true);

    Theme::setupTheme(engine());

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

void StatesEditorWidget::toggleStatesViewExpanded()
{
    bool expanded = rootObject()->property("expanded").toBool();
    rootObject()->setProperty("expanded", !expanded);
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

    if (!DesignerSettings::getValue(DesignerSettingsKey::STATESEDITOR_EXPANDED).toBool()) {
        toggleStatesViewExpanded();
        setFixedHeight(rootObject()->height());
    }

    connect(rootObject(), SIGNAL(expandedChanged()), this, SLOT(handleExpandedChanged()));
}

void StatesEditorWidget::handleExpandedChanged()
{
    bool expanded = rootObject()->property("expanded").toBool();

    DesignerSettings::setValue(DesignerSettingsKey::STATESEDITOR_EXPANDED, expanded);

    setFixedHeight(rootObject()->height());
}
}
