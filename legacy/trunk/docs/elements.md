# Progress indicators for Doom Legacy subsystems

*(don't take these too seriously!)*

**Progression:** Unmodified | Starting | Partial | Works | Done

| Subsystem | Status | Location |
|---|---|---|
| **Fundamentals** | | |
| Basic data classes | Works | util/ |
| Event system | Works | engine/ |
| Cache | Works | util/ |
| Menu | Partial | engine/menu/ |
| Console | Works | util/ |
| **Game simulation** | | |
| World object class hierarchy | Works | include/g_*.h |
| Object serialization/persistence | Works | engine/g_save.cpp |
| World subdivision (bsp-tree, octree, lists) | Works | engine/ |
| Physics engine | Partial | engine/ |
| AI | Starting | engine/ |
| Scripting | Works | engine/scripting/ |
| Network code | Partial | net/ |
| DLL interface | Starting | |
| Bots | Partial | engine/bots/ |
| **Extra stuff** | | |
| HUD | Works | engine/HUD/ |
| Automap | Partial | engine/automap/ |
| Intermission/Finale | Works | engine/animation/ |
| Cheating | Done | engine/m_cheat.cpp |
| **Video** | | |
| Texture class | Works | include/r_data.h |
| 2D sprite/billboard/decal class | Works | include/r_sprite.h |
| 3D model class | Partial | include/r_sprite.h |
| Software renderer | Works | video/ |
| OpenGL renderer | Partial | video/hardware/ |
| **Audio** | | |
| Sound renderer | Works | audio/ |
| Music | Works | audio/ |
