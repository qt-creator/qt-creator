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

#ifndef DEBUGVIEWWIDGET_H
#define DEBUGVIEWWIDGET_H

#include <QWidget>

#include "ui_debugviewwidget.h"

namespace QmlDesigner {

namespace Internal {

class DebugViewWidget : public QWidget
{
    Q_OBJECT
public:
    DebugViewWidget(QWidget *parent = 0);

    void addLogMessage(const QString &topic, const QString &message, bool highlight = false);
    void addErrorMessage(const QString &topic, const QString &message);
    void addLogInstanceMessage(const QString &topic, const QString &message, bool highlight = false);

    void setDebugViewEnabled(bool b);

public slots:
    void enabledCheckBoxToggled(bool b);

private:
    Ui::DebugViewWidget m_ui;
};

} //namespace Internal

} //namespace QmlDesigner

#endif //DEBUGVIEWWIDGET_H

