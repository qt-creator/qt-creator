/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "styledoutputpaneplaceholder.h"

#include <utils/fileutils.h>

#include <QChildEvent>
#include <QTabWidget>

namespace QmlDesigner {
namespace Internal {

StyledOutputpanePlaceHolder::StyledOutputpanePlaceHolder(Core::IMode *mode, QSplitter *parent)
    : Core::OutputPanePlaceHolder(mode, parent)
{
    QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/outputpane-style.css");
    sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
    m_customStylesheet = QString::fromUtf8(sheet);
}

void StyledOutputpanePlaceHolder::childEvent(QChildEvent *event)
{
    Core::OutputPanePlaceHolder::childEvent(event);

    if (event->type() == QEvent::ChildAdded) {
        QWidget *child = qobject_cast<QWidget*>(event->child());
        if (child) {
            QList<QTabWidget*> widgets = child->findChildren<QTabWidget*>();
            if (!widgets.isEmpty()) {
                widgets.first()->parentWidget()->ensurePolished();
                widgets.first()->parentWidget()->setStyleSheet(m_customStylesheet);
            }

        }
    } else if (event->type() == QEvent::ChildRemoved) {
        QWidget *child = qobject_cast<QWidget*>(event->child());
        if (child) {
            QList<QTabWidget*> widgets = child->findChildren<QTabWidget*>();
            if (!widgets.isEmpty())
                widgets.first()->parentWidget()->setStyleSheet(QString());
        }
    }
}

} // namespace Internal
} // namespace QmlDesigner
