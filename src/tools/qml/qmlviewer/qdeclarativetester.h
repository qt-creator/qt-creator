/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVETESTER_H
#define QDECLARATIVETESTER_H

#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QImage>
#include <QUrl>
#include <qmlruntime.h>
#include <qdeclarativelist.h>
#include <qdeclarative.h>
#include <QAbstractAnimation>

QT_BEGIN_NAMESPACE

class QDeclarativeVisualTest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeListProperty<QObject> events READ events CONSTANT)
    Q_CLASSINFO("DefaultProperty", "events")
public:
    QDeclarativeVisualTest() {}

    QDeclarativeListProperty<QObject> events() { return QDeclarativeListProperty<QObject>(this, m_events); }

    int count() const { return m_events.count(); }
    QObject *event(int idx) { return m_events.at(idx); }

private:
    QList<QObject *> m_events;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeVisualTest)

QT_BEGIN_NAMESPACE

class QDeclarativeVisualTestFrame : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int msec READ msec WRITE setMsec)
    Q_PROPERTY(QString hash READ hash WRITE setHash)
    Q_PROPERTY(QUrl image READ image WRITE setImage)
public:
    QDeclarativeVisualTestFrame() : m_msec(-1) {}

    int msec() const { return m_msec; }
    void setMsec(int m) { m_msec = m; }

    QString hash() const { return m_hash; }
    void setHash(const QString &hash) { m_hash = hash; }

    QUrl image() const { return m_image; }
    void setImage(const QUrl &image) { m_image = image; }

private:
    int m_msec;
    QString m_hash;
    QUrl m_image;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeVisualTestFrame)

QT_BEGIN_NAMESPACE

class QDeclarativeVisualTestMouse : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int type READ type WRITE setType)
    Q_PROPERTY(int button READ button WRITE setButton)
    Q_PROPERTY(int buttons READ buttons WRITE setButtons)
    Q_PROPERTY(int x READ x WRITE setX)
    Q_PROPERTY(int y READ y WRITE setY)
    Q_PROPERTY(int modifiers READ modifiers WRITE setModifiers)
    Q_PROPERTY(bool sendToViewport READ sendToViewport WRITE setSendToViewport)
public:
    QDeclarativeVisualTestMouse() : m_type(0), m_button(0), m_buttons(0), m_x(0), m_y(0), m_modifiers(0), m_viewport(false) {}

    int type() const { return m_type; }
    void setType(int t) { m_type = t; }
    
    int button() const { return m_button; }
    void setButton(int b) { m_button = b; }

    int buttons() const { return m_buttons; }
    void setButtons(int b) { m_buttons = b; }

    int x() const { return m_x; }
    void setX(int x) { m_x = x; }

    int y() const { return m_y; }
    void setY(int y) { m_y = y; }

    int modifiers() const { return m_modifiers; }
    void setModifiers(int modifiers) { m_modifiers = modifiers; }

    bool sendToViewport() const { return m_viewport; }
    void setSendToViewport(bool v) { m_viewport = v; }
private:
    int m_type;
    int m_button;
    int m_buttons;
    int m_x;
    int m_y;
    int m_modifiers;
    bool m_viewport;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeVisualTestMouse)

QT_BEGIN_NAMESPACE

class QDeclarativeVisualTestKey : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int type READ type WRITE setType)
    Q_PROPERTY(int key READ key WRITE setKey)
    Q_PROPERTY(int modifiers READ modifiers WRITE setModifiers)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool autorep READ autorep WRITE setAutorep)
    Q_PROPERTY(int count READ count WRITE setCount)
    Q_PROPERTY(bool sendToViewport READ sendToViewport WRITE setSendToViewport)
public:
    QDeclarativeVisualTestKey() : m_type(0), m_key(0), m_modifiers(0), m_autorep(false), m_count(0), m_viewport(false) {}

    int type() const { return m_type; }
    void setType(int t) { m_type = t; }

    int key() const { return m_key; }
    void setKey(int k) { m_key = k; }

    int modifiers() const { return m_modifiers; }
    void setModifiers(int m) { m_modifiers = m; }

    QString text() const { return m_text; }
    void setText(const QString &t) { m_text = t; }

    bool autorep() const { return m_autorep; }
    void setAutorep(bool a) { m_autorep = a; }

    int count() const { return m_count; }
    void setCount(int c) { m_count = c; }

    bool sendToViewport() const { return m_viewport; }
    void setSendToViewport(bool v) { m_viewport = v; }
private:
    int m_type;
    int m_key;
    int m_modifiers;
    QString m_text;
    bool m_autorep;
    int m_count;
    bool m_viewport;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeVisualTestKey)

QT_BEGIN_NAMESPACE

class QDeclarativeTester : public QAbstractAnimation
{
public:
    QDeclarativeTester(const QString &script, QDeclarativeViewer::ScriptOptions options, QDeclarativeView *parent);
    ~QDeclarativeTester();

    static void registerTypes();

    virtual int duration() const;

    void run();
    void save();

    void executefailure();
protected:
    virtual void updateCurrentTime(int msecs);
    virtual bool eventFilter(QObject *, QEvent *);

private:
    QString m_script;

    void imagefailure();
    void complete();

    enum Destination { View, ViewPort };
    void addKeyEvent(Destination, QKeyEvent *);
    void addMouseEvent(Destination, QMouseEvent *);
    QDeclarativeView *m_view;

    struct MouseEvent {
        MouseEvent(QMouseEvent *e)
            : type(e->type()), button(e->button()), buttons(e->buttons()), 
              pos(e->pos()), modifiers(e->modifiers()), destination(View) {}

        QEvent::Type type;
        Qt::MouseButton button;
        Qt::MouseButtons buttons;
        QPoint pos;
        Qt::KeyboardModifiers modifiers;
        Destination destination;

        int msec;
    };
    struct KeyEvent {
        KeyEvent(QKeyEvent *e)
            : type(e->type()), key(e->key()), modifiers(e->modifiers()), text(e->text()),
              autorep(e->isAutoRepeat()), count(e->count()), destination(View) {}
        QEvent::Type type;
        int key;
        Qt::KeyboardModifiers modifiers;
        QString text;
        bool autorep;
        ushort count;
        Destination destination;

        int msec;
    };
    struct FrameEvent {
        QImage image;
        QByteArray hash;
        int msec;
    };
    QList<MouseEvent> m_mouseEvents;
    QList<KeyEvent> m_keyEvents;

    QList<MouseEvent> m_savedMouseEvents;
    QList<KeyEvent> m_savedKeyEvents;
    QList<FrameEvent> m_savedFrameEvents;
    bool filterEvents;

    QDeclarativeViewer::ScriptOptions options;
    int testscriptidx;
    QDeclarativeVisualTest *testscript;

    bool hasCompleted;
    bool hasFailed;
};


QT_END_NAMESPACE

#endif // QDECLARATIVETESTER_H
