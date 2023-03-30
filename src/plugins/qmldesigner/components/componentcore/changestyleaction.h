// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <actioninterface.h>

#include <componentcore_constants.h>

#include <QWidgetAction>

#include <functional>
#include <memory>

namespace QmlDesigner {

class AbstractView;

struct StyleWidgetEntry {
    QString displayName;

    QString styleName;
    QString styleTheme;

    bool operator==(const StyleWidgetEntry &entry) const {
        if (displayName != entry.displayName)
            return false;
        if (styleName != entry.styleName)
            return false;
        if (styleTheme != entry.styleTheme)
            return false;

        return true;
    };
};

class ChangeStyleWidgetAction : public QWidgetAction
{
    Q_OBJECT

public:
    explicit ChangeStyleWidgetAction(QObject *parent = nullptr);
    void handleModelUpdate(const QString &style);

    const QList<StyleWidgetEntry> styleItems() const;

    static QList<StyleWidgetEntry> getAllStyleItems();

    static void changeCurrentStyle(const QString &style, const QString &qmlFileName);

    static int getCurrentStyle(const QString &fileName);

public slots:
    void handleStyleChanged(const QString &style);

protected:
    QWidget *createWidget(QWidget *parent) override;

signals:
    void modelUpdated(const QString &style);

public:
    QString qmlFileName;
    QPointer<AbstractView> view;

    QList<StyleWidgetEntry> items;
};

class ChangeStyleAction : public ActionInterface
{
public:
    ChangeStyleAction() :
        m_action(new ChangeStyleWidgetAction())
    {}

    QAction *action() const override { return m_action.get(); }
    QByteArray category() const override  { return ComponentCoreConstants::genericToolBarCategory; }
    QByteArray menuId() const override  { return QByteArray(); }
    int priority() const override { return 50; }
    Type type() const override { return ToolBarAction; }
    void currentContextChanged(const SelectionContext &) override;

private:
    std::unique_ptr<ChangeStyleWidgetAction> m_action;
};


} // namespace QmlDesigner
