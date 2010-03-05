/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "stateseditorwidget.h"
#include "stateseditormodel.h"
#include "stateseditorview.h"

#include <qmlitemnode.h>
#include <invalidargumentexception.h>
#include <invalidqmlsourceexception.h>

#include <QtCore/QFile>
#include <qapplication.h>

#include <QtGui/QBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeItem>

enum {
    debug = false
};

namespace QmlDesigner {

namespace Internal {

class StatesEditorWidgetPrivate : QObject
{
    Q_OBJECT

private:
    StatesEditorWidgetPrivate(StatesEditorWidget *q);

    int currentIndex() const;
    void setCurrentIndex(int i);

    bool validStateName(const QString &name) const;

private slots:
    void currentStateChanged();
    void addState();
    void removeState(int);
    void duplicateCurrentState();

private:
    StatesEditorWidget *m_q;
    QWeakPointer<Model> model;
    QWeakPointer<QDeclarativeView> listView;
    QWeakPointer<Internal::StatesEditorModel> statesEditorModel;
    QWeakPointer<Internal::StatesEditorView> statesEditorView;
    friend class QmlDesigner::StatesEditorWidget;
};

StatesEditorWidgetPrivate::StatesEditorWidgetPrivate(StatesEditorWidget *q) :
        QObject(q),
        m_q(q)
{
}

int StatesEditorWidgetPrivate::currentIndex() const
{
    Q_ASSERT(listView->rootObject());
    Q_ASSERT(listView->rootObject()->property("currentStateIndex").isValid());
    return listView->rootObject()->property("currentStateIndex").toInt();
}

void StatesEditorWidgetPrivate::setCurrentIndex(int i)
{
    listView->rootObject()->setProperty("currentStateIndex", i);
}

bool StatesEditorWidgetPrivate::validStateName(const QString &name) const
{
    if (name == tr("base state"))
        return false;
    QList<QmlModelState> modelStates = statesEditorView->stateRootNode().states().allStates();
    foreach (const QmlModelState &state, modelStates) {
        if (state.name() == name)
            return false;
    }
    return true;
}

void StatesEditorWidgetPrivate::currentStateChanged()
{
    statesEditorView->setCurrentState(currentIndex());
}

void StatesEditorWidgetPrivate::addState()
{
    // can happen when root node is e.g. a ListModel
    if (!statesEditorView->stateRootNode().isValid())
        return;

    QStringList modelStateNames = statesEditorView->stateRootNode().states().names();

    QString newStateName;
    int index = 1;
    while (1) {
        newStateName = tr("State%1").arg(index++);
        if (!modelStateNames.contains(newStateName))
            break;
    }
    statesEditorView->createState(newStateName);
}

void StatesEditorWidgetPrivate::removeState(int i)
{
    statesEditorView->removeState(i);
}

void StatesEditorWidgetPrivate::duplicateCurrentState()
{
    statesEditorView->duplicateCurrentState(currentIndex());
}

} // namespace Internal

StatesEditorWidget::StatesEditorWidget(QWidget *parent):
        QWidget(parent),
        m_d(new Internal::StatesEditorWidgetPrivate(this))
{
    m_d->statesEditorModel = new Internal::StatesEditorModel(this);
    m_d->listView = new QDeclarativeView(this);

    m_d->listView->setAcceptDrops(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_d->listView.data());

    m_d->listView->setResizeMode(QDeclarativeView::SizeRootObjectToView);

    m_d->listView->rootContext()->setContextProperty(QLatin1String("statesEditorModel"), m_d->statesEditorModel.data());

    // Work around ASSERT in the internal QGraphicsScene that happens when
    // the scene is created + items set dirty in one event loop run (BAUHAUS-459)
    QApplication::processEvents();

    m_d->listView->setSource(QUrl("qrc:/stateseditor/stateslist.qml"));

    if (!m_d->listView->rootObject())
        throw InvalidQmlSourceException(__LINE__, __FUNCTION__, __FILE__);

    m_d->listView->setFocusPolicy(Qt::ClickFocus);
    QApplication::sendEvent(m_d->listView->scene(), new QEvent(QEvent::WindowActivate));

    connect(m_d->listView->rootObject(), SIGNAL(currentStateIndexChanged()), m_d, SLOT(currentStateChanged()));
    connect(m_d->listView->rootObject(), SIGNAL(createNewState()), m_d, SLOT(addState()));
    connect(m_d->listView->rootObject(), SIGNAL(duplicateCurrentState()), m_d, SLOT(duplicateCurrentState()));
    connect(m_d->listView->rootObject(), SIGNAL(deleteState(int)), m_d, SLOT(removeState(int)));

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

    setWindowTitle(tr("States", "Title of Editor widget"));
}

StatesEditorWidget::~StatesEditorWidget()
{
    delete m_d;
}

void StatesEditorWidget::setup(Model *model)
{
    m_d->model = model;
    if (m_d->statesEditorView.isNull())
        m_d->statesEditorView = new Internal::StatesEditorView(m_d->statesEditorModel.data(), this);
    m_d->statesEditorModel->setStatesEditorView(m_d->statesEditorView.data());

    m_d->model->attachView(m_d->statesEditorView.data());

    // select first state (which is the base state)
    m_d->currentStateChanged();
}



QSize StatesEditorWidget::sizeHint() const
{
    return QSize(9999, 159);
}

}

#include "stateseditorwidget.moc"
