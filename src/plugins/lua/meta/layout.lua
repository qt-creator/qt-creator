---@meta Layout

local layout = {}

---The base class of all layout classes
---@class Object
layout.Layout = {}

---The base class of all layout classes
---@class Layout : Object
layout.Layout = {}

---The base class of all widget classes, an empty widget itself.
---@class Widget : Object
Widget = {}

---@param children Layout
---@return Widget
function layout.Widget(children) end

---Column layout
---@class Column : Layout
local column = {}

---@param children Layout|string|BaseAspect|function
---@return Column
function layout.Column(children) end

---A group box with a title
---@class Group : Widget
local group = {}

---@return Group
function layout.Group(children) end

---Row layout
---@class Row : Layout
local row = {}

---@param children Layout|string|BaseAspect|function
---@return Row
function layout.Row(children) end

---Flow layout
---@class Flow : Layout
local flow = {}

---@param children Layout|string|BaseAspect|function
---@return Flow
function layout.Flow(children) end

---Grid layout
---@class Grid : Layout
local grid = {}

---@param children Layout|string|BaseAspect|function
---@return Grid
function layout.Grid(children) end

---Form layout
---@class Form : Layout
local form = {}

---@param children Layout|string|BaseAspect|function
---@return Form
function layout.Form(children) end


---A stack of multiple widgets
---@class Stack : Widget
local stack = {}

---@param children Layout|string|BaseAspect|function
---@return Stack
function layout.Stack(children) end

---A Tab widget
---@class Tab : Widget
local tab = {}

---@param children Layout|string|BaseAspect|function
---@return Tab
function layout.Tab(children) end

---A Multiline text edit
---@class TextEdit : Widget
local textEdit = {}

---@param children Layout|string|BaseAspect|function
---@return TextEdit
function layout.TextEdit(children) end

---A PushButton
---@class PushButton : Widget
local pushButton = {}

---@param children Layout|string|BaseAspect|function
---@return PushButton
function layout.PushButton(children) end

---A Label
---@class Label : LayoutItem
local label = {}

---@param children LayoutItem|string|BaseAspect|function
---@return Label
function layout.Label(children) end

---A SpinBox
---@class SpinBox : Widget
local spinBox = {}

---@param children Layout|string|BaseAspect|function
---@return SpinBox
function layout.SpinBox(children) end

---A Splitter
---@class Splitter : Widget
local splitter = {}

---@param children Layout|string|BaseAspect|function
---@return Splitter
function layout.Splitter(children) end

---A Toolbar
---@class ToolBar : Widget
local toolBar = {}

---@param children Layout|string|BaseAspect|function
---@return ToolBar
function layout.ToolBar(children) end

---A TabWidget
---@class TabWidget : Widget
local tabWidget = {}

---@param children Layout|string|BaseAspect|function
---@return TabWidget
function layout.TabWidget(children) end

---@param name string
---@param child Layout|string|BaseAspect|function
---@return TabWidget
function layout.TabWidget(name, child) end
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

---Sets the alignment of the layout to "Form"
function layout.withFormAlignment() end

---Sets the size of the parent object if possible
function layout.resize(width, height) end

---Sets the spacing of the layout
function layout.spacing(spacing) end

---Sets the field growth policy of the layout
function layout.fieldGrowthPolicy(policy) end

---Sets the onClicked handler of the parent object if possible
function layout.onClicked(f) end

---Sets the onTextChanged handler of the parent object if possible
function layout.onTextChanged(f) end

return layout
