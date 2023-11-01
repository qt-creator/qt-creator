// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLabel>
#include <QPixmap>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils {
namespace Internal {

class TipLabel : public QLabel
{
public:
    TipLabel(QWidget *parent);

    virtual void setContent(const QVariant &content) = 0;
    virtual bool isInteractive() const { return false; }
    virtual int showTime() const = 0;
    virtual void configure(const QPoint &pos) = 0;
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
    void configure(const QPoint &pos) override;
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
    void configure(const QPoint &pos) override;
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
    void configure(const QPoint &pos) override;
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
