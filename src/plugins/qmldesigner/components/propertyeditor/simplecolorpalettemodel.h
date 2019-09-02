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

#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QList>
#include <QColor>

namespace QmlDesigner {

class PaletteColor;

class SimpleColorPaletteModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit SimpleColorPaletteModel(QObject *parent = nullptr);
    ~SimpleColorPaletteModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clearItems();
    Q_INVOKABLE void addItem(const QString &item);
    void addItem(const PaletteColor &item);

    const QList<PaletteColor> &items() const;

    void sortItems();

    static void registerDeclarativeType();

    Q_INVOKABLE void toggleFavorite(int id);

    bool read();
    void write();

    Q_INVOKABLE void showDialog(QColor color);

signals:
    void colorDialogRejected();
    void currentColorChanged(const QColor &color);

private slots:
    void setPalette();

private:
    void enqueue(const PaletteColor &item);

private:
    int m_paletteSize;
    int m_favoriteOffset;
    QList<PaletteColor> m_items;
    QHash<int, QByteArray> m_roleNames;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::SimpleColorPaletteModel)
