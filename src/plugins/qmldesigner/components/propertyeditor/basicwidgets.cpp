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

#include "basicwidgets.h"
#include <utils/fileutils.h>
#include <qlayoutobject.h>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include <QDebug>
#include <QFile>
#include <QPixmap>
#include <QTimeLine>
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QListView>
#include <QDebug>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QStyleFactory>
#include <QScopedPointer>

#include <QTextEdit>


QT_BEGIN_NAMESPACE

class WindowsStyleSingleton
{
   public:
     static WindowsStyleSingleton* instance();
     QStyle* style() { return m_style.data(); };

   private:
     static WindowsStyleSingleton *m_instance;

     QScopedPointer<QStyle> m_style;

     WindowsStyleSingleton() : m_style(QStyleFactory::create(QLatin1String("windows"))) {}
     WindowsStyleSingleton( const WindowsStyleSingleton& );

     class WindowsWatcher {
         public: ~WindowsWatcher() {
           if ( WindowsStyleSingleton::m_instance != 0 )
             delete WindowsStyleSingleton::m_instance;
         }
     };
     friend class WindowsWatcher;
};

WindowsStyleSingleton* WindowsStyleSingleton::m_instance = 0;

WindowsStyleSingleton* WindowsStyleSingleton::instance()
{
  static WindowsWatcher w;
  if ( m_instance == 0 )
    m_instance = new WindowsStyleSingleton();
  return m_instance;
}


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

    Q_PROPERTY(QDeclarativeListProperty<QObject> children READ children)
    Q_PROPERTY(QLayoutObject *layout READ layout WRITE setLayout)
    Q_PROPERTY(QDeclarativeListProperty<Action> actions READ actions)
    Q_PROPERTY(QFont font READ font WRITE setFont)

    Q_PROPERTY(QPoint pos READ pos)
    Q_PROPERTY(QSize size READ size)

    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)

    Q_PROPERTY(int globalX READ globalX WRITE setGlobalX)
    Q_PROPERTY(int globalY READ globalY WRITE setGlobalY)

    Q_PROPERTY(bool focus READ hasFocus NOTIFY focusChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)

    Q_PROPERTY(QUrl styleSheetFile READ styleSheetFile WRITE setStyleSheetFile)

    Q_PROPERTY(QColor windowColor READ windowColor WRITE setWindowColor)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QColor baseColor READ baseColor WRITE setBaseColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(QColor windowTextColor READ windowTextColor WRITE setWindowTextColor)
    Q_PROPERTY(QColor buttonTextColor READ buttonTextColor WRITE setButtonTextColor)
    Q_PROPERTY(QColor buttonColor READ buttonColor WRITE setButtonColor)

    Q_PROPERTY(int fixedWidth READ width WRITE setFixedWidth)
    Q_PROPERTY(int fixedHeight READ height WRITE setFixedHeight)

    Q_PROPERTY(bool mouseOver READ mouseOver NOTIFY mouseOverChanged)

    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    Q_CLASSINFO("DefaultProperty", "children")

signals:
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void focusChanged();
    void mouseOverChanged();
    void opacityChanged();
    void visibleChanged();
    void enabledChanged();

public:
    QWidgetDeclarativeUI(QObject *other) : QObject(other), _layout(0), _graphicsOpacityEffect(0) {
        q = qobject_cast<QWidget*>(other);
        ResizeEventFilter *filter(new ResizeEventFilter(q));
        filter->setTarget(q);
        filter->setDuiTarget(this);
        m_mouseOver = false;
        q->installEventFilter(filter);
        q->setStyle(WindowsStyleSingleton::instance()->style());
        Q_ASSERT(q);
    }
    virtual ~QWidgetDeclarativeUI() {
    }

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


     void emitVisibleChanged()
     {
         emit visibleChanged();
     }

     void emitEnabledChanged()
     {
         emit enabledChanged();
     }

     QDeclarativeListProperty<QObject> children() {
         return QDeclarativeListProperty<QObject>(this, 0, children_append, children_count, children_at, children_clear);
    }

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

    int globalX() const {
        if (q->parentWidget())
            return q->mapToGlobal(QPoint(x(), 0)).x();
        else
            return x();
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

    void setGlobalX(int newX) {
        if (q->parentWidget())
            setX(q->mapFromGlobal(QPoint(newX, 1)).x());
        else
            setX(newX);
    }

    int y() const {
        return q->y();
    }

    bool visible() const {
        return q->isVisible();
    }

    void setVisible(bool visible) {
        q->setVisible(visible);
    }

    bool enabled() const {
        return q->isEnabled();
    }

    void setEnabled(bool enabled) {
        q->setEnabled(enabled);
    }

    int globalY() const {
        if (q->parentWidget())
            return q->mapToGlobal(QPoint(1,y())).y();
        else return y();
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

    void setGlobalY(int newY) {
        if (q->parentWidget())
            setY(q->parentWidget()->mapFromGlobal(QPoint(1, newY)).y());
        else
            setY(newY);
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

    QColor buttonColor() const
    { return q->palette().color(QPalette::Button); }

    void setButtonColor(const QColor &color)
    {
        QPalette pal = q->palette();
        pal.setColor(QPalette::Button, color);
        q->setPalette(pal);
    }


    void setStyleSheetFile(const QUrl &url) {
        _styleSheetFile = url;
        _styleSheetFile.setScheme("file"); //### todo
        QString fileName;
        if (!QFileInfo(_styleSheetFile.toLocalFile()).exists())
            fileName = (QLatin1Char(':') + _styleSheetFile.toLocalFile().split(QLatin1Char(':')).last()); //try if it is a resource
        else
            fileName = (_styleSheetFile.toLocalFile());
        Utils::FileReader reader;
        if (reader.fetch(fileName))
            q->setStyleSheet(QString::fromLatin1(reader.data()));
        else
            qWarning() << QString::fromLatin1("setStyleSheetFile: %1").arg(reader.errorString());

    }

    QDeclarativeListProperty<Action> actions() {
        return QDeclarativeListProperty<Action>(this, 0, actions_append, actions_count, actions_at, actions_clear);
    }

private:
    QWidget *q;
    QLayoutObject *_layout;
    QFont _font;
    QUrl _styleSheetFile;
    QGraphicsOpacityEffect *_graphicsOpacityEffect;
    bool m_mouseOver;

    static void children_append(QDeclarativeListProperty<QObject> *property, QObject *o) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *q = p->q;
        if (QWidget *w = qobject_cast<QWidget *>(o))
            w->setParent(static_cast<QWidget *>(q));
        else
            o->setParent(q);
    }

    static int children_count(QDeclarativeListProperty<QObject> *property) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *q = p->q;
        return q->children().count();
    }

    static QObject * children_at(QDeclarativeListProperty<QObject> *property, int index) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *q = p->q;
        return q->children().at(index);
    }

    static void children_clear(QDeclarativeListProperty<QObject> *property) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *q = p->q;
        QObjectList c = q->children();
        for (int i = 0; i < c.count(); ++i)
            c.at(i)->setParent(0);
    }

    // ### Original had an insert, and removeAt
    static void actions_append(QDeclarativeListProperty<Action> *property, Action *o) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *w = p->q;
        o->setParent(w);
        w->addAction(o);
    }

    static int actions_count(QDeclarativeListProperty<Action> *property) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *w = p->q;
        return w->actions().count();
    }
    static Action *actions_at(QDeclarativeListProperty<Action> *property, int index) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *w = p->q;
        return qobject_cast<Action *>(w->actions().at(index));
    }

    static void actions_clear(QDeclarativeListProperty<Action> *property) {
        QWidgetDeclarativeUI *p = static_cast<QWidgetDeclarativeUI *>(property->object);
        QWidget *w = p->q;

        while (!w->actions().empty())
            w->removeAction(w->actions().first());
    }
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
    } else if ((event->type() == QEvent::Show) || (event->type() == QEvent::Hide)) {
        if (obj
            && obj->isWidgetType()
            && obj == m_target) {
                m_dui_target->emitVisibleChanged();
                return QObject::eventFilter(obj, event);
            }
    } else if (event->type() == QEvent::EnabledChange) {
        if (obj
            && obj->isWidgetType()
            && obj == m_target) {
                m_dui_target->emitEnabledChanged();
                return QObject::eventFilter(obj,event);
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
        if (_url.scheme() == QLatin1String("qrc"))
            path = QLatin1Char(':') + _url.path();
        else
            path = _url.toLocalFile();

        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QPixmap pixmap(path);
            if (pixmap.isNull())
                qWarning() << "setIconFromFile: failed to load" << path;
            pb->setIcon(pixmap);
        } else {
            qWarning() << QString::fromLatin1("setIconFromFile: %1: %2").arg(path, file.errorString());
        }

    }

    QPushButton *pb;
    QUrl _url;
};

class ElidingLabel : public QLabel
{
    Q_OBJECT
public:
    ElidingLabel(QWidget * parent = 0) : QLabel(parent) {}

protected:
    void paintEvent(QPaintEvent *event)
    {
        QFontMetrics fm(font());
        if (fm.width(text()) > (contentsRect().width() - 8) && text().length() > 4) {
            QPainter p(this);
            QString elided_txt;
            elided_txt = fm.elidedText(text(), Qt::ElideRight, contentsRect().width() - 8, Qt::TextShowMnemonic);
            p.drawText(contentsRect().adjusted(12, 0, 0, 0), Qt::TextSingleLine, elided_txt);
        }
        else
            QLabel::paintEvent(event);
       }

};


class QLabelDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl iconFromFile READ iconFromFile WRITE setIconFromFile)
public:
    QLabelDeclarativeUI(QObject *parent = 0) : QObject(parent)
     {
         lb = qobject_cast<ElidingLabel*>(parent);
     }
private:
    QUrl iconFromFile() const
    {
        return _url;
    }

    void setIconFromFile(const QUrl &url) {
        _url = url;

        QString path;
        if (_url.scheme() == QLatin1String("qrc"))
            path = QLatin1Char(':') + _url.path();
        else
            path = _url.toLocalFile();

        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QPixmap pixmap(path);
            if (pixmap.isNull())
                qWarning() << "setIconFromFile: failed to load" << path;
            lb->setPixmap(pixmap);
        } else {
            qWarning() << QString::fromLatin1("setIconFromFile: %1: %2").arg(path, file.errorString());
        }

    }

    ElidingLabel *lb;
    QUrl _url;
};

class QMenuDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("DefaultProperty", "actions")

public:
    QMenuDeclarativeUI(QObject *parent = 0) : QObject(parent)
     {
         menu = qobject_cast<QMenu*>(parent);
     }

private:
    //if not for the at() function, we could use QDeclarativeList instead

    QMenu *menu;

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
        menu->setParent(0);
        pb->setMenu(menu);
        menu->setParent(QApplication::topLevelWidgets().first());
    }

    QUrl iconFromFile() const
    {
        return _url;
    }

    void setIconFromFile(const QUrl &url) {
        _url = url;

        QString path;
        if (_url.scheme() == QLatin1String("qrc"))
            path = QLatin1Char(':') + _url.path();
        else
            path = _url.toLocalFile();

        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QPixmap pixmap(path);
            if (pixmap.isNull())
                qWarning() << "setIconFromFile: failed to load" << path;
            pb->setIcon(pixmap);
        } else {
            qWarning() << QString::fromLatin1("setIconFromFile: %1: %2").arg(path, file.errorString());
        }

    }

    QToolButton *pb;
    QUrl _url;
};

class AnimatedToolButton : public QToolButton
{

Q_OBJECT

    Q_PROPERTY(QUrl hoverIconFromFile READ hoverIconFromFile WRITE setHoverIconFromFile)

public:
    AnimatedToolButton( QWidget * parent = 0 ) : QToolButton(parent)
    {}

private:
    QUrl hoverIconFromFile() const
    { return m_hoverIconUrl; }

    void setHoverIconFromFile(const QUrl &url)
    {  m_hoverIconUrl= url;
       m_hoverIcon = QIcon(m_hoverIconUrl.toLocalFile());
    }

    void leaveEvent(QEvent *)
    {
        setIcon(m_icon);
    }

    void enterEvent(QEvent *)
    {
        m_icon = icon();
        setIcon(m_hoverIcon);
    }

    QUrl m_hoverIconUrl;
    QIcon m_hoverIcon;
    QIcon m_icon;

};

class QComboBoxDeclarativeUI : public QObject
{
     Q_OBJECT
     Q_PROPERTY(QStringList items READ items WRITE setItems)
     Q_PROPERTY(QString currentText READ currentText WRITE setCurrentText NOTIFY currentTextChanged)

public:
     QComboBoxDeclarativeUI(QObject *parent = 0) : QObject(parent), m_itemsSet(false)
     {
         cb = qobject_cast<QComboBox*>(parent);
         connect(cb, SIGNAL(currentIndexChanged(QString)), this, SLOT(setCurrentText(QString)));
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
public slots:
     void setCurrentTextSilent(const QString &text)
     {
         if (!m_itemsSet)
             return;
         int i = cb->findText(text);
         if (i  != -1) {
             blockSignals(true);
             cb->setCurrentIndex(i);
             blockSignals(false);
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
        blockSignals(true);
        cb->addItems(list);
        m_itemsSet = true;
        blockSignals(false);
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
        sa->verticalScrollBar()->show();
    }

    QWidget *_content;
    QScrollArea *sa;
};


class WidgetLoader : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString sourceString READ sourceString WRITE setSourceString NOTIFY sourceChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl)
    Q_PROPERTY(QString qmlData READ qmlData WRITE setQmlData NOTIFY qmlDataChanged)
    Q_PROPERTY(QWidget *widget READ widget NOTIFY widgetChanged)
    Q_PROPERTY(QDeclarativeComponent *component READ component NOTIFY sourceChanged)

public:
    WidgetLoader(QWidget * parent = 0) : QWidget(parent), m_source(QUrl()), m_widget(0),
                                         m_component(0), m_layout(0)
    {
        m_layout = new QVBoxLayout(this);
        m_layout->setContentsMargins(0,0,0,0);
    }

    QUrl source() const;
    void setSource(const QUrl &);
    QString qmlData() const;
    void setQmlData(const QString &data);

    QString sourceString() const
    { return m_source.toString(); }
    void setSourceString(const QString &url)
    { setSource(QUrl(url)); }

    QWidget *widget() const;
    QDeclarativeComponent *component() const
    { return m_component; }

    void setBaseUrl(const QUrl &baseUrl);

    QUrl baseUrl() const;

signals:
    void widgetChanged();
    void sourceChanged();
    void qmlDataChanged();

private:
    QUrl m_source;
    QUrl m_baseUrl;
    QString m_qmlData;
    QWidget *m_widget;
    QDeclarativeComponent *m_component;
    QVBoxLayout *m_layout;
    QHash<QString, QWidget*> m_cachedWidgets;
};

QUrl WidgetLoader::source() const
{
    return m_source;
}

QUrl WidgetLoader::baseUrl() const
{
    return m_baseUrl;
}

void WidgetLoader::setBaseUrl(const QUrl &baseUrl)
{
    if (m_baseUrl == baseUrl)
        return;
    m_baseUrl = baseUrl;
}

QString WidgetLoader::qmlData() const
{
    return m_qmlData;
}

void WidgetLoader::setQmlData(const QString &data)
{
    if (m_qmlData == data)
        return;

    m_qmlData = data;

    setSource(QUrl());

    foreach (QWidget *cachedWidget, m_cachedWidgets)
        cachedWidget->hide();

    if (m_qmlData.isEmpty()) {
        emit sourceChanged();
        emit widgetChanged();
        return;
    }

    if (m_component) {
        delete m_component;
        m_component = 0;
    }

    QString stringHash = QString::number(qHash(data));

    if (m_cachedWidgets.contains(stringHash)) {
        m_widget = m_cachedWidgets.value(stringHash);
        m_widget->show();
    } else {
        m_component = new QDeclarativeComponent(qmlEngine(this), this);
        m_component->setData (m_qmlData.toLatin1(), m_baseUrl);
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

            QDeclarativeContext *ctxt = new QDeclarativeContext(qmlContext(this));
            ctxt->setContextObject(this);
            QObject *obj = m_component->create(ctxt);
            if (obj) {
                QWidget *widget = qobject_cast<QWidget *>(obj);
                if (widget) {
                    m_cachedWidgets.insert(stringHash, widget);
                    m_widget = widget;
                    m_layout->addWidget(m_widget);
                    m_widget->show();
                }
            }
        }
    }

}

void WidgetLoader::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    setQmlData("");

//    QWidget *oldWidget = m_widget;

    if (m_widget) {
        //m_widget->deleteLater();
        m_widget->hide();
        m_widget = 0;
    }

    foreach (QWidget *cachedWidget, m_cachedWidgets)
        cachedWidget->hide();

    m_source = source;
    if (m_source.isEmpty()) {
        emit sourceChanged();
        emit widgetChanged();
        return;
    }

    if (m_component) {
        delete m_component;
        m_component = 0;
    }

    if (m_cachedWidgets.contains(source.toString())) {
        m_widget = m_cachedWidgets.value(source.toString());
        m_widget->show();
    } else {
        m_component = new QDeclarativeComponent(qmlEngine(this), m_source, this);

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

            QDeclarativeContext *ctxt = new QDeclarativeContext(qmlContext(this));
            ctxt->setContextObject(this);
            QObject *obj = m_component->create(ctxt);
            if (obj) {
                QWidget *widget = qobject_cast<QWidget *>(obj);
                if (widget) {
                    m_cachedWidgets.insert(source.toString(), widget);
                    m_widget = widget;
                    m_layout->addWidget(m_widget);
                    m_widget->show();
                }
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

    Q_PROPERTY(bool animated READ isAnimated)

public:
    MyGroupBox(QWidget * parent = 0) : QGroupBox(parent), m_animated(false), m_firstExpand(true)
    {}

void setPixmap(const QPixmap &pixmap, qreal alpha = 1)
{ m_pixmap = pixmap; m_alpha = alpha;}

void setAnimated(bool animated)
{ m_animated = animated; }

void finishFirstExpand()
{ m_firstExpand = false; }

bool isAnimated()
{ return m_animated; }

public slots:
    virtual void setVisible ( bool visible );

protected:
    virtual void paintEvent(QPaintEvent * event);
private:
    qreal m_alpha;
    QPixmap m_pixmap;
    bool m_animated;
    bool m_firstExpand;
};

void MyGroupBox::paintEvent(QPaintEvent * event)
{
    QGroupBox::paintEvent(event);
    if (m_animated) {
        QPainter p(this);
        if (!m_pixmap.isNull() && !m_firstExpand) {
            p.setOpacity(m_alpha);
            p.drawPixmap(5, 5,  m_pixmap.width(), m_pixmap.height(), m_pixmap);
        }
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
    Q_PROPERTY(bool smooth READ isSmooth WRITE setSmooth)
public:
    QGroupBoxDeclarativeUI(QObject *parent = 0) : QObject(parent), m_expanded(true)
    {
        gb =  qobject_cast<MyGroupBox*>(parent);
        connect(&m_timeLine, SIGNAL (frameChanged(int)), this, SLOT(animate(int)));
        connect(&m_timeLine, SIGNAL(finished()), this, SLOT(finish()));

        m_timeLine.setDuration(100);
        m_timeLine.setFrameRange(0, 5);
        m_smooth = true;
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

    bool isSmooth()
    { return m_smooth; }
    void setSmooth(bool smooth)
    { m_smooth = smooth; }

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
    int m_oldMinHeight;
    QPixmap m_contens;
    bool m_smooth;

    void hideChildren();
    void showChildren();

    void reLayout();
};

void QGroupBoxDeclarativeUI::reLayout()
{
    gb->setUpdatesEnabled(false);
    QLayout *layout = gb->parentWidget()->layout();
    QPoint oldPos = gb->pos();
    if (layout) {
        layout->invalidate();
        layout->activate();
    }
    gb->move(oldPos);
    gb->setUpdatesEnabled(true);
    gb->update();
}

void QGroupBoxDeclarativeUI::finish()
{
    if (m_expanded) {
        showChildren();
        gb->setUpdatesEnabled(false);
        gb->setMinimumHeight(m_oldMinHeight);
        gb->setMaximumHeight(m_oldMAxHeight);
        gb->setUpdatesEnabled(true);
        //gb->resize(gb->sizeHint());
        gb->finishFirstExpand();
    }
    else {
        gb->setMinimumHeight(30);
        gb->setMaximumHeight(30);
        gb->resize(gb->sizeHint().width(), 30);
    }
    reLayout();
    gb->setAnimated(false);
}

void QGroupBoxDeclarativeUI::hideChildren()
{
    if (gb->isVisible()) {
        gb->setMinimumHeight(gb->height());
        foreach (QWidget *widget, gb->findChildren<QWidget*>())
            if (widget->parent() == gb) {
                widget->setProperty("wasVisibleGB", widget->property("visible"));
                widget->hide();
            }
    }
}

void QGroupBoxDeclarativeUI::showChildren()
{
    foreach (QWidget *widget, gb->findChildren<QWidget*>()) {
        if (widget->parent() == gb) {
            if (widget->property("wasVisibleGB").toBool())
                widget->show();
            widget->setProperty("wasVisibleGB", QVariant());
            widget->ensurePolished();
        }
    }
    gb->show();
}

void QGroupBoxDeclarativeUI::collapse()
{
    m_oldMAxHeight = gb->maximumHeight();
    m_oldHeight = gb->height();
    m_oldMinHeight =gb->minimumHeight();
    if (!m_expanded)
        return;

#if QT_VERSION < 0x050000
    m_contens = QPixmap::grabWidget(gb, 5, 5, gb->width() - 5, gb->height() - 5);
#else
    m_contens = gb->grab(QRect(5, 5, gb->width() - 5, gb->height() - 5));
#endif
    gb->setPixmap(m_contens,1);
    hideChildren();
    m_expanded = false;
    gb->setAnimated(true);
    m_timeLine.start();
}

void QGroupBoxDeclarativeUI::expand()
{
    if (m_expanded)
        return;
    m_expanded = true;
    gb->setAnimated(true);
    m_timeLine.start();
}

void QGroupBoxDeclarativeUI::animate(int frame)
{
    if (!m_smooth)
        return;
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
}

class QTabWidgetDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDeclarativeListProperty<QTabObject> tabs READ tabs)
    Q_CLASSINFO("DefaultProperty", "tabs")
public:
    QTabWidgetDeclarativeUI(QObject *other) : QObject(other) {}

    QDeclarativeListProperty<QTabObject> tabs() {
        return QDeclarativeListProperty<QTabObject>(this, 0, tabs_append, 0, 0, tabs_clear);
    }

private:
    static void tabs_append(QDeclarativeListProperty<QTabObject> *property, QTabObject *o) {
        QTabWidget *tw = static_cast<QTabWidget*>(property->object->parent());
        if (!o->icon().isNull())
            tw->addTab(o->content(), o->icon(), o->label());
        else
            tw->addTab(o->content(), o->label());
    }

    static void tabs_clear(QDeclarativeListProperty<QTabObject> *property) {
        QTabWidget *tw = static_cast<QTabWidget*>(property->object->parent());
        tw->clear();
    }
};

class ExpressionEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    ExpressionEdit(QWidget *parent = 0) : QPlainTextEdit(parent) {}
signals:
    void returnPressed();

protected:
    void keyPressEvent( QKeyEvent * e );
};

void ExpressionEdit::keyPressEvent(QKeyEvent * e)
{
    if ((e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) && (e->modifiers() == Qt::NoModifier))
        emit returnPressed();
    else
        QPlainTextEdit::keyPressEvent(e);
}


class WidgetFrame : public QFrame
{
    Q_OBJECT
public:
    WidgetFrame( QWidget * parent = 0, Qt::WindowFlags f = 0 ) : QFrame(parent, f)
    {}
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QTabObject)
QML_DECLARE_TYPE(MyGroupBox)
QML_DECLARE_TYPE(WidgetLoader)
QML_DECLARE_TYPE(WidgetFrame)
QML_DECLARE_TYPE(AnimatedToolButton)

void BasicWidgets::registerDeclarativeTypes()
{
    qmlRegisterExtendedType<QWidget,QWidgetDeclarativeUI>("Bauhaus",1,0,"QWidget");

    //display
    qmlRegisterType<QProgressBar>("Bauhaus",1,0,"QProgressBar");
    qmlRegisterType<QLCDNumber>("Bauhaus",1,0,"QLCDNumber");

    //input
    qmlRegisterType<QLineEdit>("Bauhaus",1,0,"QLineEdit");
    qmlRegisterType<QTextEdit>("Bauhaus",1,0,"QTextEdit");
    qmlRegisterType<ExpressionEdit>("Bauhaus",1,0,"ExpressionEdit");
    qmlRegisterType<QPlainTextEdit>("Bauhaus",1,0,"QPlainTextEdit");
    qmlRegisterType<QSpinBox>("Bauhaus",1,0,"QSpinBox");
    qmlRegisterType<QDoubleSpinBox>("Bauhaus",1,0,"QDoubleSpinBox");
    qmlRegisterType<QSlider>("Bauhaus",1,0,"QSlider");
    qmlRegisterType<QDateTimeEdit>("Bauhaus",1,0,"QDateTimeEdit");
    qmlRegisterType<QDateEdit>("Bauhaus",1,0,"QDateEdit");
    qmlRegisterType<QTimeEdit>("Bauhaus",1,0,"QTimeEdit");
    qmlRegisterType<QFontComboBox>("Bauhaus",1,0,"QFontComboBox");
    qmlRegisterType<QDial>("Bauhaus",1,0,"QDial");
    qmlRegisterType<QScrollBar>("Bauhaus",1,0,"QScrollBar");
    qmlRegisterType<QCalendarWidget>("Bauhaus",1,0,"QCalendarWidget");


    //qmlRegisterType<>("Bauhaus",1,0,QComboBox,QComboBox); //need a way to populate
    //QML_REGISTER_EXTENDED_TYPE(QComboBox,QComboBox, QComboBox); //need a way to populate

    //buttons
    //qmlRegisterType<>("Bauhaus",1,0,QPushButton,QPushButton);
    qmlRegisterType<QCheckBox>("Bauhaus",1,0,"QCheckBox");
    qmlRegisterType<QGroupBox>("Bauhaus",1,0,"QGroupBox");
    qmlRegisterType<Action>("Bauhaus",1,0,"QAction");
    qmlRegisterType<QRadioButton>("Bauhaus",1,0,"QRadioButton");
    qmlRegisterType<FileWidget>("Bauhaus",1,0,"FileWidget");
    qmlRegisterType<LayoutWidget>("Bauhaus",1,0,"LayoutWidget");

    //containers
    qmlRegisterType<QFrame>("Bauhaus",1,0,"QFrame");
    qmlRegisterType<WidgetFrame>("Bauhaus",1,0,"WidgetFrame");
    qmlRegisterType<WidgetLoader>("Bauhaus",1,0,"WidgetLoader");
    qmlRegisterType<AnimatedToolButton>("Bauhaus",1,0,"AnimatedToolButton");
    qmlRegisterExtendedType<MyGroupBox,QGroupBoxDeclarativeUI>("Bauhaus",1,0,"QExtGroupBox");
    qmlRegisterExtendedType<QTabWidget,QTabWidgetDeclarativeUI>("Bauhaus",1,0,"QTabWidget");
    qmlRegisterExtendedType<QScrollArea,QScrollAreaDeclarativeUI>("Bauhaus",1,0,"QScrollArea");
    qmlRegisterExtendedType<QPushButton,QPushButtonDeclarativeUI>("Bauhaus",1,0,"QPushButton");
    qmlRegisterExtendedType<ElidingLabel,QLabelDeclarativeUI>("Bauhaus",1,0,"QLabel");
    qmlRegisterExtendedType<QToolButton, QToolButtonDeclarativeUI>("Bauhaus",1,0,"QToolButton");
    qmlRegisterExtendedType<QComboBox, QComboBoxDeclarativeUI>("Bauhaus",1,0,"QComboBox");
    qmlRegisterExtendedType<QMenu, QMenuDeclarativeUI>("Bauhaus",1,0,"QMenu");
    //qmlRegisterType<>("Bauhaus",1,0,BauhausoolBox,BauhausoolBox);
    //qmlRegisterType<>("Bauhaus",1,0,QScrollArea,QScrollArea);

    //QML_REGISTER_EXTENDED_TYPE(BauhausColorButton,BauhausColorButton,BauhausColorButtonDeclarativeUI);

    //itemviews
    //qmlRegisterType<>("Bauhaus",1,0,QListView,QListView);
    //qmlRegisterType<>("Bauhaus",1,0,BauhausreeView,BauhausreeView);
    //qmlRegisterType<>("Bauhaus",1,0,BauhausableView,BauhausableView);

    qmlRegisterType<QTabObject>("Bauhaus",1,0,"QTabObject"); //### with namespacing, this should just be 'Tab'
}

#include "basicwidgets.moc"
