// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

#include <QCoreApplication>

namespace TextEditor {
    class TextEditorWidget;
}

namespace Android {
namespace Internal {

class AndroidManifestEditorIconWidget;

class AndroidManifestEditorIconContainerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AndroidManifestEditorIconContainerWidget(QWidget *parent,
                                                      TextEditor::TextEditorWidget *textEditorWidget);
    void setIconFileName(const QString &name);
    QString iconFileName() const;
    void loadIcons();
    bool hasIcons() const;
private:
    QVector<AndroidManifestEditorIconWidget *> m_iconButtons;
    QString m_iconFileName = QLatin1String("icon");
    bool m_hasIcons = false;
signals:
    void iconsModified();
};

} // namespace Internal
} // namespace Android
