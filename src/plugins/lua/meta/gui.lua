---@meta Gui

local gui = {}

---The base class of all ui related classes.
---@class Object
gui.Object = {}

---The base class of all gui layout classes.
---@class Layout : Object
gui.Layout = {}

---The base class of all widget classes, an empty widget itself.
---@class Widget : Object
gui.Widget = {}

---@param children Layout
---@return Widget
function gui.Widget(children) end

---Column layout
---@class Column : Layout
local column = {}

---@param children Layout|string|BaseAspect|function
---@return Column
function gui.Column(children) end

---A group box with a title.
---@class Group : Widget
local group = {}

---@return Group
function gui.Group(children) end

---Row layout.
---@class Row : Layout
local row = {}

---@param children Layout|string|BaseAspect|function
---@return Row
function gui.Row(children) end

---Flow layout.
---@class Flow : Layout
local flow = {}

---@param children Layout|string|BaseAspect|function
---@return Flow
function gui.Flow(children) end

---Grid layout.
---@class Grid : Layout
local grid = {}

---@param children Layout|string|BaseAspect|function
---@return Grid
function gui.Grid(children) end

---Form layout.
---@class Form : Layout
local form = {}

---@param children Layout|string|BaseAspect|function
---@return Form
function gui.Form(children) end


---A stack of multiple widgets.
---@class Stack : Widget
local stack = {}

---@param children Layout|string|BaseAspect|function
---@return Stack
function gui.Stack(children) end

---A Tab widget.
---@class Tab : Widget
local tab = {}

---@param children Layout|string|BaseAspect|function
---@return Tab
function gui.Tab(children) end

---A Multiline text edit.
---@class TextEdit : Widget
local textEdit = {}

---@param children Layout|string|BaseAspect|function
---@return TextEdit
function gui.TextEdit(children) end

---@class PushButton : Widget
local pushButton = {}

---@param children Layout|string|BaseAspect|function
---@return PushButton
function gui.PushButton(children) end

---@class Label : LayoutItem
local label = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Label
function gui.Label(children) end

---@class SpinBox : Widget
local spinBox = {}

---@param children Layout|string|BaseAspect|function
---@return SpinBox
function gui.SpinBox(children) end

---@class Splitter : Widget
local splitter = {}

---@param children Layout|string|BaseAspect|function
---@return Splitter
function gui.Splitter(children) end

---@class ToolBar : Widget
local toolBar = {}

---@param children Layout|string|BaseAspect|function
---@return ToolBar
function gui.ToolBar(children) end

---@class TabWidget : Widget
local tabWidget = {}

---@param children Layout|string|BaseAspect|function
---@return TabWidget
function gui.TabWidget(children) end

---@param name string
---@param child Layout|string|BaseAspect|function
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

---Sets the size of the parent object if possible.
function gui.resize(width, height) end

---Sets the spacing of the gui.
function gui.spacing(spacing) end

---Sets the field growth policy of the gui.
function gui.fieldGrowthPolicy(policy) end

---Sets the onClicked handler of the parent object if possible.
function gui.onClicked(f) end

---Sets the onTextChanged handler of the parent object if possible.
function gui.onTextChanged(f) end

return gui
