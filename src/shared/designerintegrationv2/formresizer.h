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
#ifndef FORMRESIZER_H
#define FORMRESIZER_H

#include "namespace_global.h"

#include "widgethostconstants.h"

#include <QtGui/QWidget>
#include <QtCore/QVector>

QT_FORWARD_DECLARE_CLASS(QDesignerFormWindowInterface)
QT_FORWARD_DECLARE_CLASS(QFrame)

namespace SharedTools {
namespace Internal {

class SizeHandleRect;

/* A window to embed a form window interface as follows:
 *
 *            Widget
 *              |
 *          +---+----+
 *          |        |
 *          |        |
 *       Handles   QVBoxLayout [margin: SELECTION_MARGIN]
 *                   | 
 *                 Frame [margin: lineWidth]
 *                   |
 *                 QVBoxLayout
 *                   |
 *                 QDesignerFormWindowInterface
 *
 * Can be embedded into a QScrollArea. */

class FormResizer : public QWidget
{
    Q_OBJECT
public:

    FormResizer(QWidget *parent = 0);

    void updateGeometry();
    void setState(SelectionHandleState st);
    void update();

    void setFormWindow(QDesignerFormWindowInterface *fw);

signals:
    void formWindowSizeChanged(const QRect &oldGeo, const QRect &newGeo);

protected:
    virtual void resizeEvent(QResizeEvent *event);

private slots:
    void mainContainerChanged();

private:
    QSize decorationSize() const;
    QWidget *mainContainer();

    QFrame *m_frame;
    typedef QVector<SizeHandleRect*> Handles;
    Handles m_handles;
    QDesignerFormWindowInterface * m_formWindow;
};

}
} // namespace SharedTools

#endif // FORMRESIZER_H
