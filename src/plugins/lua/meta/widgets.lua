---@meta Widgets

local widgets = {}

---A QWidget, see https://doc.qt.io/qt-6/QWidget.html
---@class QWidget : QObject
---@field acceptDrops boolean
---@field accessibleDescription string
---@field accessibleName string
---@field autoFillBackground boolean
---@field baseSize QSize
---@field childrenRect QRect
---@field childrenRegion QRegion
---@field contextMenuPolicy QtContextMenuPolicy
---@field cursor QCursor
---@field enabled boolean
---@field focus boolean
---@field focusPolicy QtFocusPolicy
---@field font QFont
---@field frameGeometry QRect
---@field frameSize QSize
---@field fullScreen boolean
---@field geometry QRect
---@field height integer
---@field inputMethodHints QtInputMethodHints
---@field isActiveWindow boolean
---@field layoutDirection QtLayoutDirection
---@field locale QLocale
---@field maximized boolean
---@field maximumHeight integer
---@field maximumSize QSize
---@field maximumWidth integer
---@field minimized boolean
---@field minimumHeight integer
---@field minimumSize QSize
---@field minimumSizeHint QSize
---@field minimumWidth integer
---@field modal boolean
---@field mouseTracking boolean
---@field normalGeometry QRect
---@field palette QPalette
---@field pos QPoint
---@field rect QRect
---@field size QSize
---@field sizeHint QSize
---@field sizeIncrement QSize
---@field sizePolicy QSizePolicy
---@field statusTip string
---@field styleSheet string
---@field tabletTracking boolean
---@field toolTip string
---@field toolTipDuration integer
---@field updatesEnabled boolean
---@field visible boolean
---@field whatsThis string
---@field width integer
---@field windowFilePath string
---@field windowFlags QtWindowFlags
---@field windowIcon QIcon
---@field windowModality QtWindowModality
---@field windowModified boolean
---@field windowOpacity double
---@field windowTitle string
---@field x integer
---@field y integer
widgets.QWidget = {}

---@return boolean
function widgets.QWidget:close() end

function widgets.QWidget:hide() end

function widgets.QWidget:lower() end

function widgets.QWidget:raise() end

function widgets.QWidget:repaint() end

function widgets.QWidget:setDisabled(disable) end

function widgets.QWidget:setEnabled(enabled) end

function widgets.QWidget:setFocus() end

function widgets.QWidget:setHidden(hidden) end

function widgets.QWidget:setStyleSheet(styleSheet) end

function widgets.QWidget:setVisible(visible) end

function widgets.QWidget:setWindowModified(modified) end

function widgets.QWidget:setWindowTitle(title) end

function widgets.QWidget:show() end

function widgets.QWidget:showFullScreen() end

function widgets.QWidget:showMaximized() end

function widgets.QWidget:showMinimized() end

function widgets.QWidget:showNormal() end

function widgets.QWidget:update() end

---A QDialog, see https://doc.qt.io/qt-6/qdialog.html
---@class QDialog : QWidget
widgets.QDialog = {}

return widgets
