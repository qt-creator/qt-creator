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

#ifndef FONTWIDGET_H
#define FONTWIDGET_H

#include <QWeakPointer>
#include <QWidget>
#include <qml.h>


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

QT_BEGIN_NAMESPACE
QML_DECLARE_TYPE(QmlDesigner::FontWidget);
QT_END_NAMESPACE

#endif // FONTWIDGET_H
