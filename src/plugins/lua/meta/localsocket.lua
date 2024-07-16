---@meta LocalSocket

---@class LocalSocket
LocalSocket = {}


---Creates a new Socket to the given socketFileName
---@param socketFileName string The name of the socket file.
---@return LocalSocket localSocket The created socket.
function LocalSocket.create(socketFileName) end

---Returns true if the socket is connected.
---@return boolean isConnected True if the socket is connected.
function LocalSocket:isConnected() end

---@async
---Connects the socket to the server.
---@return boolean success True if the connection was successful.
---@return string errorString The error string if the connection failed.
function LocalSocket:connectToServer() end

---Connects the socket to the server
---@param callback function The callback function. The callback function should take two arguments: success (boolean) and errorString (string).
function LocalSocket:connectToServer_cb(callback) end

---Sends the data to the server.
---@param data string The data to send.
---@return integer bytesWritten The number of bytes written.
function LocalSocket:write(data) end

---@async
---Reads the data from the server. Waits until data is available.
---@return string data The data read from the server.
function LocalSocket:read() end

---Reads the data from the server. Calls "callback" once data is available.
---@param callback function The callback function. The callback function should take one argument: data (string).
function LocalSocket:read_cb(callback) end
---Closes the socket.
function LocalSocket:close() end

return LocalSocket
