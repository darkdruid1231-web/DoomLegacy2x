# DeHackEd Support Research for Doom Legacy

## Current Implementation Analysis

Doom Legacy's DeHackEd/BEX parsing is implemented in `legacy/trunk/util/dehacked.cpp` and `legacy/trunk/include/dehacked.h`.

### Supported Features

- **Things**: Full support for monster/item properties including ID, health, sounds, frames, flags, dimensions, etc.
- **Frames**: Sprite, duration, next frame, codepointers.
- **Weapons**: Ammo type, weapon states (deselect, select, bobbing, shooting, firing).
- **Ammo**: Max ammo, per ammo (pickup), per weapon.
- **Misc**: Various game settings like initial/max health, armor values, god mode health, IDFA/IDKFA settings, BFG ammopershoot.
- **Text**: String replacements for texts, music names, sprite names.
- **Cheats**: Modification of cheat codes.
- **BEX [CODEPTR]**: Setting codepointers for frames.
- **BEX [STRINGS]**: String replacements with mnemonics.

### Partially Supported

- **Sound**: Deprecated, redirects to SNDINFO.
- **Pointer**: Codepointer setting (legacy syntax).

### Unsupported/Missing Features

- **ammopershoot for weapons**: Only supported for BFG in Misc section. Other weapons lack ammopershoot setting.
- **Monsters section**: Marked as TODO in Misc parsing.
- **BEX [PARS]**: Unsupported, error message says "currently unsupported".
- **String escaping**: No support for escape sequences like \n in STRINGS.
- **Backslash line continuation**: FIXME comment mentions backslash-continued lines not handled.
- **Other BEX sections**: [TABLES], etc., not implemented.
- **Heretic/Hexen extensions**: Limited support beyond Doom.

## Comparison with Other Ports

### GZDoom
- Full DeHackEd 6 support.
- Complete BEX support including [CODEPTR], [STRINGS], [PARS], [TABLES].
- String escaping and backslash continuations.
- ammopershoot for all weapons.
- Monsters section.
- Advanced features like DECORATE integration.

### Eternity Engine
- Full DeHackEd and BEX support.
- Includes PARS for sprite parsing.
- ammopershoot, Monsters section.
- String handling with escaping.

### PrBoom+
- DeHackEd support with some BEX extensions.
- Basic PARS support.
- ammopershoot partially.
- Less comprehensive than GZDoom/Eternity.

## Missing Features Details

### Monsters Section
Allows modifying monster behavior like infighting, friendliness, etc.

### ammopershoot
Sets ammo consumption per shot for weapons. Currently only BFG has it via Misc.

### PARS
Parses graphics lumps to define sprites from patches, enabling custom sprites.

### Escaping Symbols
In [STRINGS], allow \n, \t, etc., for formatted text.

### Backslash Lines
Allow lines ending with \ to continue to next line.

## Implementation Plan

### Phase 1: ammopershoot for Weapons
- Extend Read_Weapon to parse ammopershoot.
- Pros: Simple, increases compatibility.
- Cons: Minor.

### Phase 2: Monsters Section
- Implement Read_Monsters or extend Misc.
- Pros: Allows more modding.
- Cons: Requires understanding monster AI flags.

### Phase 3: PARS Section
- Add parser for [PARS] to define sprites from lumps.
- Pros: Enables custom graphics mods.
- Cons: Complex, requires lump parsing integration.

### Phase 4: String Improvements
- Add escaping and backslash continuation in Read_STRINGS.
- Pros: Better text modding.
- Cons: Parser modifications.

### Phase 5: Additional BEX Sections
- [TABLES], etc., if needed.
- Pros: Full BEX compliance.
- Cons: May be overkill.

## Pros/Cons of Full Support

### Pros
- **Compatibility**: More DeHackEd patches work out of the box.
- **Modding Community**: Attracts modders, increases user base.
- **Future-Proofing**: Supports existing and new mods.

### Cons
- **Complexity**: Increases code complexity and maintenance burden.
- **Testing**: More features mean more potential bugs.
- **Performance**: Parsing more sections may add load time.
- **Scope Creep**: Endless features if not scoped.

## Recommendation

Implement full DeHackEd/BEX support gradually, starting with ammopershoot and Monsters, then PARS and string improvements. Prioritize based on community demand and existing mod compatibility issues. Avoid over-engineering; focus on core Doom modding features. Estimate effort: Phase 1-2: 1-2 weeks, Phase 3: 2-4 weeks, Phase 4: 1 week.