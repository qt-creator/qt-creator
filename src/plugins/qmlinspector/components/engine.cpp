/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "engine.h"
#include "objectpropertiesview.h"
#include "expressionquerywidget.h"
#include "objecttree.h"
#include "watchtable.h"

#include <private/qmlenginedebug_p.h>
#include <private/qmldebugclient_p.h>
#include <private/qmldebugservice_p.h>

#include <QtDeclarative/qmlcomponent.h>
#include <QtDeclarative/qmlgraphicsitem.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QFile>


QT_BEGIN_NAMESPACE


class DebuggerEngineItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT);
    Q_PROPERTY(int engineId READ engineId CONSTANT);

public:
    DebuggerEngineItem(const QString &name, int id)
    : m_name(name), m_engineId(id) {}

    QString name() const { return m_name; }
    int engineId() const { return m_engineId; }

private:
    QString m_name;
    int m_engineId;
};

EnginePane::EnginePane(QmlDebugConnection *conn, QWidget *parent)
: QWidget(parent), m_client(new QmlEngineDebug(conn, this)), m_engines(0), m_context(0), m_watchTableModel(0), m_exprQueryWidget(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QFile enginesFile(":/engines.qml");
    enginesFile.open(QFile::ReadOnly);
    Q_ASSERT(enginesFile.isOpen());

    m_engineView = new QmlView(this);
    m_engineView->rootContext()->setContextProperty("engines", qVariantFromValue(&m_engineItems));
    m_engineView->setContentResizable(true);
    m_engineView->setQml(enginesFile.readAll());
    m_engineView->execute();
    m_engineView->setFixedHeight(100);
    QObject::connect(m_engineView->root(), SIGNAL(engineClicked(int)),
                     this, SLOT(engineSelected(int)));
    QObject::connect(m_engineView->root(), SIGNAL(refreshEngines()),
                     this, SLOT(refreshEngines()));

    m_engineView->setVisible(false);
    layout->addWidget(m_engineView);

    QSplitter *splitter = new QSplitter;

    m_objTree = new ObjectTree(m_client, this);
    m_propertiesView = new ObjectPropertiesView(m_client);
    m_watchTableModel = new WatchTableModel(m_client, this);

    m_watchTableView = new WatchTableView(m_watchTableModel);
    m_watchTableView->setModel(m_watchTableModel);
    WatchTableHeaderView *header = new WatchTableHeaderView(m_watchTableModel);
    m_watchTableView->setHorizontalHeader(header);

    connect(m_objTree, SIGNAL(currentObjectChanged(QmlDebugObjectReference)),
            m_propertiesView, SLOT(reload(QmlDebugObjectReference)));
    connect(m_objTree, SIGNAL(expressionWatchRequested(QmlDebugObjectReference,QString)),
            m_watchTableModel, SLOT(expressionWatchRequested(QmlDebugObjectReference,QString)));

    connect(m_propertiesView, SIGNAL(activated(QmlDebugObjectReference,QmlDebugPropertyReference)),
            m_watchTableModel, SLOT(togglePropertyWatch(QmlDebugObjectReference,QmlDebugPropertyReference)));

    connect(m_watchTableModel, SIGNAL(watchCreated(QmlDebugWatch*)),
            m_propertiesView, SLOT(watchCreated(QmlDebugWatch*)));

    connect(m_watchTableView, SIGNAL(objectActivated(int)),
            m_objTree, SLOT(setCurrentObject(int)));

    m_exprQueryWidget = new ExpressionQueryWidget(ExpressionQueryWidget::SeparateEntryMode, m_client);
    connect(m_objTree, SIGNAL(currentObjectChanged(QmlDebugObjectReference)),
            m_exprQueryWidget, SLOT(setCurrentObject(QmlDebugObjectReference)));

    QSplitter *propertiesTab = new QSplitter(Qt::Vertical);
    propertiesTab->addWidget(m_propertiesView);
    propertiesTab->addWidget(m_exprQueryWidget);
    propertiesTab->setStretchFactor(0, 2);
    propertiesTab->setStretchFactor(1, 1);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(propertiesTab, tr("Properties"));
    m_tabs->addTab(m_watchTableView, tr("Watched"));

    splitter->addWidget(m_objTree);
    splitter->addWidget(m_tabs);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter);
}

void EnginePane::engineSelected(int id)
{
    qWarning() << "Engine selected" << id;
    queryContext(id);
}

void EnginePane::queryContext(int id)
{
    if (m_context) {
        delete m_context;
        m_context = 0;
    }

    m_context = m_client->queryRootContexts(QmlDebugEngineReference(id), this);
    if (!m_context->isWaiting())
        contextChanged();
    else
        QObject::connect(m_context, SIGNAL(stateChanged(QmlDebugQuery::State)),
                         this, SLOT(contextChanged()));
}

void EnginePane::contextChanged()
{
    //dump(m_context->rootContext(), 0);

    foreach (const QmlDebugObjectReference &object, m_context->rootContext().objects())
        m_objTree->reload(object.debugId());

    delete m_context; m_context = 0;
}

void EnginePane::refreshEngines()
{
    if (m_engines)
        return;

    m_engines = m_client->queryAvailableEngines(this);
    if (!m_engines->isWaiting())
        enginesChanged();
    else
        QObject::connect(m_engines, SIGNAL(stateChanged(QmlDebugQuery::State)),
                         this, SLOT(enginesChanged()));
}

void EnginePane::enginesChanged()
{
    qDeleteAll(m_engineItems);
    m_engineItems.clear();

    QList<QmlDebugEngineReference> engines = m_engines->engines();
    delete m_engines; m_engines = 0;

    if (engines.isEmpty())
        qWarning("qmldebugger: no engines found!");

    for (int ii = 0; ii < engines.count(); ++ii)
        m_engineItems << new DebuggerEngineItem(engines.at(ii).name(),
                                                engines.at(ii).debugId());

    m_engineView->rootContext()->setContextProperty("engines", qVariantFromValue(&m_engineItems));

    m_engineView->setVisible(m_engineItems.count() > 1);
    if (m_engineItems.count() == 1)
        engineSelected(qobject_cast<DebuggerEngineItem*>(m_engineItems.at(0))->engineId());
}


#include "engine.moc"

QT_END_NAMESPACE

