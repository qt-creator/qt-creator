/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <actioninterface.h>
#include <modelnodecontextmenu_helper.h>

#include <QByteArray>

#include <QWidgetAction>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QLabel)


namespace QmlPreview {
using QmlPreviewFpsHandler = void (*)(quint16 *);
}

namespace ProjectExplorer {
class Project;
}

namespace QmlDesigner {
class ZoomAction;

class QmlPreviewAction : public ModelNodeAction
{
public:
    QmlPreviewAction();

    void updateContext() override;

    Type type() const override;
};

class ZoomPreviewAction : public ActionInterface
{
public:
    ZoomPreviewAction();
    ~ZoomPreviewAction() override;
    QAction *action() const override;
    QByteArray category() const override;
    QByteArray menuId() const override;
    int priority() const override;
    Type type() const override;
    void currentContextChanged(const SelectionContext &) override;

private:
    std::unique_ptr<ZoomAction> m_zoomAction;
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
    bool updateProjectLocales(ProjectExplorer::Project *project);
    QStringList m_localeStrings;
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
Q_DECLARE_METATYPE(QmlPreview::QmlPreviewFpsHandler);
