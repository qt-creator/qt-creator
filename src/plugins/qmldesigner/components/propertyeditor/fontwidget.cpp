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

#include "fontwidget.h"
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QFontDialog>
#include <QApplication>
#include <QComboBox>
#include <QLabel>

QML_DEFINE_TYPE(Bauhaus,1,0,FontWidget,QmlDesigner::FontWidget);

namespace QmlDesigner {

FontWidget::FontWidget(QWidget *parent)
   : QWidget(parent),
   m_label(new QLabel(this)),
   m_fontButton(new QPushButton("ABC..", this))
{
    QHBoxLayout *horizonalBoxLayout = new QHBoxLayout(this);

    horizonalBoxLayout->addWidget(m_label);
    horizonalBoxLayout->addWidget(m_fontButton);
    m_fontButton->setMinimumHeight(20);
    m_fontButton->setCheckable(true);
    connect(m_fontButton, SIGNAL(toggled(bool)), SLOT(openFontEditor(bool)));
}

FontWidget::~FontWidget()
{
}

void FontWidget::openFontEditor(bool show)
{
    if (show && m_fontDialog.isNull()) {
        m_fontDialog = new QFontDialog();
        m_fontDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_fontDialog->setCurrentFont(m_font);
        QComboBox *comboBox = m_fontDialog->findChild<QComboBox*>();
        QList<QLabel*> labels = m_fontDialog->findChildren<QLabel*>();
        foreach (QLabel *label, labels)
            if (label->buddy() == comboBox)
                label->hide();
        comboBox->hide();
        m_fontDialog->show();
        connect(m_fontDialog.data(), SIGNAL(accepted()), SLOT(updateFont()));
        connect(m_fontDialog.data(), SIGNAL(destroyed(QObject*)), SLOT(resetFontButton()));
    } else {
        delete m_fontDialog.data();
    }
}

void FontWidget::updateFont()
{
    QFont font(m_fontDialog->currentFont());
    setFamily(font.family());
    setItalic(font.italic());
    setBold(font.bold());
    setFontSize(font.pointSizeF());
    emit dataFontChanged();
}

void FontWidget::resetFontButton()
{
    m_fontButton->setChecked(false);
}

QString FontWidget::text() const
{
    return m_label->text();
}

void FontWidget::setText(const QString &text)
{
    m_label->setText(text);
}

QString FontWidget::family() const
{
    return m_font.family();
}

void FontWidget::setFamily(const QString &fontFamily)
{
    if (fontFamily != m_font.family()) {
        m_font.setFamily(fontFamily);
        emit familyChanged();
    }
}

bool FontWidget::isBold() const
{
    return m_font.bold();
}

void FontWidget::setBold(bool isBold)
{
    if (m_font.bold() != isBold) {
        m_font.setBold(isBold);
        emit boldChanged();
    }
}

bool FontWidget::isItalic() const
{
    return m_font.italic();
}

void FontWidget::setItalic(bool isItalic)
{
    if (m_font.italic() != isItalic) {
        m_font.setItalic(isItalic);
        emit italicChanged();
    }
}


QFont FontWidget::font() const
{
    return m_font;
}

void FontWidget::setFont(QFont font)
{
    if (m_font != font) {
        m_font = font;
        emit dataFontChanged();
    }
}

qreal FontWidget::fontSize() const
{
    return m_font.pointSizeF();
}

void FontWidget::setFontSize(qreal size)
{
    if (m_font.pointSizeF() != size) {
        m_font.setPointSizeF(size);
        emit fontSizeChanged();
    }
}

} // namespace QmlDesigner
