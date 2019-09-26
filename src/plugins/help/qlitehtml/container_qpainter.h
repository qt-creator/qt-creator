/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of QLiteHtml.
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

#include <litehtml.h>

#include <QFont>
#include <QHash>
#include <QPaintDevice>
#include <QPalette>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QTextDocument>
#include <QUrl>

#include <functional>
#include <unordered_map>

class Selection
{
public:
    struct Element
    {
        litehtml::element::ptr element;
        int index = -1;
        int x = -1;
    };

    enum class Mode { Free, Word };

    bool isValid() const;

    void update();
    QRect boundingRect() const;

    Element startElem;
    Element endElem;
    QVector<QRect> selection;
    QString text;

    QPoint selectionStartDocumentPos;
    Mode mode = Mode::Free;
    bool isSelecting = false;
};

struct Index
{
    QString text;
    // only contains leaf elements
    std::unordered_map<litehtml::element::ptr, int> elementToIndex;

    using Entry = std::pair<int, litehtml::element::ptr>;
    std::vector<Entry> indexToElement;

    Entry findElement(int index) const;
};

class DocumentContainer : public litehtml::document_container
{
public:
    DocumentContainer();
    virtual ~DocumentContainer();

public: // document_container API
    litehtml::uint_ptr create_font(const litehtml::tchar_t *faceName,
                                   int size,
                                   int weight,
                                   litehtml::font_style italic,
                                   unsigned int decoration,
                                   litehtml::font_metrics *fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    int text_width(const litehtml::tchar_t *text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc,
                   const litehtml::tchar_t *text,
                   litehtml::uint_ptr hFont,
                   litehtml::web_color color,
                   const litehtml::position &pos) override;
    int pt_to_px(int pt) override;
    int get_default_font_size() const override;
    const litehtml::tchar_t *get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker &marker) override;
    void load_image(const litehtml::tchar_t *src,
                    const litehtml::tchar_t *baseurl,
                    bool redraw_on_ready) override;
    void get_image_size(const litehtml::tchar_t *src,
                        const litehtml::tchar_t *baseurl,
                        litehtml::size &sz) override;
    void draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint &bg) override;
    void draw_borders(litehtml::uint_ptr hdc,
                      const litehtml::borders &borders,
                      const litehtml::position &draw_pos,
                      bool root) override;
    void set_caption(const litehtml::tchar_t *caption) override;
    void set_base_url(const litehtml::tchar_t *base_url) override;
    void link(const std::shared_ptr<litehtml::document> &doc,
              const litehtml::element::ptr &el) override;
    void on_anchor_click(const litehtml::tchar_t *url, const litehtml::element::ptr &el) override;
    void set_cursor(const litehtml::tchar_t *cursor) override;
    void transform_text(litehtml::tstring &text, litehtml::text_transform tt) override;
    void import_css(litehtml::tstring &text,
                    const litehtml::tstring &url,
                    litehtml::tstring &baseurl) override;
    void set_clip(const litehtml::position &pos,
                  const litehtml::border_radiuses &bdr_radius,
                  bool valid_x,
                  bool valid_y) override;
    void del_clip() override;
    void get_client_rect(litehtml::position &client) const override;
    std::shared_ptr<litehtml::element> create_element(
        const litehtml::tchar_t *tag_name,
        const litehtml::string_map &attributes,
        const std::shared_ptr<litehtml::document> &doc) override;
    void get_media_features(litehtml::media_features &media) const override;
    void get_language(litehtml::tstring &language, litehtml::tstring &culture) const override;

public: // outside API
    void setPaintDevice(QPaintDevice *paintDevice);
    void setDocument(const QByteArray &data, litehtml::context *context);
    litehtml::document::ptr document() const;
    void setScrollPosition(const QPoint &pos);
    void render(int width, int height);
    void draw(QPainter *painter, const QRect &clip);

    // these return areas to redraw in document space
    QVector<QRect> mousePressEvent(const QPoint &documentPos,
                                   const QPoint &viewportPos,
                                   Qt::MouseButton button);
    QVector<QRect> mouseMoveEvent(const QPoint &documentPos, const QPoint &viewportPos);
    QVector<QRect> mouseReleaseEvent(const QPoint &documentPos,
                                     const QPoint &viewportPos,
                                     Qt::MouseButton button);
    QVector<QRect> mouseDoubleClickEvent(const QPoint &documentPos,
                                         const QPoint &viewportPos,
                                         Qt::MouseButton button);
    QVector<QRect> leaveEvent();

    QUrl linkAt(const QPoint &documentPos, const QPoint &viewportPos);

    QString caption() const;
    QString selectedText() const;

    void findText(const QString &text,
                  QTextDocument::FindFlags flags,
                  bool incremental,
                  bool *wrapped,
                  bool *success,
                  QVector<QRect> *oldSelection,
                  QVector<QRect> *newSelection);

    void setDefaultFont(const QFont &font);
    QFont defaultFont() const;

    using DataCallback = std::function<QByteArray(QUrl)>;
    void setDataCallback(const DataCallback &callback);

    using CursorCallback = std::function<void(QCursor)>;
    void setCursorCallback(const CursorCallback &callback);

    using LinkCallback = std::function<void(QUrl)>;
    void setLinkCallback(const LinkCallback &callback);

    using PaletteCallback = std::function<QPalette()>;
    void setPaletteCallback(const PaletteCallback &callback);

private:
    QPixmap getPixmap(const QString &imageUrl, const QString &baseUrl);
    QString serifFont() const;
    QString sansSerifFont() const;
    QString monospaceFont() const;
    QUrl resolveUrl(const QString &url, const QString &baseUrl) const;
    void drawSelection(QPainter *painter, const QRect &clip) const;
    void buildIndex();

    QPaintDevice *m_paintDevice = nullptr;
    litehtml::document::ptr m_document;
    Index m_index;
    QString m_baseUrl;
    QRect m_clientRect;
    QPoint m_scrollPosition;
    QString m_caption;
    QFont m_defaultFont = QFont(sansSerifFont(), 16);
    QByteArray m_defaultFontFamilyName = m_defaultFont.family().toUtf8();
    QHash<QUrl, QPixmap> m_pixmaps;
    Selection m_selection;
    DataCallback m_dataCallback;
    CursorCallback m_cursorCallback;
    LinkCallback m_linkCallback;
    PaletteCallback m_paletteCallback;
    bool m_blockLinks = false;
};
