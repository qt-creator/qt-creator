/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
