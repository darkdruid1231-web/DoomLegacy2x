# Menu Option Audit (2026-03-16)

This is a point-in-time audit of sub-menu items in Doom Legacy, focused on whether each option is wired to gameplay or rendering behavior and whether it produces a visible effect.

Legend: Working, Fixed, Partial, Disabled.

## Video Options
- Display Mode (cv_fullscreen): Working
- Aspect Ratio (cv_aspectratio): Working (filters SDL2 mode list)
- VSync (cv_vsync): Working (SDL2 swap interval)
- FPS Limit (cv_fpslimit): Working (frame limiter in the game loop)
- Brightness (cv_video_gamma): Fixed (now maps to OpenGL gamma when not in software)
- Screen size (cv_viewsize): Working
- Scale status bar (cv_scalestatusbar): Working
- Translucency (cv_translucency): Working
- Splats (cv_splats): Working
- Bloodtime (cv_bloodtime): Working
- Screenslink effect (cv_screenslink): Disabled in OpenGL (software-only), Working in software
- Quality Settings / Advanced GL: Working

## Quality Settings
- Quality Preset (cv_glquality): Working
- Anti-Aliasing (cv_msaa): Working (restart required)
- Texture Filter (cv_grfiltermode): Working
- Anisotropic Filter (cv_granisotropy): Working
- Dynamic Lighting (cv_grdynamiclighting): Working
- Shadows (cv_grshadows): Working
- Coronas (cv_grcoronas): Working
- Field of View (cv_fov): Working

## OpenGL Options
- Field of view (cv_fov): Working
- Quality/BPP (cv_scr_depth): Working (GL attributes set at context creation)
- Texture Filter / Anisotropy: Working
- Lighting / Fog / Gamma / Normal maps: Working

## Sound
- Sound Volume (cv_soundvolume): Working
- Music Volume (cv_musicvolume): Working
- CD Volume (cd_volume): Partial (device dependent)

## Input Settings / Mouse Options
- Use Mouse, Always MouseLook, Mouse Move, Invert Mouse, Mouse X/Y speed: Working

## Controls
- Key bindings: Working

## Gameplay (Game Options)
- Item respawn, respawn time, monster respawn, fast monsters, gravity, jumpspeed, rocket jump, solid corpses, voodoo dolls: Working

## Network / Server / Player Setup
- Server options: Working
- Network options: Working (server-side only)
- Player setup: Working

## Notes
- Disabled items are shown grayed in the legacy menu and with a reason tooltip in the modern sub-menu UI.
