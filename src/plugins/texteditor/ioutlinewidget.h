// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>
#include <QWidget>
#include <QVariantMap>

namespace Core { class IEditor; }

namespace TextEditor {

class TEXTEDITOR_EXPORT IOutlineWidget : public QWidget
{
    Q_OBJECT
public:
    IOutlineWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    virtual QList<QAction*> filterMenuActions() const = 0;
    virtual void setCursorSynchronization(bool syncWithCursor) = 0;
    virtual void setSorted(bool /*sorted*/) {}
    virtual bool isSorted() const { return false; }

    virtual void restoreSettings(const QVariantMap & /*map*/) { }
    virtual QVariantMap settings() const { return QVariantMap(); }
};

class TEXTEDITOR_EXPORT IOutlineWidgetFactory : public QObject
{
    Q_OBJECT

public:
    IOutlineWidgetFactory();
    ~IOutlineWidgetFactory() override;

    virtual bool supportsEditor(Core::IEditor *editor) const = 0;
    virtual bool supportsSorting() const { return false; }
    virtual IOutlineWidget *createWidget(Core::IEditor *editor) = 0;

    static void updateOutline();
};

} // namespace TextEditor
