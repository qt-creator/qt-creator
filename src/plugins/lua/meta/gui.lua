---@meta Gui

local gui = {}

---The base class of all ui related classes.
---@class Object
gui.Object = {}

---The base class of all gui layout classes.
---@class Layout : Object
gui.layout = {}

---The base class of all widget classes, an empty widget itself.
---@class Widget : Object
gui.widget = {}

---@alias LayoutChild string|BaseAspect|Layout|Widget|function
---@alias LayoutChildren LayoutChild[]

---@class (exact) BaseWidgetOptions
---@field size? integer[] Two integers, representing the width and height of the widget.
---@field windowFlags? WindowType[] The window flags of the widget.
gui.baseWidgetOptions = {}

---@class (exact) WidgetOptions : BaseWidgetOptions
---@field title? string The title of the widget, if applicable.
---@field onTextChanged? function The function to be called when the text of the widget changes, if applicable.
---@field onClicked? function The function to be called when the widget is clicked, if applicable.
---@field text? string The text of the widget, if applicable.
---@field value? integer The value of the widget, if applicable.
---@field [1]? Layout The layout of the widget, if applicable.
gui.widgetOptions = {}

---@param options WidgetOptions
---@return Widget
function gui.Widget(options) end

---Show the widget. (see [QWidget::show](https://doc.qt.io/qt-5/qwidget.html#show))
function gui.widget:show() end

---Sets the top-level widget containing this widget to be the active window. (see [QWidget::activateWindow](https://doc.qt.io/qt-5/qwidget.html#activateWindow))
function gui.widget:activateWindow() end

---Closes the widget. (see [QWidget::close](https://doc.qt.io/qt-5/qwidget.html#close))
function gui.widget:close() end

---Column layout
---@class Column : Layout
local column = {}

---@param children LayoutChildren
---@return Column
function gui.Column(children) end

---A group box with a title.
---@class Group : Widget
local group = {}

---@param options WidgetOptions
---@return Group
function gui.Group(options) end

---Row layout.
---@class Row : Layout
local row = {}

---@param children LayoutChildren
---@return Row
function gui.Row(children) end

---Flow layout.
---@class Flow : Layout
local flow = {}

---@param children LayoutChildren
---@return Flow
function gui.Flow(children) end

---Grid layout.
---@class Grid : Layout
local grid = {}

---@param children LayoutChildren
---@return Grid
function gui.Grid(children) end

---Form layout.
---@class Form : Layout
local form = {}

---@param children LayoutChildren
---@return Form
function gui.Form(children) end

---A stack of multiple widgets.
---@class Stack : Widget
local stack = {}

---@param options WidgetOptions
---@return Stack
function gui.Stack(options) end

---A Tab widget.
---@class Tab : Widget
local tab = {}

---@param name string
---@param layout Layout
---@return Tab
function gui.Tab(name, layout) end

---@class (exact) TabOptions
---@field [1] string The name of the tab.
---@field [2] Layout The layout of the tab.
gui.tabOptions = {}

---@param options TabOptions
---@return Tab tab The new tab.
function gui.Tab(options) end

---A Multiline text edit.
---@class TextEdit : Widget
local textEdit = {}

---@param options WidgetOptions
---@return TextEdit
function gui.TextEdit(options) end

---@class PushButton : Widget
local pushButton = {}

---@param options WidgetOptions
---@return PushButton
function gui.PushButton(options) end

---@class Label : Widget
local label = {}

---@param options WidgetOptions
---@return Label
function gui.Label(options) end

---@class SpinBox : Widget
local spinBox = {}

---@param options WidgetOptions
---@return SpinBox
function gui.SpinBox(options) end

---@class Splitter : Widget
local splitter = {}

---@alias Orientation "horizontal"|"vertical"

---@class (exact) SplitterOptions : BaseWidgetOptions
---@field orientation? Orientation The orientation of the splitter. (default: "vertical")
---@field childrenCollapsible? boolean A boolean, representing whether the children are collapsible. (default: true)
---@field stretchFactors? integer[] A list of integers, representing the stretch factors of the children. (default: {1, ...})
---@field size? integer[] Two integers, representing the width and height of the widget.
---@field [integer] Layout | Widget The splits.
gui.splitterOptions = {}

---@param options SplitterOptions
---@return Splitter
function gui.Splitter(options) end

---@class ToolBar : Widget
local toolBar = {}

---@param options WidgetOptions
---@return ToolBar
function gui.ToolBar(options) end

---@class TabWidget : Widget
local tabWidget = {}

---@param options Tab[]
---@return TabWidget
function gui.TabWidget(options) end

---@param name string
---@param child WidgetOptions
---@return TabWidget
function gui.TabWidget(name, child) end

---A "Line break" in the gui.
function gui.br() end

---A "Stretch" in the layout.
function gui.st() end

---An empty grid cell in a grid layout.
function gui.empty() end

---A horizontal line in the layout.
function gui.hr() end

---Clears the margin of the layout.
function gui.noMargin() end

---Sets the margin of the layout to the default value.
function gui.normalMargin() end

---Sets the alignment of a Grid layout according to the Form layout rules.
function gui.withFormAlignment() end

--- Enum representing various window types.
---@enum WindowType
gui.WindowType = {
    Widget = 0,
    Window = 0,
    Dialog = 0,
    Sheet = 0,
    Drawer = 0,
    Popup = 0,
    Tool = 0,
    ToolTip = 0,
    SplashScreen = 0,
    Desktop = 0,
    SubWindow = 0,
    ForeignWindow = 0,
    CoverWindow = 0,
    WindowType_Mask = 0,
    MSWindowsFixedSizeDialogHint = 0,
    MSWindowsOwnDC = 0,
    BypassWindowManagerHint = 0,
    X11BypassWindowManagerHint = 0,
    FramelessWindowHint = 0,
    WindowTitleHint = 0,
    WindowSystemMenuHint = 0,
    WindowMinimizeButtonHint = 0,
    WindowMaximizeButtonHint = 0,
    WindowMinMaxButtonsHint = 0,
    WindowContextHelpButtonHint = 0,
    WindowShadeButtonHint = 0,
    WindowStaysOnTopHint = 0,
    WindowTransparentForInput = 0,
    WindowOverridesSystemGestures = 0,
    WindowDoesNotAcceptFocus = 0,
    MaximizeUsingFullscreenGeometryHint = 0,
    CustomizeWindowHint = 0,
    WindowStaysOnBottomHint = 0,
    WindowCloseButtonHint = 0,
    MacWindowToolBarButtonHint = 0,
    BypassGraphicsProxyWidget = 0,
    NoDropShadowWindowHint = 0,
    WindowFullscreenButtonHint = 0,
}

---@class Space : Layout
gui.space = {}

---Adds a non-strecthable space with size size to the layout.
---@param size integer size in pixels of the space.
---@return Space
function gui.Space(size) end

---@class Span : Layout
gui.span = {}

---@class SpanOptions
---@field [1] integer The number of columns to span.
---@field [2] Layout The inner layout to span.

---@class SpanOptionsWithRow
---@field [1] integer The number of columns to span.
---@field [2] integer The number of rows to span.
---@field [3] Layout The inner layout.

---@param options SpanOptions|SpanOptionsWithRow
---@return Span
function gui.Span(options) end

---@param col integer The number of columns to span.
---@param layout Layout The inner layout.
---@return Span
function gui.Span(col, layout) end

---@param col integer The number of columns to span.
---@param row integer The number of rows to span.
---@param layout Layout The inner layout.
---@return Span
function gui.Span(col, row, layout) end

---@class Stretch : Layout
gui.stretch = {}

---Adds a stretchable space to the layout.
---@param factor integer The factor by which the space should stretch.
function gui.Stretch(factor) end
return gui
