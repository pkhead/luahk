local task = require("task")

os.throttle(false)

task.spawn(function()
	local i = 1
	
	while true do
		print(i)
		i=i+1
		task.wait(1000)
	end
end)