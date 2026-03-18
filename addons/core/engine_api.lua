---@meta

---@class Engine
Engine = {}

---Write info message to engine log
---@param message string
function Engine.log_info(message) end

---Write warning message to engine log
---@param message string
function Engine.log_warn(message) end

---Write error message to engine log
---@param message string
function Engine.log_error(message) end

---@type integer
Engine.api_version = 0

---@type string
Engine.id = ""

return Engine