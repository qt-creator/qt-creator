// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QVariantMap>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QToolButton)

namespace ScxmlEditor {

namespace PluginInterface { class ScxmlDocument; }

namespace Common {

class ColorThemes : public QObject
{
    Q_OBJECT

public:
    ColorThemes(QObject *parent = nullptr);
    ~ColorThemes() override;

    QAction *modifyThemeAction() const;
    QToolButton *themeToolButton() const;
    QMenu *themeMenu() const;

    void showDialog();
    void setDocument(PluginInterface::ScxmlDocument *doc);

private:
    void updateColorThemeMenu();
    void selectColorTheme(const QString &name);
    void setCurrentColors(const QVariantMap &colorData);

    QString m_currentTheme;
    QAction *m_modifyAction = nullptr;
    QToolButton *m_toolButton = nullptr;
    QMenu *m_menu = nullptr;
    QPointer<PluginInterface::ScxmlDocument> m_document;
    QVariantMap m_documentColors;
};

} // namespace Common
} // namespace ScxmlEditor
