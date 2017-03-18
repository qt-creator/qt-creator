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

#ifndef StackedUtilityPanelController_h
#define StackedUtilityPanelController_h

#include "utilitypanelcontroller.h"

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class DesignDocument;

class StackedUtilityPanelController : public UtilityPanelController
{
    Q_OBJECT

public:
    StackedUtilityPanelController(QObject* parent = 0);

    void show(DesignDocument* DesignDocument);
    void close(DesignDocument* DesignDocument);

protected:
    virtual QWidget* contentWidget() const;
    virtual QWidget* stackedPageWidget(DesignDocument* DesignDocument) const = 0;

private:
    class QStackedWidget* m_stackedWidget;
};

} // namespace QmlDesigner

#endif // StackedUtilityPanelController_h
