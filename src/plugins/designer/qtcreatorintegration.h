// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QDesignerIntegration>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class QtCreatorIntegration : public QDesignerIntegration
{
    Q_OBJECT

public:
    explicit QtCreatorIntegration(QDesignerFormEditorInterface *core, QObject *parent = nullptr);
    ~QtCreatorIntegration();

    QWidget *containerWindow(QWidget *widget) const override;

    bool supportsToSlotNavigation() { return true; }

    void updateSelection() override;

signals:
    void creatorHelpRequested(const QUrl &url);

private:
    void slotNavigateToSlot(const QString &objectName, const QString &signalSignature, const QStringList &parameterNames);
    void slotDesignerHelpRequested(const QString &manual, const QString &document);
    void slotSyncSettingsToDesigner();

    bool navigateToSlot(const QString &objectName,
                        const QString &signalSignature,
                        const QStringList &parameterNames,
                        QString *errorMessage);
    void handleSymbolRenameStage1(QDesignerFormWindowInterface *formWindow, QObject *object,
                            const QString &newName, const QString &oldName);
    void handleSymbolRenameStage2(QDesignerFormWindowInterface *formWindow,
                            const QString &newName, const QString &oldName);

    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace Designer
