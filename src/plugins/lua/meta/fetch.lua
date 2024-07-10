---@meta Fetch
local Fetch = {}

---A network reply from fetch.
---@class QNetworkReply
---@field error integer The error code of the reply or 0 if no error.
local QNetworkReply = {}

---Returns the data of the reply.
---@return string
function QNetworkReply:readAll() end

---Fetches a url. Call `a.wait` on the returned value to get the result.
---@param options FetchOptions
---@return table|QNetworkReply|string
function Fetch.fetch(options) end

--@param options FetchOptions
--@param callback function The callback to call when the fetch is done.
function Fetch.fetch_cb(options, callback) end

---@class FetchOptions
---@field url string The url to fetch.
---@field method? string The method to use (GET, POST, ...), default is GET.
---@field headers? table The headers to send.
---@field body? string The body to send.
---@field convertToTable? boolean If true, the resulting data will expect JSON and converted it to a table.
local FetchOptions = {}

return Fetch
