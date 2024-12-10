/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file    qanStyle.h
// \author  benoit@destrat.io
// \date    2015 06 05
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QColor>
#include <QFont>
#include <QSizeF>
#include <QVector>
#include <QQmlEngine>

namespace qan { // ::qan

/*! Models a set of properties affecting a graph primitive visual appearance.
 *
 *  Style instances should be created from StyleManager createStyle() or createStyleFrom() methods,
 *  and associed to nodes trought their setStyle() method. Changing a style property will
 *  automatically be reflected in styled nodes or edges appearance. While Style use standard QObject
 *  properties to store style settings, the QuickContainers ObjectModel interface could be use to edit
 *  style properties.
 *
 *  Main qan::Style properties are:
 * \li \b name: Style name as it will appears in style edition dialogs.
 *
 * \sa qan::NodeItem::setStyle()
 * \sa qan::EdgeItem::setStyle()
 */
class Style : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /*! \name Style Object Management *///-------------------------------------
    //@{
public:
    /*! \brief Style constructor with style name initialisation.
     */
    Style(QObject* parent = nullptr);
    explicit Style(QString name, QObject* parent = nullptr);
    virtual ~Style() override = default;
    Style(const Style&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Style Management *///--------------------------------------------
    //@{
public:
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged FINAL)
    inline  void        setName(QString name) noexcept { if (name != _name) { _name = name; emit nameChanged(); } }
    inline  QString     getName() noexcept { return _name; }
    inline  QString     getName() const noexcept { return _name; }
signals:
    void        nameChanged();
private:
    QString     _name = QString{};
    //@}
    //-------------------------------------------------------------------------
};

class NodeStyle : public qan::Style
{
    /*! \name NodeStyle Object Management *///---------------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT
public:
    /*! \brief Style constructor with style \c name and \c target initialisation.
     *
     * Style \c metaTarget is "qan::Node". NodeStyle objects are usually created
     * with qan:StyleManager::createNodeStyle() factory method.
     */
    NodeStyle(QObject* parent = nullptr);
    explicit NodeStyle(QString name, QObject* parent = nullptr);
    virtual ~NodeStyle() override = default;
    NodeStyle(const NodeStyle&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Node Style Properties *///---------------------------------------
    //@{
public:
    /*! \brief Node background rectangle border (corner) radius (default to 4.).
     *
     * \note \c backRadius is interpreted as background rectangle border radius when nodes items are
     * built using Qan.RectNodeTemplate and Qan.RectSolidBackground. When defining custom node
     * items, value can't be interpreted by the user with no limitations.
     */
    Q_PROPERTY(qreal backRadius READ getBackRadius WRITE setBackRadius NOTIFY backRadiusChanged FINAL)
    //! \copydoc backRadius
    bool            setBackRadius(qreal backRadius) noexcept;
    //! \copydoc backRadius
    inline qreal    getBackRadius() const noexcept { return _backRadius; }
protected:
    //! \copydoc backRadius
    qreal           _backRadius = 4.;
signals:
    //! \copydoc backRadius
    void            backRadiusChanged();

public:
    /*! \brief Node item background opacity (default to 0.85, ie 85% opaque).
     *
     * \note \c backOpacity affect node item background, but not it's content or border shadow opacity. Changing
     * node opacity directly is more efficient, but also affect node content and shadows. Property works
     * when using Qan.RectNodeTemplate and Qan.RectGroupTemplate, it could be used in custom node items with no
     * limitations (Qan.RectSolidBackground could be used to add backOpacity support to custom node delegates).
     *
     */
    Q_PROPERTY(qreal backOpacity READ getBackOpacity WRITE setBackOpacity NOTIFY backOpacityChanged FINAL)
    //! \copydoc backOpacity
    bool            setBackOpacity(qreal backOpacity) noexcept;
    //! \copydoc backOpacity
    inline qreal    getBackOpacity() const noexcept { return _backOpacity; }
protected:
    //! \copydoc backOpacity
    qreal           _backOpacity = 0.80;
signals:
    //! \copydoc backOpacity
    void            backOpacityChanged();

public:
    /*! \brief Define how node background is filled (either with a solid plain color or a gradient), default to solid.
     *
     * In FillType::FillGradient, \c baseColor is used at gradient for linear gradient origin, \c backColor
     * as final color value.
     *
     * \note 20180325: Gradient fill orientation is currently fixed from top left to bottom right.
     * \since 0.9.4
     */
    enum class FillType : unsigned int {
        Undefined       = 0,    // Reserved for serialization purposes, do not use Undefined = set to default value
        FillSolid       = 1,
        FillGradient    = 2
    };
    Q_ENUM(FillType)

public:
    //! \copydoc FillType
    Q_PROPERTY(FillType fillType READ getFillType WRITE setFillType NOTIFY fillTypeChanged FINAL)
    //! \copydoc FillType
    bool                setFillType(FillType fillType) noexcept;
    //! \copydoc FillType
    inline FillType     getFillType() const noexcept { return _fillType; }
protected:
    //! \copydoc FillType
    FillType            _fillType{FillType::FillSolid};
signals:
    //! \copydoc FillType
    void                fillTypeChanged();

public:
    //! \brief Node background color (default to white).
    Q_PROPERTY(QColor backColor READ getBackColor WRITE setBackColor NOTIFY backColorChanged FINAL)
    bool            setBackColor(const QColor& backColor) noexcept;
    const QColor&   getBackColor() const noexcept { return _backColor; }
protected:
    QColor          _backColor{Qt::white};
signals:
    void            backColorChanged();

public:
    /*! \brief Base color is used as gradient start color when \c fillType is set to FillType::Gradient (default to white).
     *
     * \since 0.9.4
     */
    Q_PROPERTY(QColor baseColor READ getBaseColor WRITE setBaseColor NOTIFY baseColorChanged FINAL)
    bool            setBaseColor(const QColor& backColor) noexcept;
    const QColor&   getBaseColor() const noexcept { return _baseColor; }
protected:
    QColor          _baseColor{Qt::white};
signals:
    void            baseColorChanged();

public:
    Q_PROPERTY(QColor borderColor READ getBorderColor WRITE setBorderColor NOTIFY borderColorChanged FINAL)
    bool            setBorderColor(const QColor& borderColor) noexcept;
    const QColor&   getBorderColor() const noexcept { return _borderColor; }
protected:
    QColor          _borderColor = QColor(Qt::black);
signals:
    void            borderColorChanged();

public:
    Q_PROPERTY(qreal borderWidth READ getBorderWidth WRITE setBorderWidth NOTIFY borderWidthChanged FINAL)
    bool            setBorderWidth(qreal borderWidth) noexcept;
    inline qreal    getBorderWidth() const noexcept { return _borderWidth; }
protected:
    qreal           _borderWidth = 1.0;
signals:
    void            borderWidthChanged();

public:
    //! Define what effect should be used when drawing node background (either nothing, a drop shadow or a glow effect).
    enum class EffectType : unsigned int {
        Undefined       = 0,
        EffectNone      = 1,
        EffectShadow    = 2,
        EffectGlow      = 3
    };
    Q_ENUM(EffectType)

public:
    Q_PROPERTY(EffectType effectType READ getEffectType WRITE setEffectType NOTIFY effectTypeChanged FINAL)
    bool                setEffectType(EffectType effectType) noexcept;
    inline EffectType   getEffectType() const noexcept { return _effectType; }
protected:
    EffectType          _effectType = EffectType::EffectShadow;
signals:
    void                effectTypeChanged();

public:
    Q_PROPERTY(bool effectEnabled READ getEffectEnabled WRITE setEffectEnabled NOTIFY effectEnabledChanged FINAL)
    bool            setEffectEnabled(bool effectEnabled) noexcept;
    inline bool     getEffectEnabled() const noexcept { return _effectEnabled; }
protected:
    bool            _effectEnabled = true;
signals:
    void            effectEnabledChanged();

public:
    Q_PROPERTY(QColor effectColor READ getEffectColor WRITE setEffectColor NOTIFY effectColorChanged FINAL)
    bool            setEffectColor(QColor effectColor) noexcept;
    inline QColor   getEffectColor() const noexcept { return _effectColor; }
protected:
    QColor          _effectColor = QColor{0, 0, 0, 127};
signals:
    void            effectColorChanged();

public:
    Q_PROPERTY(qreal effectRadius READ getEffectRadius WRITE setEffectRadius NOTIFY effectRadiusChanged FINAL)
    bool            setEffectRadius(qreal effectRadius) noexcept;
    inline qreal    getEffectRadius() const noexcept { return _effectRadius; }
protected:
    qreal           _effectRadius = 3.;
signals:
    void            effectRadiusChanged();

public:
    /*! \brief Effect offset, when effect is a shadow offset is drop shadow offset, no meaning for glow effect.
     *
     * Any value is considered valid (even negative offset for drop shadows).
     */
    Q_PROPERTY(qreal effectOffset READ getEffectOffset WRITE setEffectOffset NOTIFY effectOffsetChanged FINAL)
    bool            setEffectOffset(qreal effectOFfset) noexcept;
    inline qreal    getEffectOffset() const noexcept { return _effectOffset; }
protected:
    qreal           _effectOffset = 3.;
signals:
    void            effectOffsetChanged();

public:
    /*! \brief Node content text font \c pointSize, set to -1 to use system default (default to -1 ie system default size).
     */
    Q_PROPERTY(int fontPointSize READ getFontPointSize WRITE setFontPointSize NOTIFY fontPointSizeChanged FINAL)
    //! \copydoc fontPointSize
    bool            setFontPointSize(int fontPointSize) noexcept;
    //! \copydoc fontPointSize
    inline int      getFontPointSize() const noexcept { return _fontPointSize; }
protected:
    //! \copydoc fontPointSize
    int             _fontPointSize{-1};
signals:
    //! \copydoc fontPointSize
    void            fontPointSizeChanged();

public:
    /*! \brief Set to true to display node text with bold font (default to false).
     */
    Q_PROPERTY(bool fontBold READ getFontBold WRITE setFontBold NOTIFY fontBoldChanged FINAL)
    //! \copydoc fontBold
    bool            setFontBold(bool fontBold) noexcept;
    //! \copydoc fontBold
    inline bool     getFontBold() const noexcept { return _fontBold; }
protected:
    //! \copydoc fontBold
    bool            _fontBold = false;
signals:
    //! \copydoc fontBold
    void            fontBoldChanged();

public:
    /*! \brief Color to use to paint the node label (default to black)
     */
    Q_PROPERTY(QColor labelColor READ getLabelColor WRITE setLabelColor NOTIFY labelColorChanged FINAL)
    //! \copydoc labelColor
    bool            setLabelColor(QColor labelColor) noexcept;
    //! \copydoc labelColor
    inline QColor   getLabelColor() const noexcept { return _labelColor; }
protected:
    //! \copydoc labelColor
    QColor          _labelColor = QColor{"black"};
signals:
    //! \copydoc labelColor
    void            labelColorChanged();
    //@}
    //-------------------------------------------------------------------------
};

class EdgeStyle : public qan::Style
{
    Q_OBJECT
    QML_ELEMENT
    /*! \name EdgeStyle Object Management *///---------------------------------
    //@{
public:
    /*! \brief Edge style constructor with style \c name and \c target initialisation.
     *
     */
    EdgeStyle(QObject* parent = nullptr);
    explicit EdgeStyle(QString name, QObject* parent = nullptr);
    virtual ~EdgeStyle() override = default;
    EdgeStyle(const EdgeStyle&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Properties Management *///---------------------------------------
    //@{
signals:
    void            styleModified();

public:
    //! End type drawing configuration
    enum class ArrowShape {
        //! Do not draw an end.
        None = 0,
        //! End shape is an arrow.
        Arrow = 1,
        //! End shape is an open arrow.
        ArrowOpen = 2,
        //! End shape is a filled circle.
        Circle = 3,
        //! End shape is an open circle.
        CircleOpen = 4,
        //! End shape is a filled rectangle.
        Rect = 5,
        //! End shape is a open rectangle.
        RectOpen = 6
    };
    Q_ENUM(ArrowShape)

    //! Define edge style: either straight (drawn with a line) or curved (drawn with a cubic path).
    enum class LineType : unsigned int {
        Undefined   = 0,
        Straight    = 1,
        Curved      = 2,
        Ortho       = 3
    };
    Q_ENUM(LineType)

public:
    //! Define edge line type: either plain line (Straight) or curved cubic path (Curved), default to Straight.
    Q_PROPERTY(LineType lineType READ getLineType WRITE setLineType NOTIFY lineTypeChanged FINAL)
    bool            setLineType(LineType lineType) noexcept;
    inline LineType getLineType() const noexcept { return _lineType; }
protected:
    LineType        _lineType = LineType::Straight;
signals:
    void            lineTypeChanged();

public:
    Q_PROPERTY(QColor lineColor READ getLineColor WRITE setLineColor NOTIFY lineColorChanged FINAL)
    bool                    setLineColor(const QColor& lineColor) noexcept;
    inline const QColor&    getLineColor() const noexcept { return _lineColor; }
protected:
    QColor                  _lineColor = QColor{0, 0, 0, 255};
signals:
    void                    lineColorChanged();

public:
    Q_PROPERTY(qreal lineWidth READ getLineWidth WRITE setLineWidth NOTIFY lineWidthChanged FINAL)
    bool            setLineWidth(qreal lineWidth) noexcept;
    inline qreal    getLineWidth() const noexcept { return _lineWidth; }
protected:
    qreal           _lineWidth = 3.0;
signals:
    void            lineWidthChanged();

public:
    Q_PROPERTY(qreal arrowSize READ getArrowSize WRITE setArrowSize NOTIFY arrowSizeChanged FINAL)
    bool            setArrowSize(qreal arrowSize) noexcept;
    inline qreal    getArrowSize() const noexcept { return _arrowSize; }
protected:
    qreal           _arrowSize = 4.0;
signals:
    void            arrowSizeChanged();

public:

    //! \copydoc Define shape of source arrow, default None.
    Q_PROPERTY(qan::EdgeStyle::ArrowShape srcShape READ getSrcShape WRITE setSrcShape NOTIFY srcShapeChanged FINAL)
    //! \copydoc srcShape
    inline ArrowShape   getSrcShape() const noexcept { return _srcShape; }
    //! \copydoc srcShape
    auto                setSrcShape(ArrowShape srcShape) noexcept -> bool;
private:
    //! \copydoc srcShape
    ArrowShape          _srcShape = ArrowShape::None;
signals:
    void                srcShapeChanged();

public:
    //! \copydoc Define shape of destination arrow, default arrow.
    Q_PROPERTY(qan::EdgeStyle::ArrowShape dstShape READ getDstShape WRITE setDstShape NOTIFY dstShapeChanged FINAL)

    //! \copydoc dstShape
    inline ArrowShape   getDstShape() const noexcept { return _dstShape; }
    //! \copydoc dstShape
    auto                setDstShape(ArrowShape dstShape) noexcept -> bool;
private:
    //! \copydoc dstShape
    ArrowShape          _dstShape = ArrowShape::Arrow;
signals:
    void                dstShapeChanged();

public:
    //! Draw edge with dashed line (default to false), when set to true \c dashPattern is active.
    Q_PROPERTY(bool dashed READ getDashed WRITE setDashed NOTIFY dashedChanged FINAL)
    //! \copydoc dashed
    bool            setDashed(bool dashed) noexcept;
    //! \copydoc dashed
    inline bool     getDashed() const noexcept { return _dashed; }
protected:
    //! \copydoc dashed
    bool            _dashed = false;
signals:
    //! \copydoc dashed
    void            dashedChanged();

public:
    //! See QtQuick.Shape ShapePath.dashPattern property documentation, default to [2,2] ie regular dash line, used when dashed property is set to true.
    Q_PROPERTY(QVector<qreal> dashPattern READ getDashPattern WRITE setDashPattern NOTIFY dashPatternChanged FINAL)
    //! \copydoc dashPattern
    bool            setDashPattern(const QVector<qreal>& dashPattern) noexcept;
    //! \copydoc dashPattern
    const QVector<qreal>&   getDashPattern() const noexcept;
protected:
    //! \copydoc dashPattern
    QVector<qreal>  _dashPattern{2, 2};
signals:
    //! \copydoc dashPattern
    void            dashPatternChanged();
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::Style)

QML_DECLARE_TYPE(qan::NodeStyle)
Q_DECLARE_METATYPE(qan::NodeStyle::FillType)
Q_DECLARE_METATYPE(qan::NodeStyle::EffectType)

QML_DECLARE_TYPE(qan::EdgeStyle)
Q_DECLARE_METATYPE(qan::EdgeStyle::LineType)
Q_DECLARE_METATYPE(qan::EdgeStyle::ArrowShape)
