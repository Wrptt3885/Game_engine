#pragma once

struct PhysicsMaterial {
    enum class Phase { Solid, Liquid, Gas };

    // ---- Mechanical ----------------------------------------------------------
    float density     = 1000.0f; // kg/m³  (น้ำ=1000, เหล็ก=7874, ไม้=600)
    float friction    = 0.5f;    // 0–1
    float restitution = 0.0f;    // 0=ดูดซับ, 1=กระดอน
    float hardness    = 1.0f;    // 0=นิ่ม (soft body), 1=แข็ง (rigid)

    // ---- Optical (Renderer ใช้) ----------------------------------------------
    float ior          = 1.0f;   // Index of Refraction: อากาศ=1.0, น้ำ=1.33, กระจก=1.5
    float transparency = 0.0f;   // 0=ทึบ, 1=ใส
    float absorbance   = 0.0f;   // แสงดูดซึมใน volume (ทำให้สีเปลี่ยนตามความลึก)

    // ---- Phase ---------------------------------------------------------------
    Phase phase = Phase::Solid;  // บอก PhysicsWorld ว่าใช้ simulation system ไหน

    // ---- Presets -------------------------------------------------------------
    static PhysicsMaterial Default() { return {}; }

    static PhysicsMaterial Metal() {
        PhysicsMaterial m;
        m.density = 7874.0f; m.friction = 0.4f; m.restitution = 0.1f;
        m.hardness = 1.0f;
        return m;
    }
    static PhysicsMaterial Wood() {
        PhysicsMaterial m;
        m.density = 600.0f; m.friction = 0.7f; m.restitution = 0.2f;
        return m;
    }
    static PhysicsMaterial Rubber() {
        PhysicsMaterial m;
        m.density = 1200.0f; m.friction = 0.9f; m.restitution = 0.8f;
        m.hardness = 0.3f;
        return m;
    }
    static PhysicsMaterial Ice() {
        PhysicsMaterial m;
        m.density = 917.0f; m.friction = 0.05f; m.restitution = 0.1f;
        m.ior = 1.31f; m.transparency = 0.7f;
        return m;
    }
    static PhysicsMaterial Glass() {
        PhysicsMaterial m;
        m.density = 2500.0f; m.friction = 0.4f; m.restitution = 0.05f;
        m.ior = 1.52f; m.transparency = 0.95f; m.absorbance = 0.02f;
        return m;
    }
    static PhysicsMaterial Water() {
        PhysicsMaterial m;
        m.density = 1000.0f; m.friction = 0.01f; m.restitution = 0.0f;
        m.ior = 1.33f; m.transparency = 0.85f; m.absorbance = 0.1f;
        m.phase = Phase::Liquid;
        return m;
    }
    static PhysicsMaterial Stone() {
        PhysicsMaterial m;
        m.density = 2700.0f; m.friction = 0.8f; m.restitution = 0.1f;
        return m;
    }
};
