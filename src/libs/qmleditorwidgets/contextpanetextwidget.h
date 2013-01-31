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

#ifndef CONTEXTPANETEXTWIDGET_H
#define CONTEXTPANETEXTWIDGET_H

#include "qmleditorwidgets_global.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QVariant;
namespace Ui {
    class ContextPaneTextWidget;
}
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

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
    void changeEvent(QEvent *e);
    void timerEvent(QTimerEvent *event);

private:
    Ui::ContextPaneTextWidget *ui;
    QString m_verticalAlignment;
    QString m_horizontalAlignment;
    int m_fontSizeTimer;
};

} //QmlDesigner

#endif // CONTEXTPANETEXTWIDGET_H
