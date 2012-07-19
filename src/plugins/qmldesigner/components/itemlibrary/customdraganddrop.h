/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include <QPixmap>
#include <QLabel>
#include <QTimeLine>
#include <QDrag>

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace QmlDesignerItemLibraryDragAndDrop {

class CustomDragAndDropIcon : public QLabel { //this class shows the icon for the drag drop operation
    Q_OBJECT
public:
    CustomDragAndDropIcon(QWidget *parent = 0) : QLabel(parent), m_oldTarget(0)
    {
    }
    void setPreview(const QPixmap &pixmap)
    { m_preview = pixmap; }
    void setIcon(const QPixmap &pixmap)
    { m_icon = pixmap; }

    void enter();
    void leave();
    void startDrag();
    void grabMouseSafely();

    bool isAnimated() const
    { return m_timeLine.state() == QTimeLine::Running; }

public slots:
    void animateDrag(int frame);

protected:
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *);
private:
    QWidget *m_oldTarget;
    QPixmap m_preview;
    QPixmap m_icon;
    QTimeLine m_timeLine;
    QSize m_size;
    qreal m_iconAlpha, m_previewAlpha;
};


class CustomDragAndDrop { //this class handles the drag and drop logic
public:
    static void startCustomDrag(const QPixmap icon, const QPixmap preview, QMimeData * mimeData);
    static bool customDragActive();
    static bool isAccepted();
    static void endCustomDrag();
    static void enter(QWidget *target, QPoint globalPos);
    static void leave(QWidget *target, QPoint globalPos);
    static void move(QWidget *target, QPoint globalPos);
    static void drop(QWidget *target, QPoint globalPos);
    static bool isAnimated();
    static void hide()
    { instance()->m_isVisible = false; }
    static void show()
    {
        instance()->m_isVisible = true;
        instance()->m_widget->show();
        instance()->m_widget->update();
    }
    static bool isVisible()
    { return instance()->m_isVisible; }

private:
    CustomDragAndDrop();
    static CustomDragAndDrop* instance();
    static CustomDragAndDrop* m_instance;

    CustomDragAndDropIcon *m_widget;
    bool m_customDragActive;
    QPoint m_pos;
    QMimeData *m_mimeData;
    bool m_accepted;
    bool m_isVisible;

    friend class CustomDragAndDropGuard;
};

} //namespace QmlDesignerItemLibraryDragAndDrop

class CustomItemLibraryDrag : public QDrag {
    Q_OBJECT
    public:
        CustomItemLibraryDrag(QWidget * dragSource) : QDrag(dragSource), m_mimeData(0)
        {}
        void setPixmap(const QPixmap &pixmap)
        { m_pixmap = pixmap; }
        void setPreview(const QPixmap &pixmap)
        { m_preview = pixmap; }
        void setMimeData(QMimeData *mimeData)
        { m_mimeData = mimeData; }
        void exec()
        { QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::startCustomDrag(m_pixmap, m_preview, m_mimeData); }

    public slots:
        void stopDrag() {
            QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::endCustomDrag();
        }

private:
        QPixmap m_pixmap, m_preview;
        QMimeData *m_mimeData;
    };

} //namespave QmlDesigner
