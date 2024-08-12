// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QDesignerIntegration>

QT_BEGIN_NAMESPACE
class QUrl;
class QVersionNumber;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

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

    bool setQtVersionFromFile(const Utils::FilePath &filePath);
    void resetQtVersion();

signals:
    void creatorHelpRequested(const QUrl &url);

private:
    void slotActiveFormWindowChanged(QDesignerFormWindowInterface *formWindow);
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

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
    void setQtVersion(const QVersionNumber &version);
#endif

    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace Designer
