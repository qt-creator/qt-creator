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

#ifndef WIDGETHOST_H
#define WIDGETHOST_H

#include "namespace_global.h"

#include <QtGui/QScrollArea>

QT_FORWARD_DECLARE_CLASS(QDesignerFormWindowInterface)

namespace SharedTools {

namespace Internal {
    class FormResizer;
}

/* A scroll area that embeds a Designer form window */

class WidgetHost : public QScrollArea
{
    Q_OBJECT
public:
    WidgetHost(QWidget *parent = 0, QDesignerFormWindowInterface *formWindow = 0);
    virtual ~WidgetHost();
    // Show handles if active and main container is selected.
    void updateFormWindowSelectionHandles(bool active);

    inline QDesignerFormWindowInterface *formWindow() const { return m_formWindow; }

    QWidget *integrationContainer() const;

protected:
    void setFormWindow(QDesignerFormWindowInterface *fw);

signals:
    void formWindowSizeChanged(int, int);

private slots:
    void fwSizeWasChanged(const QRect &, const QRect &);

private:
    QSize formWindowSize() const;

    QDesignerFormWindowInterface *m_formWindow;
    Internal::FormResizer *m_formResizer;
    QSize m_oldFakeWidgetSize;
};

} // namespace SharedTools

#endif // WIDGETHOST_H
