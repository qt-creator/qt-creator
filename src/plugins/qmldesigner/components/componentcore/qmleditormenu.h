// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QMenu>

class QStyleOptionMenuItem;
class QmlEditorMenuPrivate;

class QmlEditorMenu : public QMenu
{
    Q_OBJECT

    Q_PROPERTY(bool qmlEditorMenu READ qmlEditorMenu CONSTANT)
    Q_PROPERTY(bool iconsVisible READ iconsVisible WRITE setIconsVisible CONSTANT)

public:
    explicit QmlEditorMenu(QWidget *parent = nullptr);
    explicit QmlEditorMenu(const QString &title, QWidget *parent = nullptr);
    virtual ~QmlEditorMenu();

    static bool isValid(const QMenu *menu);

    bool iconsVisible() const;
    void setIconsVisible(bool visible);

protected:
    virtual void initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const override;

private:
    bool qmlEditorMenu() const;

    QmlEditorMenuPrivate *d = nullptr;
};
