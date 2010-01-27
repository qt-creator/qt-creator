/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BASICWIDGETS_H
#define BASICWIDGETS_H

#include <qml.h>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QDateEdit>
#include <QTimeEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QDial>
#include <QLCDNumber>
#include <QFontComboBox>
#include <QScrollBar>
#include <QCalendarWidget>
#include <QTabWidget>
#include <QAction>
#include "colorwidget.h"
#include "filewidget.h"
#include "layoutwidget.h"


QT_BEGIN_NAMESPACE

QML_DECLARE_TYPE(QWidget);

//display
QML_DECLARE_TYPE(QLabel);
QML_DECLARE_TYPE(QProgressBar);
QML_DECLARE_TYPE(QLCDNumber);

//input
QML_DECLARE_TYPE(QLineEdit);
QML_DECLARE_TYPE(QTextEdit);
QML_DECLARE_TYPE(QPlainTextEdit);
QML_DECLARE_TYPE(QSpinBox);
QML_DECLARE_TYPE(QDoubleSpinBox);
QML_DECLARE_TYPE(QSlider);
QML_DECLARE_TYPE(QDateTimeEdit);
QML_DECLARE_TYPE(QDateEdit);
QML_DECLARE_TYPE(QTimeEdit);
QML_DECLARE_TYPE(QFontComboBox);
QML_DECLARE_TYPE(QDial);
QML_DECLARE_TYPE(QScrollBar);
QML_DECLARE_TYPE(QCalendarWidget);
QML_DECLARE_TYPE(QComboBox);

//buttons
QML_DECLARE_TYPE(QPushButton);
QML_DECLARE_TYPE(QToolButton);
QML_DECLARE_TYPE(QCheckBox);
QML_DECLARE_TYPE(QRadioButton);

//containers
QML_DECLARE_TYPE(QGroupBox);
QML_DECLARE_TYPE(QFrame);
QML_DECLARE_TYPE(QScrollArea);
QML_DECLARE_TYPE(QTabWidget);
QML_DECLARE_TYPE(FileWidget);
QML_DECLARE_TYPE(LayoutWidget);

class Action : public QAction {
    Q_OBJECT
public:
    Action(QObject *parent = 0) : QAction(parent) {}
};

QML_DECLARE_TYPE(QMenu);
QML_DECLARE_TYPE(Action);

//QML_DECLARE_TYPE(QToolBox);

//itemviews
//QML_DECLARE_TYPE(QListView);
//QML_DECLARE_TYPE(QTreeView);
//QML_DECLARE_TYPE(QTableView);

//top-level windows?


QT_END_NAMESPACE
#endif // BASICWIDGETS_H
