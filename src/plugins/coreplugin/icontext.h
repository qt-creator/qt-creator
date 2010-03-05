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

#ifndef ICONTEXT_H
#define ICONTEXT_H

#include <coreplugin/core_global.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IContext : public QObject
{
    Q_OBJECT
public:
    IContext(QObject *parent = 0) : QObject(parent) {}
    virtual ~IContext() {}

    virtual QList<int> context() const = 0;
    virtual QWidget *widget() = 0;
    virtual QString contextHelpId() const { return QString(); }
};

class BaseContext : public Core::IContext
{
public:
    BaseContext(QWidget *widget, const QList<int> &context, QObject *parent = 0)
        : Core::IContext(parent),
        m_widget(widget),
        m_context(context)
    {
    }

    QList<int> context() const { return m_context; }

    QWidget *widget() { return m_widget; }

private:
    QWidget *m_widget;
    QList<int> m_context;
};

} // namespace Core

#endif //ICONTEXT_H
