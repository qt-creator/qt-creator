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

#ifndef FONTWIDGET_H
#define FONTWIDGET_H

#include <QWeakPointer>
#include <QWidget>
#include <qdeclarative.h>


QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QFontDialog;
QT_END_NAMESPACE

namespace QmlDesigner {

class FontWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString family READ family WRITE setFamily NOTIFY familyChanged)
    Q_PROPERTY(bool bold READ isBold WRITE setBold NOTIFY boldChanged)
    Q_PROPERTY(bool italic READ isItalic WRITE setItalic NOTIFY italicChanged)
    Q_PROPERTY(qreal fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QFont dataFont READ font WRITE setFont NOTIFY dataFontChanged)

public:
    FontWidget(QWidget *parent = 0);
    ~FontWidget();

    QString text() const;
    void setText(const QString &text);

    QString family() const;
    void setFamily(const QString &fontFamily);

    bool isBold() const;
    void setBold(bool isBold);

    bool isItalic() const;
    void setItalic(bool isItalic);

    qreal fontSize() const;
    void setFontSize(qreal size);

    QFont font() const;
    void setFont(QFont size);

    static void registerDeclarativeTypes();

signals:
    void familyChanged();
    void boldChanged();
    void italicChanged();
    void fontSizeChanged();
    void dataFontChanged();

private slots:
    void openFontEditor(bool show);
    void updateFont();
    void resetFontButton();

private: //variables
    QFont m_font;
    QLabel *m_label;
    QPushButton *m_fontButton;
    QWeakPointer<QFontDialog> m_fontDialog;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::FontWidget)

#endif // FONTWIDGET_H
