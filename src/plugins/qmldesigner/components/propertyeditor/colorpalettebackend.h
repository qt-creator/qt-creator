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

const int g_maxPaletteSize = 8;
const QString g_recent = "Recent";
const QString g_favorite = "Favorite";

class EyeDropperEventFilter : public QObject
{
public:
    enum class LeaveReason { Default, Cancel };

    explicit EyeDropperEventFilter(std::function<void(QPoint, LeaveReason)> callOnLeave,
                                   std::function<void(QPoint)> callOnUpdate)
        : m_leave(callOnLeave)
        , m_update(callOnUpdate)
    {}

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::MouseMove: {
            m_lastPosition = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
            m_update(m_lastPosition);
            return true;
        }
        case QEvent::MouseButtonRelease: {
            m_lastPosition = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
            m_leave(m_lastPosition, EyeDropperEventFilter::LeaveReason::Default);
            return true;
        }
        case QEvent::MouseButtonPress:
            return true;
        case QEvent::KeyPress: {
            auto keyEvent = static_cast<QKeyEvent *>(event);
#if QT_CONFIG(shortcut)
            if (keyEvent->matches(QKeySequence::Cancel))
                m_leave(m_lastPosition, EyeDropperEventFilter::LeaveReason::Cancel);
            else
#endif
                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                m_leave(m_lastPosition, EyeDropperEventFilter::LeaveReason::Default);
            } else if (keyEvent->key() == Qt::Key_Escape) {
                m_leave(m_lastPosition, EyeDropperEventFilter::LeaveReason::Cancel);
            }
            keyEvent->accept();
            return true;
        }
        default:
            return QObject::eventFilter(obj, event);
        }
    }

private:
    std::function<void(QPoint, LeaveReason)> m_leave;
    std::function<void(QPoint)> m_update;
    QPoint m_lastPosition;
};

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
    Q_PROPERTY(bool eyeDropperActive READ eyeDropperActive NOTIFY eyeDropperActiveChanged)

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

    //Q_INVOKABLE void eyeDropper();

    Q_INVOKABLE void invokeEyeDropper();

    QColor grabScreenColor(const QPoint &p);
    QImage grabScreenRect(const QRect &r);
    void updateEyeDropper();
    void updateEyeDropperPosition(const QPoint &globalPos);

    void updateCursor(const QImage &image);

    //void releaseEyeDropper();
    //
    //bool handleEyeDropperMouseMove(QMouseEvent *e);
    //bool handleEyeDropperMouseButtonRelease(QMouseEvent *e);
    //bool handleEyeDropperKeyPress(QKeyEvent *e);

    bool eyeDropperActive() const;

    ColorPaletteBackend(const ColorPaletteBackend &) = delete;
    void operator=(const ColorPaletteBackend &) = delete;

signals:
    void currentPaletteChanged(const QString &palette);
    void currentPaletteColorsChanged();
    void palettesChanged();

    void colorDialogRejected();
    void currentColorChanged(const QColor &color);

    void eyeDropperRejected();
    void eyeDropperActiveChanged();

private:
    ColorPaletteBackend();

    void eyeDropperEnter();
    void eyeDropperLeave(const QPoint &pos, EyeDropperEventFilter::LeaveReason actionOnLeave);
    void eyeDropperPointerMoved(const QPoint &pos);

private:
    static QPointer<ColorPaletteBackend> m_instance;
    QString m_currentPalette;
    QStringList m_currentPaletteColors;
    QHash<QString, Palette> m_data;

    std::unique_ptr<EyeDropperEventFilter> m_eyeDropperEventFilter;
    QPointer<QWindow> m_eyeDropperWindow;
    QColor m_eyeDropperCurrentColor;
    QColor m_eyeDropperPreviousColor;
    bool m_eyeDropperActive = false;
#ifdef Q_OS_WIN32
    QTimer *updateTimer;
    QWindow dummyTransparentWindow;
#endif
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::ColorPaletteBackend)
