/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "../utils_global.h"

#include <QLabel>
#include <QPixmap>
#include <QSharedPointer>
#include <QVariant>
#include <QVBoxLayout>

namespace Utils {
namespace Internal {

int screenNumber(const QPoint &pos, QWidget *w);
QRect screenGeometry(const QPoint &pos, QWidget *w);

class TipLabel : public QLabel
{
public:
    TipLabel(QWidget *parent);

    virtual void setContent(const QVariant &content) = 0;
    virtual bool isInteractive() const { return false; }
    virtual int showTime() const = 0;
    virtual void configure(const QPoint &pos, QWidget *w) = 0;
    virtual bool canHandleContentReplacement(int typeId) const = 0;
    virtual bool equals(int typeId, const QVariant &other, const QVariant &contextHelp) const = 0;
    virtual void setContextHelp(const QVariant &help);
    virtual QVariant contextHelp() const;

protected:
    const QMetaObject *metaObject() const override;

private:
    QVariant m_contextHelp;
};

using TextItem = std::pair<QString, Qt::TextFormat>;

class TextTip : public TipLabel
{
public:
    TextTip(QWidget *parent);

    void setContent(const QVariant &content) override;
    bool isInteractive() const override;
    void configure(const QPoint &pos, QWidget *w) override;
    bool canHandleContentReplacement(int typeId) const override;
    int showTime() const override;
    bool equals(int typeId, const QVariant &other, const QVariant &otherContextHelp) const override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QString m_text;
    Qt::TextFormat m_format = Qt::AutoText;
};

class ColorTip : public TipLabel
{
public:
    ColorTip(QWidget *parent);

    void setContent(const QVariant &content) override;
    void configure(const QPoint &pos, QWidget *w) override;
    bool canHandleContentReplacement(int typeId) const override;
    int showTime() const override { return 4000; }
    bool equals(int typeId, const QVariant &other, const QVariant &otherContextHelp) const override;
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color;
    QPixmap m_tilePixmap;
};

class WidgetTip : public TipLabel
{
    Q_OBJECT

public:
    explicit WidgetTip(QWidget *parent = nullptr);
    void pinToolTipWidget(QWidget *parent);

    void setContent(const QVariant &content) override;
    void configure(const QPoint &pos, QWidget *w) override;
    bool canHandleContentReplacement(int typeId) const override;
    int showTime() const override { return 30000; }
    bool equals(int typeId, const QVariant &other, const QVariant &otherContextHelp) const override;
    bool isInteractive() const override { return true; }

private:
    QWidget *m_widget;
    QVBoxLayout *m_layout;
};

} // namespace Internal
} // namespace Utils

Q_DECLARE_METATYPE(Utils::Internal::TextItem)
