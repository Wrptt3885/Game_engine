-- example.lua — basic rotation script
-- Attach this to any GameObject with a MeshRenderer

local speed = 90.0   -- degrees per second

function Awake()
    Log("Awake: " .. GameObject.GetName())
end

function Update(dt)
    local x, y, z = Transform.GetEulerAngles()
    Transform.SetEulerAngles(x, y + speed * dt, z)
end

function OnCollisionEnter(otherName)
    Log("Collision with: " .. otherName)
end
