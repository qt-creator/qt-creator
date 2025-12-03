// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <actioninterface.h>
#include <modelnodecontextmenu_helper.h>

#include <QByteArray>

#include <QWidgetAction>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace QmlDesigner {

void setupQmlPreviewActions();

class ZoomAction;

class QmlPreviewAction : public ModelNodeAction
{
public:
    QmlPreviewAction();

    void updateContext() override;

    Type type() const override;
};

class ZoomPreviewAction : public AbstractAction
{
public:
    ZoomPreviewAction();
    ~ZoomPreviewAction() override;
    QByteArray category() const override;
    QByteArray menuId() const override;
    int priority() const override;
    Type type() const override;

    bool isVisible(const SelectionContext &selectionState) const override;
    bool isEnabled(const SelectionContext &selectionState) const override;
};

class FpsLabelAction : public QWidgetAction
{
public:
    explicit FpsLabelAction(QObject *parent = nullptr);
    static void fpsHandler(quint16 fpsValues[8]);
    static void cleanFpsCounter();
protected:
    QWidget *createWidget(QWidget *parent) override;
private:
    static void refreshFpsLabel(quint16 frames);
    static QPointer<QLabel> m_labelInstance;
    static QList<QPointer<QLabel>> fpsHandlerLabelList;
    static quint16 lastValidFrames;
};

class FpsAction : public ActionInterface
{
public:
    FpsAction();
    QAction *action() const override;
    QByteArray category() const override;
    QByteArray menuId() const override;
    int priority() const override;
    Type type() const override;
    void currentContextChanged(const SelectionContext &) override;
private:
    std::unique_ptr<FpsLabelAction> m_fpsLabelAction;
};

class SwitchLanguageComboboxAction : public QWidgetAction
{
    Q_OBJECT
public:
    explicit SwitchLanguageComboboxAction(QObject *parent = nullptr);
signals:
    void currentLocaleChanged(const QString& string);
protected:
    QWidget *createWidget(QWidget *parent) override;
private:
    QStringList m_previousLocales;
};

class SwitchLanguageAction : public ActionInterface
{
public:
    SwitchLanguageAction();
    QAction *action() const override;
    QByteArray category() const override;
    QByteArray menuId() const override;
    int priority() const override;
    Type type() const override;
    void currentContextChanged(const SelectionContext &) override;
private:
    std::unique_ptr<SwitchLanguageComboboxAction> m_switchLanguageAction;
};

} // namespace QmlDesigner
