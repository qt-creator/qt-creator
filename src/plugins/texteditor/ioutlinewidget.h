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

#ifndef IOUTLINEWIDGET_H
#define IOUTLINEWIDGET_H

#include <texteditor/texteditor_global.h>
#include <QWidget>

namespace Core {
class IEditor;
}

namespace TextEditor {

class TEXTEDITOR_EXPORT IOutlineWidget : public QWidget
{
    Q_OBJECT
public:
    IOutlineWidget(QWidget *parent = 0) : QWidget(parent) {}

    virtual QList<QAction*> filterMenuActions() const = 0;
    virtual void setCursorSynchronization(bool syncWithCursor) = 0;

    virtual void restoreSettings(int position) { Q_UNUSED(position); }
    virtual void saveSettings(int position) { Q_UNUSED(position); }
};

class TEXTEDITOR_EXPORT IOutlineWidgetFactory : public QObject {
    Q_OBJECT
public:
    virtual bool supportsEditor(Core::IEditor *editor) const = 0;
    virtual IOutlineWidget *createWidget(Core::IEditor *editor) = 0;
};

} // namespace TextEditor

#endif // IOUTLINEWIDGET_H
