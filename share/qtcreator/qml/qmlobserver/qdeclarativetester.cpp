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

#include <qdeclarativetester.h>
#include <qdeclarativeview.h>
#include <QDeclarativeComponent>
#include <QGraphicsObject>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>

#ifndef NO_PRIVATE_HEADERS
#include <private/qabstractanimation_p.h>
#include <private/qdeclarativeitem_p.h>
#endif

QT_BEGIN_NAMESPACE

extern Q_GUI_EXPORT bool qt_applefontsmoothing_enabled;

QDeclarativeTester::QDeclarativeTester(const QString &script, QDeclarativeViewer::ScriptOptions opts,
                     QDeclarativeView *parent)
: QAbstractAnimation(parent), m_script(script), m_view(parent), filterEvents(true), options(opts),
  testscript(0), hasCompleted(false), hasFailed(false)
{
    parent->viewport()->installEventFilter(this);
    parent->installEventFilter(this);
#ifndef NO_PRIVATE_HEADERS
    QUnifiedTimer::instance()->setConsistentTiming(true);
#endif

    //Font antialiasing makes tests system-specific, so disable it
    QFont noAA = QApplication::font();
    noAA.setStyleStrategy(QFont::NoAntialias);
    QApplication::setFont(noAA);

    if (options & QDeclarativeViewer::Play)
        this->run();
    start();
}

QDeclarativeTester::~QDeclarativeTester()
{
    if (!hasFailed &&
        options & QDeclarativeViewer::Record &&
        options & QDeclarativeViewer::SaveOnExit)
        save();
}

int QDeclarativeTester::duration() const
{
    return -1;
}

void QDeclarativeTester::addMouseEvent(Destination dest, QMouseEvent *me)
{
    MouseEvent e(me);
    e.destination = dest;
    m_mouseEvents << e;
}

void QDeclarativeTester::addKeyEvent(Destination dest, QKeyEvent *ke)
{
    KeyEvent e(ke);
    e.destination = dest;
    m_keyEvents << e;
}

bool QDeclarativeTester::eventFilter(QObject *o, QEvent *e)
{
    if (!filterEvents)
        return false;

    Destination destination;
    if (o == m_view) {
        destination = View;
    } else if (o == m_view->viewport()) {
        destination = ViewPort;
    } else {
        return false;
    }

    switch (e->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            addKeyEvent(destination, (QKeyEvent *)e);
            return true;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::MouseButtonDblClick:
            addMouseEvent(destination, (QMouseEvent *)e);
            return true;
        default:
            break;
    }
    return false;
}

void QDeclarativeTester::executefailure()
{
    hasFailed = true;

    if (options & QDeclarativeViewer::ExitOnFailure)
        exit(-1);
}

void QDeclarativeTester::imagefailure()
{
    hasFailed = true;

    if (options & QDeclarativeViewer::ExitOnFailure){
        testSkip();
        exit(hasFailed?-1:0);
    }
}

void QDeclarativeTester::testSkip()
{
    if (options & QDeclarativeViewer::TestSkipProperty){
        QString e = m_view->rootObject()->property("skip").toString();
        if (!e.isEmpty()) {
            if(hasFailed){
                qWarning() << "Test failed, but skipping it: " << e;
            }else{
                qWarning() << "Test skipped: " << e;
            }
            hasFailed = 0;
        }
    }
}

void QDeclarativeTester::complete()
{
    if ((options & QDeclarativeViewer::TestErrorProperty) && !hasFailed) {
        QString e = m_view->rootObject()->property("error").toString();
        if (!e.isEmpty()) {
            qWarning() << "Test failed:" << e;
            hasFailed = true;
        }
    }


    testSkip();
    if (options & QDeclarativeViewer::ExitOnComplete)
        QApplication::exit(hasFailed?-1:0);

    if (hasCompleted)
        return;
    hasCompleted = true;

    if (options & QDeclarativeViewer::Play)
        qWarning("Script playback complete");
}

void QDeclarativeTester::run()
{
    QDeclarativeComponent c(m_view->engine(), m_script + QLatin1String(".qml"));

    testscript = qobject_cast<QDeclarativeVisualTest *>(c.create());
    if (testscript) testscript->setParent(this);
    else { executefailure(); exit(-1); }
    testscriptidx = 0;
}

void QDeclarativeTester::save()
{
    QString filename = m_script + QLatin1String(".qml");
    QFileInfo filenameInfo(filename);
    QDir saveDir = filenameInfo.absoluteDir();
    saveDir.mkpath(".");

    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QTextStream ts(&file);

    ts << "import Qt.VisualTest 4.7\n\n";
    ts << "VisualTest {\n";

    int imgCount = 0;
    QList<KeyEvent> keyevents = m_savedKeyEvents;
    QList<MouseEvent> mouseevents = m_savedMouseEvents;
    for (int ii = 0; ii < m_savedFrameEvents.count(); ++ii) {
        const FrameEvent &fe = m_savedFrameEvents.at(ii);
        ts << "    Frame {\n";
        ts << "        msec: " << fe.msec << "\n";
        if (!fe.hash.isEmpty()) {
            ts << "        hash: \"" << fe.hash.toHex() << "\"\n";
        } else if (!fe.image.isNull()) {
            QString filename = filenameInfo.baseName() + QLatin1Char('.')
                               + QString::number(imgCount) + ".png";
            fe.image.save(m_script + QLatin1Char('.') + QString::number(imgCount) + ".png");
            imgCount++;
            ts << "        image: \"" << filename << "\"\n";
        }
        ts << "    }\n";

        while (!mouseevents.isEmpty() &&
               mouseevents.first().msec == fe.msec) {
            MouseEvent me = mouseevents.takeFirst();

            ts << "    Mouse {\n";
            ts << "        type: " << me.type << "\n";
            ts << "        button: " << me.button << "\n";
            ts << "        buttons: " << me.buttons << "\n";
            ts << "        x: " << me.pos.x() << "; y: " << me.pos.y() << "\n";
            ts << "        modifiers: " << me.modifiers << "\n";
            if (me.destination == ViewPort)
                ts << "        sendToViewport: true\n";
            ts << "    }\n";
        }

        while (!keyevents.isEmpty() &&
               keyevents.first().msec == fe.msec) {
            KeyEvent ke = keyevents.takeFirst();

            ts << "    Key {\n";
            ts << "        type: " << ke.type << "\n";
            ts << "        key: " << ke.key << "\n";
            ts << "        modifiers: " << ke.modifiers << "\n";
            ts << "        text: \"" << ke.text.toUtf8().toHex() << "\"\n";
            ts << "        autorep: " << (ke.autorep?"true":"false") << "\n";
            ts << "        count: " << ke.count << "\n";
            if (ke.destination == ViewPort)
                ts << "        sendToViewport: true\n";
            ts << "    }\n";
        }
    }

    ts << "}\n";
    file.close();
}

void QDeclarativeTester::updateCurrentTime(int msec)
{
#ifndef NO_PRIVATE_HEADERS
    QDeclarativeItemPrivate::setConsistentTime(msec);
#endif
    if (!testscript && msec > 16 && options & QDeclarativeViewer::Snapshot)
        return;

    QImage img(m_view->width(), m_view->height(), QImage::Format_RGB32);

    if (options & QDeclarativeViewer::TestImages) {
        img.fill(qRgb(255,255,255));

#ifdef Q_OS_MAC
        bool oldSmooth = qt_applefontsmoothing_enabled;
        qt_applefontsmoothing_enabled = false;
#endif
        QPainter p(&img);
#ifdef Q_OS_MAC
        qt_applefontsmoothing_enabled = oldSmooth;
#endif

        m_view->render(&p);
    }

    bool snapshot = msec == 16 && (options & QDeclarativeViewer::Snapshot
                                   || (testscript && testscript->count() == 2));

    FrameEvent fe;
    fe.msec = msec;
    if (msec == 0 || !(options & QDeclarativeViewer::TestImages)) {
        // Skip first frame, skip if not doing images
    } else if (0 == ((m_savedFrameEvents.count()-1) % 60) || snapshot) {
        fe.image = img;
    } else {
        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData((const char *)img.bits(), img.bytesPerLine() * img.height());
        fe.hash = hash.result();
    }
    m_savedFrameEvents.append(fe);

    // Deliver mouse events
    filterEvents = false;

    if (!testscript) {
        for (int ii = 0; ii < m_mouseEvents.count(); ++ii) {
            MouseEvent &me = m_mouseEvents[ii];
            me.msec = msec;
            QMouseEvent event(me.type, me.pos, me.button, me.buttons, me.modifiers);

            if (me.destination == View) {
                QCoreApplication::sendEvent(m_view, &event);
            } else {
                QCoreApplication::sendEvent(m_view->viewport(), &event);
            }
        }

        for (int ii = 0; ii < m_keyEvents.count(); ++ii) {
            KeyEvent &ke = m_keyEvents[ii];
            ke.msec = msec;
            QKeyEvent event(ke.type, ke.key, ke.modifiers, ke.text, ke.autorep, ke.count);

            if (ke.destination == View) {
                QCoreApplication::sendEvent(m_view, &event);
            } else {
                QCoreApplication::sendEvent(m_view->viewport(), &event);
            }
        }
        m_savedMouseEvents.append(m_mouseEvents);
        m_savedKeyEvents.append(m_keyEvents);
    }

    m_mouseEvents.clear();
    m_keyEvents.clear();

    // Advance test script
    while (testscript && testscript->count() > testscriptidx) {

        QObject *event = testscript->event(testscriptidx);

        if (QDeclarativeVisualTestFrame *frame = qobject_cast<QDeclarativeVisualTestFrame *>(event)) {
            if (frame->msec() < msec) {
                if (options & QDeclarativeViewer::TestImages && !(options & QDeclarativeViewer::Record)) {
                    qWarning() << "QDeclarativeTester(" << m_script << "): Extra frame.  Seen:"
                               << msec << "Expected:" << frame->msec();
                    imagefailure();
                }
            } else if (frame->msec() == msec) {
                if (!frame->hash().isEmpty() && frame->hash().toUtf8() != fe.hash.toHex()) {
                    if (options & QDeclarativeViewer::TestImages && !(options & QDeclarativeViewer::Record)) {
                        qWarning() << "QDeclarativeTester(" << m_script << "): Mismatched frame hash at" << msec
                                   << ".  Seen:" << fe.hash.toHex()
                                   << "Expected:" << frame->hash().toUtf8();
                        imagefailure();
                    }
                }
            } else if (frame->msec() > msec) {
                break;
            }

            if (options & QDeclarativeViewer::TestImages && !(options & QDeclarativeViewer::Record) && !frame->image().isEmpty()) {
                QImage goodImage(frame->image().toLocalFile());
                if (frame->msec() == 16 && goodImage.size() != img.size()){
                    //Also an image mismatch, but this warning is more informative. Only checked at start though.
                    qWarning() << "QDeclarativeTester(" << m_script << "): Size mismatch. This test must be run at " << goodImage.size();
                    imagefailure();
                }
                if (goodImage != img) {
                    QString reject(frame->image().toLocalFile() + ".reject.png");
                    qWarning() << "QDeclarativeTester(" << m_script << "): Image mismatch.  Reject saved to:"
                               << reject;
                    img.save(reject);
                    bool doDiff = (goodImage.size() == img.size());
                    if (doDiff) {
                        QImage diffimg(m_view->width(), m_view->height(), QImage::Format_RGB32);
                        diffimg.fill(qRgb(255,255,255));
                        QPainter p(&diffimg);
                        int diffCount = 0;
                        for (int x = 0; x < img.width(); ++x) {
                            for (int y = 0; y < img.height(); ++y) {
                                if (goodImage.pixel(x,y) != img.pixel(x,y)) {
                                    ++diffCount;
                                    p.drawPoint(x,y);
                                }
                            }
                        }
                        QString diff(frame->image().toLocalFile() + ".diff.png");
                        diffimg.save(diff);
                        qWarning().nospace() << "                    Diff (" << diffCount << " pixels differed) saved to: " << diff;
                    }
                    imagefailure();
                }
            }
        } else if (QDeclarativeVisualTestMouse *mouse = qobject_cast<QDeclarativeVisualTestMouse *>(event)) {
            QPoint pos(mouse->x(), mouse->y());
            QPoint globalPos = m_view->mapToGlobal(QPoint(0, 0)) + pos;
            QMouseEvent event((QEvent::Type)mouse->type(), pos, globalPos, (Qt::MouseButton)mouse->button(), (Qt::MouseButtons)mouse->buttons(), (Qt::KeyboardModifiers)mouse->modifiers());

            MouseEvent me(&event);
            me.msec = msec;
            if (!mouse->sendToViewport()) {
                QCoreApplication::sendEvent(m_view, &event);
                me.destination = View;
            } else {
                QCoreApplication::sendEvent(m_view->viewport(), &event);
                me.destination = ViewPort;
            }
            m_savedMouseEvents.append(me);
        } else if (QDeclarativeVisualTestKey *key = qobject_cast<QDeclarativeVisualTestKey *>(event)) {

            QKeyEvent event((QEvent::Type)key->type(), key->key(), (Qt::KeyboardModifiers)key->modifiers(), QString::fromUtf8(QByteArray::fromHex(key->text().toUtf8())), key->autorep(), key->count());

            KeyEvent ke(&event);
            ke.msec = msec;
            if (!key->sendToViewport()) {
                QCoreApplication::sendEvent(m_view, &event);
                ke.destination = View;
            } else {
                QCoreApplication::sendEvent(m_view->viewport(), &event);
                ke.destination = ViewPort;
            }
            m_savedKeyEvents.append(ke);
        }
        testscriptidx++;
    }

    filterEvents = true;

    if (testscript && testscript->count() <= testscriptidx) {
        //if (msec == 16) //for a snapshot, leave it up long enough to see
        //    (void)::sleep(1);
        complete();
    }
}

void QDeclarativeTester::registerTypes()
{
    qmlRegisterType<QDeclarativeVisualTest>("Qt.VisualTest", 4,7, "VisualTest");
    qmlRegisterType<QDeclarativeVisualTestFrame>("Qt.VisualTest", 4,7, "Frame");
    qmlRegisterType<QDeclarativeVisualTestMouse>("Qt.VisualTest", 4,7, "Mouse");
    qmlRegisterType<QDeclarativeVisualTestKey>("Qt.VisualTest", 4,7, "Key");
}

QT_END_NAMESPACE
