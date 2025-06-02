// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fileresourcesmodel.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

class FontResourcesModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend
                   NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QStringList model READ model NOTIFY modelChanged)

public:
    static void registerDeclarativeType();
    [[nodiscard]] static QVariant modelNodeBackend();

    explicit FontResourcesModel(QObject *parent = nullptr);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    [[nodiscard]] QStringList model() const;

    void refreshModel();

    Q_INVOKABLE void openFileDialog(const QString &customPath = {});
    [[nodiscard]] Q_INVOKABLE QString resolve(const QString &relative) const;
    [[nodiscard]] Q_INVOKABLE bool isLocal(const QString &path) const;

signals:
    void fileNameChanged(const QUrl &fileName);
    void filterChanged(const QString &filter);
    void modelNodeBackendChanged();
    void pathChanged(const QUrl &path);
    void modelChanged();

private:
    std::unique_ptr<FileResourcesModel> m_resourceModel;
};

QML_DECLARE_TYPE(FontResourcesModel)
