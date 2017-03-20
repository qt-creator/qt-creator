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

#include "actioninterface.h"

#include <QAction>
#include <QScopedPointer>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT DefaultAction : public QAction
{
    Q_OBJECT

public:
    DefaultAction(const QString &description);

    // virtual function instead of slot
    virtual void actionTriggered(bool enable);
    void setSelectionContext(const SelectionContext &selectionContext);

signals:
    void triggered(bool checked, const SelectionContext &selectionContext);

protected:
    SelectionContext m_selectionContext;
};

class QMLDESIGNERCORE_EXPORT AbstractAction : public ActionInterface
{
public:
    AbstractAction(const QString &description = QString());
    AbstractAction(DefaultAction *action);

    QAction *action() const;
    DefaultAction *defaultAction() const;

    void currentContextChanged(const SelectionContext &selectionContext);

protected:
    virtual void updateContext();
    virtual bool isVisible(const SelectionContext &selectionContext) const = 0;
    virtual bool isEnabled(const SelectionContext &selectionContext) const = 0;
    SelectionContext selectionContext() const;

private:
    QScopedPointer<DefaultAction> m_defaultAction;
    SelectionContext m_selectionContext;
};

} // namespace QmlDesigner
