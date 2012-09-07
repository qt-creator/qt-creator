/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef KITCONFIGWIDGET_H
#define KITCONFIGWIDGET_H

#include "projectexplorer_export.h"

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QGridLayout)

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// KitConfigWidget
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT KitConfigWidget : public QWidget
{
    Q_OBJECT

public:
    enum LayoutColumns {
        LabelColumn,
        WidgetColumn,
        ButtonColumn
    };

    KitConfigWidget(QWidget *parent = 0) : QWidget(parent)
    { }

    virtual QString displayName() const = 0;
    virtual void makeReadOnly() = 0;

    virtual void apply() = 0;
    virtual void discard() = 0;
    virtual bool isDirty() const = 0;

    virtual QWidget *buttonWidget() const { return 0; }

    virtual void addToLayout(QGridLayout *layout, int row);

signals:
    void dirty();

protected:
    void addLabel(QGridLayout *layout, int row);
    void addButtonWidget(QGridLayout *layout, int row);
};

} // namespace ProjectExplorer

#endif // KITCONFIGWIDGET_H
