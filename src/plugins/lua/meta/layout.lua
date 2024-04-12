---@meta Layout

local layout = {}

---The base class of all layout items
---@class LayoutItem
layout.LayoutItem = {
    ---Attaches the layout to the specified widget
    ---@param widget QWidget
    attachTo = function(widget) end
}

---Column layout
---@class Column : LayoutItem
local column = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Column
function layout.Column(children) end

---A group box with a title
---@class Group : LayoutItem
local group = {}

---@return Group
function layout.Group(children) end

---Row layout
---@class Row : LayoutItem
local row = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Row
function layout.Row(children) end

---Flow layout
---@class Flow : LayoutItem
local flow = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Flow
function layout.Flow(children) end

---Grid layout
---@class Grid : LayoutItem
local grid = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Grid
function layout.Grid(children) end

---Form layout
---@class Form : LayoutItem
local form = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Form
function layout.Form(children) end

---An empty widget
---@class Widget : LayoutItem
local widget = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Widget
function layout.Widget(children) end

---A stack of multiple widgets
---@class Stack : LayoutItem
local stack = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Stack
function layout.Stack(children) end

---A Tab widget
---@class Tab : LayoutItem
local tab = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Tab
function layout.Tab(children) end

---A Multiline text edit
---@class TextEdit : LayoutItem
local textEdit = {}

---@param children LayoutItem|string|BaseAspect|function
---@return TextEdit
function layout.TextEdit(children) end

---A PushButton
---@class PushButton : LayoutItem
local pushButton = {}

---@param children LayoutItem|string|BaseAspect|function
---@return PushButton
function layout.PushButton(children) end

---A SpinBox
---@class SpinBox : LayoutItem
local spinBox = {}

---@param children LayoutItem|string|BaseAspect|function
---@return SpinBox
function layout.SpinBox(children) end

---A Splitter
---@class Splitter : LayoutItem
local splitter = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Splitter
function layout.Splitter(children) end

---A Toolbar
---@class ToolBar : LayoutItem
local toolBar = {}

---@param children LayoutItem|string|BaseAspect|function
---@return ToolBar
function layout.ToolBar(children) end

---A TabWidget
---@class TabWidget : LayoutItem
local tabWidget = {}

---@param children LayoutItem|string|BaseAspect|function
---@return TabWidget
function layout.TabWidget(children) end

---A "Line break" in the layout
function layout.br() end

---A "Stretch" in the layout
function layout.st() end

---An empty space in the layout
function layout.empty() end

---A horizontal line in the layout
function layout.hr() end

---Clears the margin of the layout
function layout.noMargin() end

---Sets the margin of the layout to the default value
function layout.normalMargin() end

---Sets the margin of the layout to a custom value
function layout.customMargin(left, top, right, bottom) end

---Sets the alignment of the layout to "Form"
function layout.withFormAlignment() end

---Sets the title of the parent object if possible
function layout.title(text) end

---Sets the text of the parent object if possible
function layout.text(text) end

---Sets the tooltip of the parent object if possible
function layout.tooltip(text) end

---Sets the size of the parent object if possible
function layout.resize(width, height) end

---Sets the stretch of the column at `index`
function layout.columnStretch(index, stretch) end

---Sets the spacing of the layout
function layout.spacing(spacing) end

---Sets the window title of the parent object if possible
function layout.windowTitle(text) end

---Sets the field growth policy of the layout
function layout.fieldGrowthPolicy(policy) end

---Sets the onClicked handler of the parent object if possible
function layout.onClicked(f) end

---Sets the onTextChanged handler of the parent object if possible
function layout.onTextChanged(f) end

return layout
