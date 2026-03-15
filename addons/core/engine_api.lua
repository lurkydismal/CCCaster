---@meta

---@class EngineEvent
---@field player table
---@field damage number
---@field damage_type string
---@field attacker table

---@class Engine
local engine = {}

---Register callback for an event
---@param event string
---@param callback fun(event: EngineEvent)
function engine.register_event(event, callback) end

---Emit event
---@param event string
---@param data table
function engine.emit_event(event, data) end

---Define new event
---@param event string
function engine.define_event(event) end

---Write message to engine log
---@param message string
function engine.log(message) end

---@class DamageParams
---@field target any
---@field amount number
---@field damage_type string

---Apply damage
---@param params DamageParams
function engine.damage(params) end

---@class StatusParams
---@field target any
---@field status string
---@field duration number
---@field intensity number

---Apply status effect
---@param params StatusParams
function engine.apply_status(params) end

return engine
