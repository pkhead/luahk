local scheduled = {}
local deferred = {}

local function spawn(f, ...)
	coroutine.resume(coroutine.create(f), ...)
end

local function defer(f, ...)
	table.insert(deferred, { coroutine.create(f), ... })
end

local function wait(ms)
	table.insert(scheduled, { coroutine.running(), os.tick() + ms, os.tick() })
	return coroutine.yield()
end

os.update(function()
	local tick = os.tick()

	for i=#scheduled, 1, -1 do
		local v = scheduled[i]

		if tick >= v[2] then
			coroutine.resume(v[1], tick - v[3])
			table.remove(scheduled, i)
		end
	end

	for _, coro in ipairs(deferred) do
		coroutine.resume(table.unpack(coro))
	end

	deferred = {}
end)

return {
	wait = wait,
	spawn = spawn,
}