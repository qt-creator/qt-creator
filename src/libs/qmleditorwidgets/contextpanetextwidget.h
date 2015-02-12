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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CONTEXTPANETEXTWIDGET_H
#define CONTEXTPANETEXTWIDGET_H

#include "qmleditorwidgets_global.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QVariant;
namespace Ui { class ContextPaneTextWidget; }
QT_END_NAMESPACE

namespace QmlJS { class PropertyReader; }

namespace QmlEditorWidgets {

class CustomColorDialog;

class QMLEDITORWIDGETS_EXPORT ContextPaneTextWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneTextWidget(QWidget *parent = 0);
    ~ContextPaneTextWidget();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setVerticalAlignmentVisible(bool);
    void setStyleVisible(bool);

public slots:
    void onTextColorButtonToggled(bool);
    void onColorButtonToggled(bool);
    void onColorDialogApplied(const QColor &color);
    void onColorDialogCancled();
    void onFontSizeChanged(int value);
    void onFontFormatChanged();
    void onBoldCheckedChanged(bool value);
    void onItalicCheckedChanged(bool value);
    void onUnderlineCheckedChanged(bool value);
    void onStrikeoutCheckedChanged(bool value);
    void onCurrentFontChanged(const QFont &font);
    void onHorizontalAlignmentChanged();
    void onVerticalAlignmentChanged();
    void onStyleComboBoxChanged(const QString &style);


signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

protected:
    void timerEvent(QTimerEvent *event);

private:
    Ui::ContextPaneTextWidget *ui;
    QString m_verticalAlignment;
    QString m_horizontalAlignment;
    int m_fontSizeTimer;
};

} //QmlDesigner

#endif // CONTEXTPANETEXTWIDGET_H
