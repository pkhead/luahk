--[[
local task = require("task")

local function write(text)
	os.setClipboard(text)
	os.send("lctrl down", "v down", "v up", "lctrl up")
end

os.hotkey("numpad7", function()
	print("activated")
	write("In sagittis vel est id facilisis. Praesent id mi in massa tempus blandit. Curabitur euismod aliquet bibendum. Duis at mauris vitae urna convallis venenatis. Donec lobortis sem non ultricies varius. Integer ac mi at mauris ultrices scelerisque ac suscipit felis. Nam accumsan nulla velit, a imperdiet mi scelerisque eu. Cras in turpis semper, mattis est vel, placerat metus. Aenean venenatis diam felis, vel convallis sapien tristique eget. Suspendisse sapien leo, laoreet et eros ut, lacinia varius ex. Duis erat libero, tempor et elit ac, egestas tempor augue. Integer eu condimentum nisi. Curabitur consequat congue lobortis. Fusce elementum euismod magna, sit amet fermentum purus scelerisque in.")
end)
--]]

--[[
os.hotkey("mousex1", function()
	print("activated")
end)
--]]

--local task = require("task")

os.hotkey("numpad7", function()
	local name = os.prompt()
	print("AAA")
	
	if name then
		os.message("Hello, " .. name)
	end
	--os.sendtext("In sagittis vel est id facilisis. Praesent id mi in massa tempus blandit. Curabitur euismod aliquet bibendum. Duis at mauris vitae urna convallis venenatis. Donec lobortis sem non ultricies varius. Integer ac mi at mauris ultrices scelerisque ac suscipit felis. Nam accumsan nulla velit, a imperdiet mi scelerisque eu. Cras in turpis semper, mattis est vel, placerat metus. Aenean venenatis diam felis, vel convallis sapien tristique eget. Suspendisse sapien leo, laoreet et eros ut, lacinia varius ex. Duis erat libero, tempor et elit ac, egestas tempor augue. Integer eu condimentum nisi. Curabitur consequat congue lobortis. Fusce elementum euismod magna, sit amet fermentum purus scelerisque in.")
end)