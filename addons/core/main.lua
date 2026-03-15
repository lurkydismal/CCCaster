-- main.lua

local mod = {}

---@class Engine
engine = {}

-- event = {
--     player = player_object,
--     damage = 25,
--     damage_type = "slash",
--     attacker = enemy_object
-- }

-- Minimal engine-side API expected by the script:
-- engine.register_event(event_name, function)
-- engine.emit_event(event_name, data)
-- engine.define_event(event_name)
-- engine.log(message)
-- engine.damage(data)
-- engine.apply_status(data)

-- Called when the mod is loaded
function mod.on_init(
	_ --[[ event ]]
)
	engine.log("BetterBleeding initialized")
end

-- Called when a player takes damage
function mod.on_player_damage(event)
	local damage = event.damage
	local player = event.player

	if event.damage_type == "slash" then
		engine.log("Applying bleed effect")

		engine.apply_status({
			target = player,
			status = "bleeding",
			duration = 5,
			intensity = damage * 0.2,
		})
	end
end

-- Called when player spawns
function mod.on_player_spawn(event)
	engine.log("Player spawned: " .. event.player.name)
end

-- Example custom event from this mod
function mod.on_bleed_tick(event)
	local entity = event.entity

	engine.damage({
		target = entity,
		amount = 1,
		damage_type = "bleed",
	})
end

-- Register callbacks
engine.register_event("engine:init", mod.on_init)
engine.register_event("player:damage", mod.on_player_damage)
engine.register_event("player:spawn", mod.on_player_spawn)

-- Define custom event namespace
engine.define_event("better_bleeding:bleed_tick")

engine.register_event("better_bleeding:bleed_tick", mod.on_bleed_tick)

return mod
