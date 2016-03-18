/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <texteditor/texteditor_global.h>
#include <QWidget>
#include <QVariantMap>

namespace Core { class IEditor; }

namespace TextEditor {

class TEXTEDITOR_EXPORT IOutlineWidget : public QWidget
{
    Q_OBJECT
public:
    IOutlineWidget(QWidget *parent = 0) : QWidget(parent) {}

    virtual QList<QAction*> filterMenuActions() const = 0;
    virtual void setCursorSynchronization(bool syncWithCursor) = 0;

    virtual void restoreSettings(const QVariantMap & /*map*/) { }
    virtual QVariantMap settings() const { return QVariantMap(); }
};

class TEXTEDITOR_EXPORT IOutlineWidgetFactory : public QObject {
    Q_OBJECT
public:
    virtual bool supportsEditor(Core::IEditor *editor) const = 0;
    virtual IOutlineWidget *createWidget(Core::IEditor *editor) = 0;
};

} // namespace TextEditor
