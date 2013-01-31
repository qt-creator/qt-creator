/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVESTATE_H
#define QDECLARATIVESTATE_H

#include "../qmljsdebugger_global.h"

#include <qdeclarative.h>
#include <qdeclarativeproperty.h>
#include <qobject.h>
#include <qsharedpointer.h>

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeActionEvent;
class QDeclarativeAbstractBinding;
class QDeclarativeBinding;
class QDeclarativeExpression;
class QMLJSDEBUGGER_EXTERN QDeclarativeAction
{
public:
    QDeclarativeAction();
    QDeclarativeAction(QObject *, const QString &, const QVariant &);
    QDeclarativeAction(QObject *, const QString &,
                       QDeclarativeContext *, const QVariant &);

    bool restore:1;
    bool actionDone:1;
    bool reverseEvent:1;
    bool deletableToBinding:1;

    QDeclarativeProperty property;
    QVariant fromValue;
    QVariant toValue;

    QDeclarativeAbstractBinding *fromBinding;
    QWeakPointer<QDeclarativeAbstractBinding> toBinding;
    QDeclarativeActionEvent *event;

    //strictly for matching
    QObject *specifiedObject;
    QString specifiedProperty;

    void deleteFromBinding();
};

class QMLJSDEBUGGER_EXTERN QDeclarativeActionEvent
{
public:
    virtual ~QDeclarativeActionEvent();
    virtual QString typeName() const;

    enum Reason { ActualChange, FastForward };

    virtual void execute(Reason reason = ActualChange);
    virtual bool isReversable();
    virtual void reverse(Reason reason = ActualChange);
    virtual void saveOriginals() {}
    virtual bool needsCopy() { return false; }
    virtual void copyOriginals(QDeclarativeActionEvent *) {}
    virtual bool isRewindable() { return isReversable(); }
    virtual void rewind() {}
    virtual void saveCurrentValues() {}
    virtual void saveTargetValues() {}

    virtual bool changesBindings();
    virtual void clearBindings();
    virtual bool override(QDeclarativeActionEvent*other);
};

//### rename to QDeclarativeStateChange?
class QDeclarativeStateGroup;
class QDeclarativeState;
class QDeclarativeStateOperationPrivate;
class QMLJSDEBUGGER_EXTERN QDeclarativeStateOperation : public QObject
{
    Q_OBJECT
public:
    QDeclarativeStateOperation(QObject *parent = 0)
        : QObject(parent) {}
    typedef QList<QDeclarativeAction> ActionList;

    virtual ActionList actions();

    QDeclarativeState *state() const;
    void setState(QDeclarativeState *state);

protected:
    QDeclarativeStateOperation(QObjectPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(QDeclarativeStateOperation)
    Q_DISABLE_COPY(QDeclarativeStateOperation)
};

typedef QDeclarativeStateOperation::ActionList QDeclarativeStateActions;

class QDeclarativeTransition;
class QDeclarativeStatePrivate;
class QMLJSDEBUGGER_EXTERN QDeclarativeState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QDeclarativeBinding *when READ when WRITE setWhen)
    Q_PROPERTY(QString extend READ extends WRITE setExtends)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarativeStateOperation> changes READ changes)
    Q_CLASSINFO("DefaultProperty", "changes")
    Q_CLASSINFO("DeferredPropertyNames", "changes")

public:
    QDeclarativeState(QObject *parent=0);
    virtual ~QDeclarativeState();

    QString name() const;
    void setName(const QString &);
    bool isNamed() const;

    /*'when' is a QDeclarativeBinding to limit state changes oscillation
     due to the unpredictable order of evaluation of bound expressions*/
    bool isWhenKnown() const;
    QDeclarativeBinding *when() const;
    void setWhen(QDeclarativeBinding *);

    QString extends() const;
    void setExtends(const QString &);

    QDeclarativeListProperty<QDeclarativeStateOperation> changes();
    int operationCount() const;
    QDeclarativeStateOperation *operationAt(int) const;

    QDeclarativeState &operator<<(QDeclarativeStateOperation *);

    void apply(QDeclarativeStateGroup *, QDeclarativeTransition *, QDeclarativeState *revert);
    void cancel();

    QDeclarativeStateGroup *stateGroup() const;
    void setStateGroup(QDeclarativeStateGroup *);

    bool containsPropertyInRevertList(QObject *target, const QString &name) const;
    bool changeValueInRevertList(QObject *target, const QString &name, const QVariant &revertValue);
    bool changeBindingInRevertList(QObject *target, const QString &name, QDeclarativeAbstractBinding *binding);
    bool removeEntryFromRevertList(QObject *target, const QString &name);
    void addEntryToRevertList(const QDeclarativeAction &action);
    void removeAllEntriesFromRevertList(QObject *target);
    void addEntriesToRevertList(const QList<QDeclarativeAction> &actions);
    QVariant valueInRevertList(QObject *target, const QString &name) const;
    QDeclarativeAbstractBinding *bindingInRevertList(QObject *target, const QString &name) const;

    bool isStateActive() const;

Q_SIGNALS:
    void completed();

private:
    Q_DECLARE_PRIVATE(QDeclarativeState)
    Q_DISABLE_COPY(QDeclarativeState)
};

QT_END_NAMESPACE

//QML_DECLARE_TYPE(QDeclarativeStateOperation)
//QML_DECLARE_TYPE(QDeclarativeState)

#endif // QDECLARATIVESTATE_H
