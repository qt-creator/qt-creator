---@meta SimpleTypes

---@class QRect
---@field x integer The x position of the rectangle.
---@field y integer The y position of the rectangle.
---@field width integer The width of the rectangle.
---@field height integer The height of the rectangle.
QRect = {}

---@class QSize
---@field width integer The width of the size.
---@field height integer The height of the size.
QSize = {}

---@class QPoint
---@field x integer The x position of the point.
---@field y integer The y position of the point.
QPoint = {}

---@class QMargins
---@field left integer The left margin.
---@field top integer The top margin.
---@field right integer The right margin.
---@field bottom integer The bottom margin.
QMargins = {}

---@class QPointF
---@field x number The x position of the floating point.
---@field y number The y position of the floating point.
QPointF = {}

---@class QSizeF
---@field width number The width of the floating point size.
---@field height number The height of the floating point size.
QSizeF = {}

---@class QRectF A rectangle with floating point coordinates.
---@field x number The x position of the floating point rectangle.
---@field y number The y position of the floating point rectangle.
---@field width number The width of the floating point rectangle.
---@field height number The height of the floating point rectangle.
QRectF = {}

---@class QMarginsF
---@field left number The left margin.
---@field top number The top margin.
---@field right number The right margin.
---@field bottom number The bottom margin.
QMarginsF = {}

---@class NullType
NullType = {}

---Just a workaround to let "Null" show the correct type in the documentation.
---@return NullType null
local function null() end

---A special object to represent a nullptr value.
Null = null()
