/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CUSTOMWIDGET
#define CUSTOMWIDGET

#include <QDesignerCustomWidgetInterface>

#include <QtCore/QString>
#include <QtCore/QSize>
#include <QtCore/QTextStream>
#include <QtGui/QIcon>

// Parametrizable Template for custom widgets.

template <class Widget>
class CustomWidget : public QDesignerCustomWidgetInterface
{

public:
    explicit CustomWidget(const QString &includeFile,
                          bool isContainer,
                          const QString &group,
                          const QIcon &icon,
                          const QString &toolTip,
                          const QSize &size = QSize());

    bool isContainer() const        { return m_isContainer; }
    bool isInitialized() const      { return m_initialized; }
    QIcon icon() const              { return m_icon; }
    QString domXml() const;
    QString group() const           { return m_group; }
    QString includeFile() const     { return m_includeFile; }
    QString name() const;
    QString toolTip() const         { return m_toolTip; }
    QString whatsThis() const       { return m_toolTip; }
    QWidget *createWidget(QWidget *parent);
    void initialize(QDesignerFormEditorInterface *core);

private:
    QString displayName() const;
    QString geometryProperty() const;

    const QString m_includeFile;
    const bool m_isContainer;
    const QString m_group;
    const QIcon m_icon;
    const QString m_toolTip;
    const QSize m_size;

    bool m_initialized;
};


template <class Widget>
    CustomWidget<Widget>::CustomWidget(const QString &includeFile,
                                       bool isContainer,
                                       const QString &group,
                                       const QIcon &icon,
                                       const QString &toolTip,
                                       const QSize &size) :
    m_includeFile(includeFile),
    m_isContainer(isContainer),
    m_group(group),
    m_icon(icon),
    m_toolTip(toolTip),
    m_size(size),
    m_initialized(false)
{
}

// Determine the name to be displayed in the widget box: Strip
// namespaces.
template <class Widget>
    QString CustomWidget<Widget>::displayName() const
{
    QString rc = name();
    const int index = rc.lastIndexOf(QLatin1Char(':'));
    if (index != -1)
        rc.remove(0, index + 1);
    return rc;
}

template <class Widget>
    QString CustomWidget<Widget>::geometryProperty() const
{
    QString rc;
    if (m_size.isEmpty())
        return rc;
    QTextStream(&rc) << "<property name=\"geometry\"><rect><x>0</x><y>0</y><width>"
        << m_size.width() << "</width><height>" <<  m_size.height()
            << "</height></rect></property>";
    return rc;
}

template <class Widget>
    QString CustomWidget<Widget>::domXml() const
{
    const QString className = name();
    QString rc;
    // Name: 'QClass' -> 'class'
    QString name = className;
    if (name.startsWith(QLatin1Char('Q')))
        name.remove(0, 1);
    name[0] = name.at(0).toLower();
    // Format
    QTextStream str(&rc);
    str << "<ui displayname=\"" << displayName()
        << "\" language=\"c++\"><widget class=\"" << className << "\" name=\""
        << name  << "\">" << geometryProperty()  << "</widget></ui>";
    return rc;
}

template <class Widget>
    QString CustomWidget<Widget>::name() const
{
    return QLatin1String(Widget::staticMetaObject.className());
}

template <class Widget>
    QWidget *CustomWidget<Widget>::createWidget(QWidget *parent)
{
    return new Widget(parent);
}

template <class Widget>
    void CustomWidget<Widget>::initialize(QDesignerFormEditorInterface *)
{
    if (!m_initialized) {
        m_initialized = true;
    }
}

#endif // CUSTOMWIDGET
