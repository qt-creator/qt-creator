// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QUrl>
#include <QtQml/qqml.h>

class PartialScene : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl container READ container WRITE setContainer NOTIFY containerChanged)
    Q_PROPERTY(QString itemType READ itemType WRITE setItemType NOTIFY itemTypeChanged)
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 0)
public:
    PartialScene(QObject *parent = nullptr)
        : QObject(parent)
    {}

    const QUrl container() const { return m_container; }
    const QString itemType() const { return m_itemType; }

    void setContainer(const QUrl &a)
    {
        if (a == m_container)
            return;
        m_container = a;
        emit containerChanged();
    }
    void setItemType(const QString &a)
    {
        if (a == m_itemType)
            return;
        m_itemType = a;
        emit itemTypeChanged();
    }

signals:
    void containerChanged();
    void itemTypeChanged();

private:
    QUrl m_container;
    QString m_itemType;
};

class Config : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<PartialScene> sceneCompleters READ sceneCompleters)
    Q_CLASSINFO("DefaultProperty", "sceneCompleters")
    QML_NAMED_ELEMENT(Configuration)
    QML_ADDED_IN_VERSION(1, 0)
public:
    Config(QObject *parent = nullptr)
        : QObject(parent)
    {}

    QQmlListProperty<PartialScene> sceneCompleters()
    {
        return QQmlListProperty<PartialScene>(this, &completers);
    }

    QList<PartialScene *> completers;
};
