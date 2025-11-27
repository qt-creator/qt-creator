// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qqmlintegration.h>
#include <QObject>

#include <projectexplorer/buildstep.h>

class McuOutputModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool mcu READ mcu NOTIFY mcuChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

    enum { MessageRole = Qt::DisplayRole, ColorRole = Qt::UserRole };

signals:
    void mcuChanged(bool mcu);
    void textChanged(const QString &text);

public:
    McuOutputModel(QObject *parent = nullptr);

    bool mcu() const;
    QString text() const;

    void setMcu(bool mcu);
    void setText(const QString &text);

    Q_INVOKABLE void clear();

    void setup(ProjectExplorer::BuildConfiguration *bc);
    void addOutput(const QString &str, ProjectExplorer::BuildStep::OutputFormat fmt);

private:
    QString m_text;
    bool m_mcu = false;
};
