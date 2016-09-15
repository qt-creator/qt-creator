/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
