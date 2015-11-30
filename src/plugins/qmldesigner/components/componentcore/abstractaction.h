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


#ifndef QMLDESIGNER_DEFAULTDESIGNERACTION_H
#define QMLDESIGNER_DEFAULTDESIGNERACTION_H

#include "actioninterface.h"

#include <QAction>
#include <QScopedPointer>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT DefaultAction : public QAction
{
    Q_OBJECT

public:
    DefaultAction(const QString &description);

signals:
    void triggered(bool checked, const SelectionContext &selectionContext);

public slots: //virtual function instead of slot
    virtual void actionTriggered(bool enable);
    void setSelectionContext(const SelectionContext &selectionContext);

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

#endif // QMLDESIGNER_DEFAULTDESIGNERACTION_H
