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

#ifndef WIDGETHOST_H
#define WIDGETHOST_H

#include <QScrollArea>

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
    explicit WidgetHost(QWidget *parent = 0, QDesignerFormWindowInterface *formWindow = 0);
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
