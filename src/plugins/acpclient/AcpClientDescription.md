Chat with AI agents that understand your codebase and can perform actions on
your behalf, such as editing files, running commands, or triggering builds.

## Start your first AI chat session

### 1. Activate the extension

Activate the **ACP Client** extension. Optionally, also activate **MCP Server**
if you want the AI agent to trigger Qt Creator actions such as building or
running your project.

### 2. Configure an agent

Go to **Preferences > AI > ACP Servers** and select **Add**.

- To use a preconfigured agent, pick a template from the **Template** drop-down
  and select **Apply**.
- To set up a custom agent, select **Custom**, fill in a **Name**, the
  **Launch command**, and any **Launch arguments**, and then select **Apply**.

### 3. Open the chat window

In **Edit** mode, open the ACP chat window by going to
**Tools > ACP Client > Show Agentic AI Chat in Side Panel**, or by selecting
the chat icon in the editor toolbar.

### 4. Connect to a server

In the chat window, select your configured agent from the **Server** drop-down,
verify the **Working directory**, and select **Connect**. Qt Creator starts the
agent process and establishes the connection.

### 5. Start chatting

Type your message in the input field and press **Enter** or select **Send**.
Qt Creator automatically attaches the current editor context, so you can use
natural references like:

- *Explain the current code.*
- *Refactor the selected block to use a range-based for loop.*
- *Build the project and fix any compiler errors.* (Requires **MCP Server**.)

To add extra context manually, select **+ > Add file** in the **Context** area.
If the agent proposes file edits, review and approve them directly in the chat
window.
