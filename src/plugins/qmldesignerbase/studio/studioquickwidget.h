// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <QObject>
#include <QQmlPropertyMap>
#include <QtQuickWidgets/QQuickWidget>

class QMLDESIGNERBASE_EXPORT StudioQmlTextBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    explicit StudioQmlTextBackend(QObject *parent = nullptr) : QObject(parent) {}

    void setText(const QString &text)
    {
        if (m_text == text)
            return;

        m_text = text;
        emit textChanged();
    }

    QString text() const { return m_text; }

    Q_INVOKABLE void activateText(const QString &text)
    {
        if (m_text == text)
            return;

        setText(text);
        emit activated(text);
    }

signals:
    void textChanged();
    void activated(const QString &text);

private:
    QString m_text;
};

class QMLDESIGNERBASE_EXPORT StudioQmlComboBoxBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentText READ currentText WRITE setCurrentText NOTIFY currentTextChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QStringList model READ model NOTIFY modelChanged) //TODO turn into model

public:
    explicit StudioQmlComboBoxBackend(QObject *parent = nullptr) : QObject(parent) {}

    void setModel(const QStringList &model)
    {
        if (m_model == model)
            return;
        m_model = model;
        emit countChanged();
        emit modelChanged();
        emit currentTextChanged();
        emit currentIndexChanged();
    }

    QStringList model() const { return m_model; }

    int count() const { return m_model.count(); }

    QString currentText() const
    {
        if (m_currentIndex < 0)
            return {};

        if (m_model.isEmpty())
            return {};

        if (m_currentIndex >= m_model.count())
            return {};

        return m_model.at(m_currentIndex);
    }

    int currentIndex() const { return m_currentIndex; }

    void setCurrentIndex(int i)
    {
        if (m_currentIndex == i)
            return;

        m_currentIndex = i;
        emit currentTextChanged();
        emit currentIndexChanged();
    }

    void setCurrentText(const QString &text)
    {
        if (currentText() == text)
            return;

        if (!m_model.contains(text))
            return;

        setCurrentIndex(m_model.indexOf(text));
    }

    Q_INVOKABLE void activateIndex(int i)
    {
        if (m_currentIndex == i)
            return;
        setCurrentIndex(i);
        emit activated(i);
    }

signals:
    void currentIndexChanged();
    void currentTextChanged();
    void countChanged();
    void modelChanged();
    void activated(int i);

private:
    int m_currentIndex = -1;
    QStringList m_model;
};

class QMLDESIGNERBASE_EXPORT StudioPropertyMap : public QQmlPropertyMap
{
public:
    struct PropertyPair
    {
        QString name;
        QVariant value;
    };

    explicit StudioPropertyMap(QObject *parent = 0);

    void setProperties(const QList<PropertyPair> &properties);
};

class QMLDESIGNERBASE_EXPORT StudioQuickWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StudioQuickWidget(QWidget *parent = nullptr);

    QQmlEngine *engine() const;
    QQmlContext *rootContext() const;

    QQuickItem *rootObject() const;

    void setResizeMode(QQuickWidget::ResizeMode mode);

    void setSource(const QUrl &url);

    void refresh();

    void setClearColor(const QColor &color);

    QList<QQmlError> errors() const;

    StudioPropertyMap *registerPropertyMap(const QByteArray &name);
    QQuickWidget *quickWidget() const;

signals:
    void adsFocusChanged();

private:
    QQuickWidget *m_quickWidget = nullptr;
};
