// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QMenu>

QT_BEGIN_NAMESPACE
class QStyleOptionMenuItem;
QT_END_NAMESPACE

namespace QmlDesigner {

class QmlEditorMenuPrivate;
class DesignerIcons;

class QmlEditorMenu : public QMenu
{
    Q_OBJECT

    Q_PROPERTY(bool qmlEditorMenu READ qmlEditorMenu CONSTANT)
    Q_PROPERTY(bool iconsVisible READ iconsVisible WRITE setIconsVisible NOTIFY iconVisibilityChanged)

public:
    explicit QmlEditorMenu(QWidget *parent = nullptr);
    explicit QmlEditorMenu(const QString &title, QWidget *parent = nullptr);
    virtual ~QmlEditorMenu();

    static bool isValid(const QMenu *menu);

    bool iconsVisible() const;
    void setIconsVisible(bool visible);

signals:
    void iconVisibilityChanged(bool);

protected:
    virtual void initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const override;

private:
    bool qmlEditorMenu() const;

    QmlEditorMenuPrivate *d = nullptr;
};

class QmlEditorStyleObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QIcon cascadeIconLeft READ cascadeIconLeft CONSTANT)
    Q_PROPERTY(QIcon cascadeIconRight READ cascadeIconRight CONSTANT)
    Q_PROPERTY(QIcon backspaceIcon READ backspaceIcon CONSTANT)
    Q_PROPERTY(QIcon tickIcon READ tickIcon CONSTANT)

public:
    static QmlEditorStyleObject *instance();

    QIcon cascadeIconLeft() const;
    QIcon cascadeIconRight() const;
    QIcon tickIcon() const;
    QIcon backspaceIcon() const;

private:
    QmlEditorStyleObject();
};

} // namespace QmlDesigner
