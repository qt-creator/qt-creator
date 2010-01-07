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

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
****************************************************************************/

#include "basicwidgets.h"
#include <qlayoutobject.h>
#include <private/graphicswidgets_p.h>
#include <qmlcontext.h>
#include <qmlengine.h>
#include <qmlcomponent.h>
#include <QtCore/QDebug>
#include <QFile>
#include <QPixmap>
#include <QTimeLine>
#include "colorwidget.h"
#include "filewidget.h"
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QListView>
#include <QDebug>
#include <QApplication>
#include <QGraphicsOpacityEffect>


QT_BEGIN_NAMESPACE


class QWidgetDeclarativeUI;

class ResizeEventFilter : public QObject
{
    Q_OBJECT
public:
    ResizeEventFilter(QObject *parent) : QObject(parent), m_target(0) { }

    void setTarget(QObject *target) { m_target = target; }
    void setDuiTarget(QWidgetDeclarativeUI* dui_target) {m_dui_target = dui_target;}

protected:
    bool eventFilter(QObject *obj, QEvent *event);
private:
    QObject *m_target;
    QWidgetDeclarativeUI* m_dui_target;
};

class QWidgetDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QmlList<QObject *> *children READ children)
    Q_PROPERTY(QLayoutObject *layout READ layout WRITE setLayout)
    Q_PROPERTY(QFont font READ font CONSTANT)

    Q_PROPERTY(QPoint pos READ pos)
    Q_PROPERTY(QSize size READ size)

    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(bool focus READ hasFocus NOTIFY focusChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)

    Q_PROPERTY(QUrl styleSheetFile READ styleSheetFile WRITE setStyleSheetFile NOTIFY styleSheetFileChanged)

    Q_PROPERTY(QColor windowColor READ windowColor WRITE setWindowColor)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor baseColor READ baseColor WRITE setBaseColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(QColor windowTextColor READ windowTextColor WRITE setWindowTextColor)
    Q_PROPERTY(QColor buttonTextColor READ buttonTextColor WRITE setButtonTextColor)

    Q_PROPERTY(int fixedWidth READ width WRITE setFixedWidth)
    Q_PROPERTY(int fixedHeight READ height WRITE setFixedHeight)

    Q_PROPERTY(bool mouseOver READ mouseOver NOTIFY mouseOverChanged)

    Q_CLASSINFO("DefaultProperty", "children")

signals:
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void focusChanged();
    void mouseOverChanged();
    void opacityChanged();

public:
    QWidgetDeclarativeUI(QObject *other) : QObject(other), _children(other), _layout(0), _graphicsOpacityEffect(0) {


        q = qobject_cast<QWidget*>(other);
        ResizeEventFilter *filter(new ResizeEventFilter(q));
        filter->setTarget(q);
        filter->setDuiTarget(this);
        m_mouseOver = false;
        q->installEventFilter(filter);
        Q_ASSERT(q);
    }
    virtual ~QWidgetDeclarativeUI() {
    }

    class Children : public QmlConcreteList<QObject *>
    {
    public:
        Children(QObject *widget) : q(qobject_cast<QWidget *>(widget)) {}
        virtual void append(QObject *o)
        {
            insert(-1, o);
        }
        virtual void clear()
        {
            for (int i = 0; i < count(); ++i)
                at(i)->setParent(0);
            QmlConcreteList<QObject *>::clear();
        }
        virtual void removeAt(int i)
        {
            at(i)->setParent(0);
            QmlConcreteList<QObject *>::removeAt(i);
        }
        virtual void insert(int i, QObject *o)
        {
            QmlConcreteList<QObject *>::insert(i, o);
            if (QWidget *w = qobject_cast<QWidget *>(o))
                w->setParent(static_cast<QWidget *>(q));
            else
                o->setParent(q);
        }

    private:
        QWidget *q;
    };

public:

    void setMouseOver(bool _mouseOver)
    {
        m_mouseOver = _mouseOver;
    }

    void emitResize()
    {
        emit widthChanged();
        emit heightChanged();
    }

    void emitMove()
    {
        emit xChanged();
        emit yChanged();
    }

    void emitFocusChanged()
    {
        emit focusChanged();
    }

     void emitMouseOverChanged()
    {
        emit mouseOverChanged();
    }

    QmlList<QObject *> *children() { return &_children; }

    QLayoutObject *layout() const { return _layout; }
    void setLayout(QLayoutObject *lo)
    {
        _layout = lo;
        static_cast<QWidget *>(parent())->setLayout(lo->layout());
    }

    QFont font() const
    {
        return _font;
    }

    void setFont(const QFont &font)
    {
        if (font != _font) {
            _font = font;
            static_cast<QWidget *>(parent())->setFont(_font);
        }
    }

    int x() const {
        return q->x();
    }

    bool hasFocus() const {
        return q->hasFocus();
    }

    bool mouseOver() const {
        return m_mouseOver;
    }

    void setX(int x) {
        q->move(x, y());
    }

    int y() const {
        return q->y();
    }

    qreal opacity() const {
        if (_graphicsOpacityEffect)
            return _graphicsOpacityEffect->opacity();
        else
            return 1;
    }

    void setOpacity(qreal newOpacity) {
        if (newOpacity != opacity()) {
            if (!_graphicsOpacityEffect) {
                _graphicsOpacityEffect = new QGraphicsOpacityEffect(this);
                q->setGraphicsEffect(_graphicsOpacityEffect);
            }
            _graphicsOpacityEffect->setOpacity(newOpacity);
            emit opacityChanged();
        }
    }

    void setY(int y) {
        q->move(x(), y);
    }

    int width() const {
        return q->width();
    }

    void setWidth(int width) {
        q->resize(width, height());
    }

    int height() const {
        return q->height();
    }

    void setHeight(int height) {
        q->resize(width(), height);
    }

    void setFixedWidth(int width) {
        q->setFixedWidth(width);
    }

    void setFixedHeight(int height) {
        q->setFixedHeight(height);
    }

    QPoint pos() const {
        return q->pos();
    }

    QSize size() const {
        return q->size();
    }

    QUrl styleSheetFile() const {
        return _styleSheetFile;
    }

    QColor windowColor() const
    { return q->palette().color(QPalette::Window); }

    void setWindowColor(const QColor &color)
    {
        QPalette pal = q->palette();
        pal.setColor(QPalette::Window, color);
        q->setPalette(pal);
    }

    QColor backgroundColor() const
    { return q->palette().color(QPalette::Background); }

    void setBackgroundColor(const QColor &color)
    {
        QPalette pal = q->palette();
        pal.setColor(QPalette::Background, color);
        q->setPalette(pal);
    }

    QColor baseColor() const
    { return q->palette().color(QPalette::Base); }

    void setBaseColor(const QColor &color)
    {
        QPalette pal = q->palette();
        pal.setColor(QPalette::Base, color);
        q->setPalette(pal);
    }

    QColor textColor() const
    { return q->palette().color(QPalette::Text); }

    void setTextColor(const QColor &color)
    {
        QPalette pal = q->palette();
        pal.setColor(QPalette::Text, color);
        q->setPalette(pal);
    }

    QColor windowTextColor() const
    { return q->palette().color(QPalette::WindowText); }

    void setWindowTextColor(const QColor &color)
    {
        QPalette pal = q->palette();
        pal.setColor(QPalette::WindowText, color);
        q->setPalette(pal);
    }

    QColor buttonTextColor() const
    { return q->palette().color(QPalette::ButtonText); }

    void setButtonTextColor(const QColor &color)
    {
        QPalette pal = q->palette();
        pal.setColor(QPalette::ButtonText, color);
        q->setPalette(pal);
    }

    void setStyleSheetFile(const QUrl &url) {
        _styleSheetFile = url;
        _styleSheetFile.setScheme("file"); //### todo
        QString fileName;
        if (!QFileInfo(_styleSheetFile.toLocalFile()).exists())
            fileName = (":" + _styleSheetFile.toLocalFile().split(":").last()); //try if it is a resource
        else
            fileName = (_styleSheetFile.toLocalFile());
        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        if (file.isOpen()) {
            QString styleSheet(file.readAll());
            q->setStyleSheet(styleSheet);
        } else {
            qWarning() << QLatin1String("setStyleSheetFile: ") << url << QLatin1String(" not found!");
        }

    }

private:
    QWidget *q;
    Children _children;
    QLayoutObject *_layout;
    QFont _font;
    QUrl _styleSheetFile;
    QGraphicsOpacityEffect *_graphicsOpacityEffect;
    bool m_mouseOver;
};

bool ResizeEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        if (obj
            && obj->isWidgetType()
            && obj == m_target) {
                m_dui_target->emitResize();
                return QObject::eventFilter(obj, event);
        }
    } else if (event->type() == QEvent::Move) {
        if (obj
            && obj->isWidgetType()
            && obj == m_target) {
                m_dui_target->emitMove();
                return QObject::eventFilter(obj, event);
        }
    } else if ((event->type() == QEvent::FocusIn) || (event->type() == QEvent::FocusOut)) {
        if (obj
            && obj->isWidgetType()
            && obj == m_target) {
                m_dui_target->emitFocusChanged();
                return QObject::eventFilter(obj, event);
        }
    } else if ((event->type() == QEvent::Enter) || (event->type() == QEvent::Leave)) {
        if (obj
            && obj->isWidgetType()
            && obj == m_target) {
                m_dui_target->setMouseOver(event->type() == QEvent::Enter);
                m_dui_target->emitMouseOverChanged();
                return QObject::eventFilter(obj, event);
        }
    }
    return QObject::eventFilter(obj, event);
}


class QTabObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QWidget *content READ content WRITE setContent)
    Q_PROPERTY(QString label READ label WRITE setLabel)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_CLASSINFO("DefaultProperty", "content")
public:
    QTabObject(QObject *parent = 0) : QObject(parent), _content(0) {}

    QWidget *content() const { return _content; }
    void setContent(QWidget *content)
    {
        _content = content;
    }

    QString label() const { return _label; }
    void setLabel(const QString &label)
    {
        _label = label;
    }

    QIcon icon() const { return _icon; }
    void setIcon(const QIcon &icon)
    {
        _icon = icon;
    }

private:
    QWidget *_content;
    QString _label;
    QIcon _icon;
};

class QPushButtonDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl iconFromFile READ iconFromFile WRITE setIconFromFile)
public:
    QPushButtonDeclarativeUI(QObject *parent = 0) : QObject(parent)
     {
         pb = qobject_cast<QPushButton*>(parent);
     }
private:
    QUrl iconFromFile() const
    {
        return _url;
    }

    void setIconFromFile(const QUrl &url) {
        _url = url;

        QString path;
        if (_url.scheme() == QLatin1String("qrc")) {
            path = ":" + _url.path();
        } else {
            path = _url.toLocalFile();
        }

        QFile file(path);
        file.open(QIODevice::ReadOnly);
        if (file.exists() && file.isOpen()) {
            QPixmap pixmap(path);
            if (pixmap.isNull())
                qWarning() << QLatin1String("setIconFromFile: ") << url << QLatin1String(" not found!");
            pb->setIcon(pixmap);
        } else {
            qWarning() << QLatin1String("setIconFromFile: ") << url << QLatin1String(" not found!");
        }

    }

    QPushButton *pb;
    QUrl _url;
};

class QMenuDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QmlList<Action *> *actions READ actions)
    Q_CLASSINFO("DefaultProperty", "actions")

public:
    QMenuDeclarativeUI(QObject *parent = 0) : QObject(parent), _actions(parent)
     {
         menu = qobject_cast<QMenu*>(parent);
     }

     QmlList<Action *> *actions() { return &_actions; }

private:
    //if not for the at() function, we could use QmlList instead
    class Actions : public QmlConcreteList<Action *>
    {
    public:
        Actions(QObject *o) : menu(qobject_cast<QMenu*>(o)) {}
        virtual void append(Action *o)
        {
            QmlConcreteList<Action *>::append(o);
            o->setParent(menu);
            menu->addAction(o);
        }
        virtual void clear()
        {
            QmlConcreteList<Action *>::clear();
            menu->clear();
        }
        virtual void removeAt(int i)
        {
            QmlConcreteList<Action *>::removeAt(i);
            menu->removeAction(menu->actions().at(i));
        }
        virtual void insert(int i, Action *obj)
        {
            QmlConcreteList<Action *>::insert(i, obj);
            obj->setParent(menu);
            menu->addAction(obj);
        }
    private:
        QMenu *menu;
    };

    QMenu *menu;
    Actions _actions;
};

class QToolButtonDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl iconFromFile READ iconFromFile WRITE setIconFromFile)
    Q_PROPERTY(QMenu *menu READ menu WRITE setMenu)

public:
    QToolButtonDeclarativeUI(QObject *parent = 0) : QObject(parent)
     {
         pb = qobject_cast<QToolButton*>(parent);
     }
private:

    QMenu *menu() const { return pb->menu(); }
    void setMenu(QMenu *menu)
    {
        pb->setMenu(menu);
    }

    QUrl iconFromFile() const
    {
        return _url;
    }

    void setIconFromFile(const QUrl &url) {
        _url = url;

        QString path;
        if (_url.scheme() == QLatin1String("qrc")) {
            path = ":" + _url.path();
        } else {
            path = _url.toLocalFile();
        }

        QFile file(path);
        file.open(QIODevice::ReadOnly);
        if (file.exists() && file.isOpen()) {
            QPixmap pixmap(path);
            if (pixmap.isNull())
                qWarning() << QLatin1String("setIconFromFile: ") << url << QLatin1String(" not found!");
            pb->setIcon(pixmap);
        } else {
            qWarning() << QLatin1String("setIconFromFile: ") << url << QLatin1String(" not found!");
        }

    }

    QToolButton *pb;
    QUrl _url;
};

class QComboBoxDeclarativeUI : public QObject
{
     Q_OBJECT
     Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemChanged)
     Q_PROPERTY(QString currentText READ currentText WRITE setCurrentText NOTIFY currentTextChanged)

public:
     QComboBoxDeclarativeUI(QObject *parent = 0) : QObject(parent), m_itemsSet(false)
     {
         cb = qobject_cast<QComboBox*>(parent);
         connect(cb, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setCurrentText(const QString &)));
     }

     QString currentText() const
     {
         return cb->currentText();
     }

public slots:
     void setCurrentText(const QString &text)
     {
         if (!m_itemsSet)
             return;
         int i = cb->findText(text);
         if (i  != -1) {
             cb->setCurrentIndex(i);
             emit currentTextChanged();
         }
     }
signals:
    void currentTextChanged();
    void itemsChanged();

private:
    void setItems(const QStringList &list)
    {
        _items = list;
        cb->clear();
        cb->addItems(list);
        m_itemsSet = true;
        emit itemsChanged();
    }

    QStringList items() const
    {
        return _items;
    }

    QComboBox *cb;
    QStringList _items;

    bool m_itemsSet;
};

class QScrollAreaDeclarativeUI : public QObject
{
     Q_OBJECT
     Q_PROPERTY(QWidget *content READ content WRITE setContent)
     Q_CLASSINFO("DefaultProperty", "content")

public slots:
    void setupProperWheelBehaviour();

public:
     QScrollAreaDeclarativeUI(QObject *parent = 0) : QObject(parent), _content(0)
     {
         sa = qobject_cast<QScrollArea*>(parent);
     }

private:
    QWidget *content() const { return _content; }
    void setContent(QWidget *content)
    {
        _content = content;
        sa->setWidget(content);
        sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        sa->verticalScrollBar()->show();
        setupProperWheelBehaviour();
    }

    QWidget *_content;
    QScrollArea *sa;
};

class MouseWheelFilter : public QObject
{
    Q_OBJECT
public:
    MouseWheelFilter(QObject *parent) : QObject(parent), m_target(0) { }

    void setTarget(QObject *target) { m_target = target; }

protected:
    bool eventFilter(QObject *obj, QEvent *event);
private:
    QObject *m_target;
};

bool MouseWheelFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        if (obj
            && obj->isWidgetType()
            && obj != m_target) {
                QApplication::sendEvent(m_target, event);
                return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void QScrollAreaDeclarativeUI::setupProperWheelBehaviour()
{
// We install here an eventfilter to avoid that scrolling in
// in the ScrollArea changes values in editor widgets
    if (_content) {
        MouseWheelFilter *forwarder(new MouseWheelFilter(this));
        forwarder->setTarget(_content);

        foreach (QWidget *childWidget, _content->findChildren<QWidget*>()) {
            childWidget->installEventFilter(forwarder);
        }
    }
}

class WidgetLoader : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString sourceString READ sourceString WRITE setSourceString NOTIFY sourceChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QWidget *widget READ widget NOTIFY widgetChanged)
    Q_PROPERTY(QmlComponent *component READ component NOTIFY sourceChanged)

public:
    WidgetLoader(QWidget * parent = 0) : QWidget(parent), m_source(QUrl()), m_widget(0),
                                         m_component(0), m_layout(0)
    {
        m_layout = new QVBoxLayout(this);
        m_layout->setContentsMargins(0,0,0,0);
    }

    QUrl source() const;
    void setSource(const QUrl &);

    QString sourceString() const
    { return m_source.toString(); }
    void setSourceString(const QString &url)
    { setSource(QUrl(url)); }

    QWidget *widget() const;
    QmlComponent *component() const
    { return m_component; }

signals:
    void widgetChanged();
    void sourceChanged();

private:
    QUrl m_source;
    QWidget *m_widget;
    QmlComponent *m_component;
    QVBoxLayout *m_layout;

};

QUrl WidgetLoader::source() const
{
    return m_source;
}

void WidgetLoader::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    if (m_component) {
        delete m_component;
        m_component = 0;
    }

    QWidget *oldWidget = m_widget;

    if (m_widget) {
        //m_widget->deleteLater();
        m_widget->hide();
        m_widget = 0;
    }

    m_source = source;
    if (m_source.isEmpty()) {
        emit sourceChanged();
        emit widgetChanged();
        return;
    }

    m_component = new QmlComponent(qmlEngine(this), m_source, this);

    if (m_component) {
        emit sourceChanged();
        emit widgetChanged();

        while (m_component->isLoading())
            QApplication::processEvents();

        if (!m_component->isReady()) {
            if (!m_component->errors().isEmpty())
                qWarning() << m_component->errors();
            emit sourceChanged();
            return;
        }

        QmlContext *ctxt = new QmlContext(qmlContext(this));
        ctxt->addDefaultObject(this);

        QObject *obj = m_component->create(ctxt);
        if (obj) {
            QWidget *widget = qobject_cast<QWidget *>(obj);
            if (widget) {
                m_widget = widget;
                m_layout->addWidget(m_widget);
                m_widget->show();
            }
        }
    }

}

QWidget *WidgetLoader::widget() const
{
    return m_widget;
}

class MyGroupBox : public QGroupBox
{
     Q_OBJECT

public:
    MyGroupBox(QWidget * parent = 0) : QGroupBox(parent), m_animated(false)
    {}

void setPixmap(const QPixmap &pixmap, qreal alpha = 1)
{ m_pixmap = pixmap; m_alpha = alpha;}

void setAnimated(bool animated)
{ m_animated = animated; }

public slots:
    virtual void setVisible ( bool visible );

protected:
    virtual void paintEvent(QPaintEvent * event);
private:
    qreal m_alpha;
    QPixmap m_pixmap;
    bool m_animated;
};

void MyGroupBox::paintEvent(QPaintEvent * event)
{
    QGroupBox::paintEvent(event);
    if (m_animated) {
        QPainter p(this);
        p.setOpacity(m_alpha);
        p.drawPixmap(5, 5,  m_pixmap.width(), m_pixmap.height(), m_pixmap);
    }
}

void MyGroupBox::setVisible ( bool visible )
{
    if (parentWidget())
        QGroupBox::setVisible(visible);
}

class QGroupBoxDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)
public:
    QGroupBoxDeclarativeUI(QObject *parent = 0) : QObject(parent), m_expanded(true)
    {
        gb =  qobject_cast<MyGroupBox*>(parent);
        connect(&m_timeLine, SIGNAL(frameChanged(int)), this, SLOT(animate(int)));
        connect(&m_timeLine, SIGNAL(finished()), this, SLOT(finish()));

        m_timeLine.setDuration(150);
        m_timeLine.setFrameRange(0, 5);
    }

    bool isCollapsed()
    { return ! m_expanded; }

    void setCollapsed(bool collapsed)
    {
        if (collapsed)
            collapse();
        else
            expand();
    }

public slots:
    void collapse();
    void expand();

    void animate(int frame);
    void finish();

private:
    MyGroupBox *gb;
    QTimeLine m_timeLine;
    bool m_expanded;
    int m_oldHeight;
    int m_oldMAxHeight;
    QPixmap m_contens;

    void hideChildren();
    void showChildren();

    void reLayout();
};

void QGroupBoxDeclarativeUI::reLayout()
{
    QLayout *layout = gb->parentWidget()->layout();
    if (layout) {
        layout->activate();
        layout->update();
    }
}

void QGroupBoxDeclarativeUI::finish()
{
    gb->setAnimated(false);
    if (m_expanded) {
        showChildren();

        gb->setMaximumHeight(m_oldMAxHeight);
    }
    else {
        gb->setMaximumHeight(30);
    }
    reLayout();
}

void QGroupBoxDeclarativeUI::hideChildren()
{
    if (gb->isVisible()) {
        gb->setMinimumHeight(gb->height());
        foreach (QWidget *widget, gb->findChildren<QWidget*>()) {
            widget->setProperty("wasVisibleGB", widget->property("visible"));
        }
        foreach (QWidget *widget, gb->findChildren<QWidget*>())
            if (widget->parent() == gb)
                widget->hide();
    }
}

void QGroupBoxDeclarativeUI::showChildren()
{
    foreach (QWidget *widget, gb->findChildren<QWidget*>()) {
        if (widget->property("wasVisibleGB").toBool())
          widget->show();
        widget->setProperty("wasVisibleGB", QVariant());
        widget->ensurePolished();
    }
    gb->show();
}

void QGroupBoxDeclarativeUI::collapse()
{
    m_oldMAxHeight = gb->maximumHeight();
    m_oldHeight = gb->height();
    if (!m_expanded)
        return;
    m_contens = QPixmap::grabWidget (gb, 5, 5, gb->width() - 5, gb->height() - 5);
    gb->setPixmap(m_contens,1);
    hideChildren();
    m_expanded = false;
    m_timeLine.start();
}

void QGroupBoxDeclarativeUI::expand()
{
    if (m_expanded)
        return;
    m_expanded = true;
    m_timeLine.start();
}

void QGroupBoxDeclarativeUI::animate(int frame)
{
    gb->setAnimated(true);
    qreal height;

    if (m_expanded) {
        height = ((qreal(frame) / 5.0) * qreal(m_oldHeight)) + (30.0 * (1.0 - qreal(frame) / 5.0));
        gb->setPixmap(m_contens, qreal(frame) / 5.0);
    }
    else {
        height = ((qreal(frame) / 5.0) * 30.0) + (qreal(m_oldHeight) * (1.0 - qreal(frame) / 5.0));
        qreal alpha = 0.8 - qreal(frame) / 4.0;
        if (alpha < 0)
            alpha = 0;
        gb->setPixmap(m_contens, alpha);
    }

    gb->setMaximumHeight(height);
    gb->setMinimumHeight(height);
    reLayout();
    gb->update();
}

QML_DECLARE_TYPE(QTabObject);
QML_DEFINE_TYPE(Qt,4,6,QTabObject,QTabObject); //### with namespacing, this should just be 'Tab'

class QTabWidgetDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QmlList<QTabObject *> *tabs READ tabs)
    Q_CLASSINFO("DefaultProperty", "tabs")
public:
    QTabWidgetDeclarativeUI(QObject *other) : QObject(other), _tabs(other) {}

    QmlList<QTabObject *> *tabs() { return &_tabs; }

private:
    //if not for the at() function, we could use QmlList instead
    class Tabs : public QmlConcreteList<QTabObject *>
    {
    public:
        Tabs(QObject *o) : tw(o) {}
        virtual void append(QTabObject *o)
        {
            QmlConcreteList<QTabObject *>::append(o);
            //XXX can we insertTab(-1, o) instead?
            if (!o->icon().isNull())
                static_cast<QTabWidget *>(tw)->addTab(o->content(), o->icon(), o->label());
            else
                static_cast<QTabWidget *>(tw)->addTab(o->content(), o->label());
        }
        virtual void clear()
        {
            QmlConcreteList<QTabObject *>::clear();
            static_cast<QTabWidget *>(tw)->clear();
        }
        virtual void removeAt(int i)
        {
            QmlConcreteList<QTabObject *>::removeAt(i);
            static_cast<QTabWidget *>(tw)->removeTab(i);
        }
        virtual void insert(int i, QTabObject *obj)
        {
            QmlConcreteList<QTabObject *>::insert(i, obj);
            if (!obj->icon().isNull())
                static_cast<QTabWidget *>(tw)->insertTab(i, obj->content(), obj->icon(), obj->label());
            else
                static_cast<QTabWidget *>(tw)->insertTab(i, obj->content(), obj->label());
        }
    private:
        QObject *tw;
    };
    Tabs _tabs;
};


class WidgetFrame : public QFrame
{
    Q_OBJECT
public:
    WidgetFrame( QWidget * parent = 0, Qt::WindowFlags f = 0 ) : QFrame(parent, f)
    {}
};

QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QWidget,QWidget,QWidgetDeclarativeUI);

//display
QML_DEFINE_TYPE(Qt,4,6,QLabel,QLabel);
QML_DEFINE_TYPE(Qt,4,6,QProgressBar,QProgressBar);
QML_DEFINE_TYPE(Qt,4,6,QLCDNumber,QLCDNumber);

//input
QML_DEFINE_TYPE(Qt,4,6,QLineEdit,QLineEdit);
QML_DEFINE_TYPE(Qt,4,6,QTextEdit,QTextEdit);
QML_DEFINE_TYPE(Qt,4,6,QPlainTextEdit,QPlainTextEdit);
QML_DEFINE_TYPE(Qt,4,6,QSpinBox,QSpinBox);
QML_DEFINE_TYPE(Qt,4,6,QDoubleSpinBox,QDoubleSpinBox);
QML_DEFINE_TYPE(Qt,4,6,QSlider,QSlider);
QML_DEFINE_TYPE(Qt,4,6,QDateTimeEdit,QDateTimeEdit);
QML_DEFINE_TYPE(Qt,4,6,QDateEdit,QDateEdit);
QML_DEFINE_TYPE(Qt,4,6,QTimeEdit,QTimeEdit);
QML_DEFINE_TYPE(Qt,4,6,QFontComboBox,QFontComboBox);
QML_DEFINE_TYPE(Qt,4,6,QDial,QDial);
QML_DEFINE_TYPE(Qt,4,6,QScrollBar,QScrollBar);
QML_DEFINE_TYPE(Qt,4,6,QCalendarWidget, QCalendarWidget);


QML_DECLARE_TYPE(MyGroupBox);
QML_DECLARE_TYPE(WidgetLoader);
QML_DECLARE_TYPE(WidgetFrame);
//QML_DEFINE_TYPE(Qt,4,6,QComboBox,QComboBox); //need a way to populate
//QML_DEFINE_EXTENDED_TYPE(QComboBox,QComboBox, QComboBox); //need a way to populate

//buttons
//QML_DEFINE_TYPE(Qt,4,6,QPushButton,QPushButton);
QML_DEFINE_TYPE(Qt,4,6,QCheckBox,QCheckBox);
QML_DEFINE_TYPE(Qt,4,6,QAction,Action);
QML_DEFINE_TYPE(Qt,4,6,QRadioButton,QRadioButton);
QML_DEFINE_TYPE(Qt,4,6,FileWidget, FileWidget);
QML_DEFINE_TYPE(Qt,4,6,LayoutWidget, LayoutWidget);

//containers
QML_DEFINE_TYPE(Qt,4,6,QFrame,QFrame);
QML_DEFINE_TYPE(Qt,4,6,WidgetFrame,WidgetFrame);
QML_DEFINE_TYPE(Qt,4,6,WidgetLoader,WidgetLoader);
QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QExtGroupBox,MyGroupBox,QGroupBoxDeclarativeUI);
QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QTabWidget,QTabWidget,QTabWidgetDeclarativeUI);
QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QScrollArea,QScrollArea,QScrollAreaDeclarativeUI);
QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QPushButton,QPushButton,QPushButtonDeclarativeUI);
QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QToolButton,QToolButton, QToolButtonDeclarativeUI);
QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QComboBox,QComboBox, QComboBoxDeclarativeUI);
QML_DEFINE_EXTENDED_TYPE(Qt,4,6,QMenu,QMenu, QMenuDeclarativeUI);
//QML_DEFINE_TYPE(Qt,4,6,QToolBox,QToolBox);
//QML_DEFINE_TYPE(Qt,4,6,QScrollArea,QScrollArea);

//QML_DEFINE_EXTENDED_TYPE(QtColorButton,QtColorButton,QtColorButtonDeclarativeUI);

//itemviews
//QML_DEFINE_TYPE(Qt,4,6,QListView,QListView);
//QML_DEFINE_TYPE(Qt,4,6,QTreeView,QTreeView);
//QML_DEFINE_TYPE(Qt,4,6,QTableView,QTableView);


#include "basicwidgets.moc"

QT_END_NAMESPACE
