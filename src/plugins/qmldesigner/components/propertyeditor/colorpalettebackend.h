// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QList>
#include <QMouseEvent>
#include <QWindow>

#include <QPixmap>

#include "designersettings.h"
#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>

namespace QmlDesigner {

class QColorPickingEventFilter;

const int g_maxPaletteSize = 8;
const QString g_recent = "Recent";
const QString g_favorite = "Favorite";

struct Palette
{
    Palette()
        : m_settingsKey()
        , m_colors()
    {}

    Palette(const QByteArray &key)
        : m_settingsKey(key)
        , m_colors()
    {}

    bool read()
    {
        QStringList data = QmlDesignerPlugin::settings().value(m_settingsKey).toStringList();
        if (data.isEmpty())
            return false;

        m_colors.clear();
        m_colors = data;

        return true;
    }

    void write() const
    {
        QmlDesignerPlugin::settings().insert(m_settingsKey, m_colors);
    }

    QByteArray m_settingsKey;
    QStringList m_colors;
};

class ColorPaletteBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList currentPaletteColors
               READ currentPaletteColors
               NOTIFY currentPaletteColorsChanged)
    Q_PROPERTY(QString currentPalette
               READ currentPalette
               WRITE setCurrentPalette
               NOTIFY currentPaletteChanged)
    Q_PROPERTY(QStringList palettes
               READ palettes
               NOTIFY palettesChanged)

public:
    ~ColorPaletteBackend();

    void readPalettes();
    void writePalettes();

    void addColor(const QString &color, const QString &palette);
    void removeColor(int id, const QString &palette);

    Q_INVOKABLE void addRecentColor(const QString &color);

    Q_INVOKABLE void addFavoriteColor(const QString &color);
    Q_INVOKABLE void removeFavoriteColor(int id);

    // Returns list of palette names
    QStringList palettes() const;

    // Get/Set currently active palette
    const QString &currentPalette() const;
    void setCurrentPalette(const QString &palette);

    // Returns the colors of the currently active palette as a list
    const QStringList &currentPaletteColors() const;

    static void registerDeclarativeType();

    Q_INVOKABLE void showDialog(QColor color);


    Q_INVOKABLE void eyeDropper();

    QColor grabScreenColor(const QPoint &p);
    QImage grabScreenRect(const QPoint &p);
    void updateEyeDropper();
    void updateEyeDropperPosition(const QPoint &globalPos);

    void updateCursor(const QImage &image);

    void releaseEyeDropper();

    bool handleEyeDropperMouseMove(QMouseEvent *e);
    bool handleEyeDropperMouseButtonRelease(QMouseEvent *e);
    bool handleEyeDropperKeyPress(QKeyEvent *e);


    ColorPaletteBackend(const ColorPaletteBackend &) = delete;
    void operator=(const ColorPaletteBackend &) = delete;

signals:
    void currentPaletteChanged(const QString &palette);
    void currentPaletteColorsChanged();
    void palettesChanged();

    void colorDialogRejected();
    void currentColorChanged(const QColor &color);

    void eyeDropperRejected();

private:
    ColorPaletteBackend();

private:
    static QPointer<ColorPaletteBackend> m_instance;
    QString m_currentPalette;
    QStringList m_currentPaletteColors;
    QHash<QString, Palette> m_data;

    QColorPickingEventFilter *m_colorPickingEventFilter;
#ifdef Q_OS_WIN32
    QTimer *updateTimer;
    QWindow dummyTransparentWindow;
#endif
};

class QColorPickingEventFilter : public QObject {
public:
    explicit QColorPickingEventFilter(ColorPaletteBackend *colorPalette)
        : QObject(colorPalette)
        , m_colorPalette(colorPalette)
    {}

    bool eventFilter(QObject *, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::MouseMove:
            return m_colorPalette->handleEyeDropperMouseMove(
                        static_cast<QMouseEvent *>(event));
        case QEvent::MouseButtonRelease:
            return m_colorPalette->handleEyeDropperMouseButtonRelease(
                        static_cast<QMouseEvent *>(event));
        case QEvent::KeyPress:
            return m_colorPalette->handleEyeDropperKeyPress(
                        static_cast<QKeyEvent *>(event));
        default:
            break;
        }
        return false;
    }

private:
    ColorPaletteBackend *m_colorPalette;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::ColorPaletteBackend)
