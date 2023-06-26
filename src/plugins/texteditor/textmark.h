// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QIcon>
#include <QStaticText>
#include <QVector>

#include <optional>

QT_BEGIN_NAMESPACE
class QAction;
class QGridLayout;
class QLayout;
class QPainter;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace TextEditor {

class TextDocument;

class TextMarkCategory
{
public:
    QString displayName;
    Utils::Id id;
};

class TEXTEDITOR_EXPORT TextMark
{
public:
    TextMark() = delete;
    TextMark(const Utils::FilePath &filePath, int lineNumber, TextMarkCategory category);
    TextMark(TextDocument *document, int lineNumber, TextMarkCategory category);
    virtual ~TextMark();

    // determine order on markers on the same line.
    enum Priority
    {
        LowPriority,
        NormalPriority,
        HighPriority // shown on top.
    };

    Utils::FilePath filePath() const;
    int lineNumber() const;

    virtual void paintIcon(QPainter *painter, const QRect &rect) const;
    virtual void paintAnnotation(QPainter &painter,
                                 const QRect &eventRect,
                                 QRectF *annotationRect,
                                 const qreal fadeInOffset,
                                 const qreal fadeOutOffset,
                                 const QPointF &contentOffset) const;
    struct AnnotationRects
    {
        QRectF fadeInRect;
        QRectF annotationRect;
        QRectF iconRect;
        QRectF textRect;
        QRectF fadeOutRect;
        QString text;
    };
    AnnotationRects annotationRects(const QRectF &boundingRect, const QFontMetrics &fm,
                                    const qreal fadeInOffset, const qreal fadeOutOffset) const;
    /// called if the filename of the document changed
    virtual void updateFilePath(const Utils::FilePath &filePath);
    virtual void updateLineNumber(int lineNumber);
    virtual void updateBlock(const QTextBlock &block);
    virtual void move(int line);
    virtual void removedFromEditor();
    virtual bool isClickable() const;
    virtual void clicked();
    virtual bool isDraggable() const;
    virtual void dragToLine(int lineNumber);
    void addToToolTipLayout(QGridLayout *target) const;
    virtual bool addToolTipContent(QLayout *target) const;
    virtual QColor annotationColor() const;

    void setIcon(const QIcon &icon);
    void setIconProvider(const std::function<QIcon()> &iconProvider);
    const QIcon icon() const;
    void updateMarker();
    Priority priority() const { return m_priority;}
    void setPriority(Priority prioriy);
    bool isVisible() const;
    void setVisible(bool isVisible);
    TextMarkCategory category() const { return m_category; }

    std::optional<Utils::Theme::Color> color() const;
    void setColor(const Utils::Theme::Color &color);

    QString defaultToolTip() const { return m_defaultToolTip; }
    void setDefaultToolTip(const QString &toolTip) { m_defaultToolTip = toolTip; }

    TextDocument *baseTextDocument() const { return m_baseTextDocument; }
    void setBaseTextDocument(TextDocument *baseTextDocument) { m_baseTextDocument = baseTextDocument; }

    QString lineAnnotation() const { return m_lineAnnotation; }
    void setLineAnnotation(const QString &lineAnnotation);

    QString toolTip() const;
    void setToolTip(const QString &toolTip);
    void setToolTipProvider(const std::function<QString ()> &toolTipProvider);

    QVector<QAction *> actions() const;
    void setActions(const QVector<QAction *> &actions); // Takes ownership
    void setActionsProvider(const std::function<QList<QAction *>()> &actionsProvider); // Takes ownership

    bool isLocationMarker() const;
    void setIsLocationMarker(bool newIsLocationMarker);


protected:
    void setSettingsPage(Utils::Id settingsPage);

private:
    Q_DISABLE_COPY(TextMark)

    void setDeleteCallback(const std::function<void()> &callback) { m_deleteCallback = callback; };

    TextDocument *m_baseTextDocument = nullptr;
    Utils::FilePath m_fileName;
    int m_lineNumber = 0;
    Priority m_priority = LowPriority;
    bool m_isLocationMarker = false;
    QIcon m_icon;
    std::function<QIcon()> m_iconProvider;
    std::optional<Utils::Theme::Color> m_color;
    bool m_visible = false;
    TextMarkCategory m_category;
    QString m_lineAnnotation;
    mutable QStaticText m_staticAnnotationText;
    QString m_toolTip;
    std::function<QString()> m_toolTipProvider;
    QString m_defaultToolTip;
    QVector<QAction *> m_actions; // FIXME Remove in master
    std::function<QList<QAction *>()> m_actionsProvider;
    Utils::Id m_settingsPage;
    std::function<void()> m_deleteCallback;

    friend class TextDocumentLayout;
};

} // namespace TextEditor
