# Reference-panel visual update

Matched the supplied original reference more closely: stacked italic logo, compact top-row square switches, wide inset display, six coloured fader markers, champagne-metal crosshatched fader caps, charcoal panel grain, vertical transport buttons and a larger brushed-metal MUTATE dial below the faders.

The separate input/output waveform lanes remain, as requested. Fader locks, readouts and NEW SEED remain functional. No decorative LENGTH, SPEED or MIX ENV knobs were added because these are not implemented audio parameters. Sound, state format and temporary recording behaviour are unchanged.

This is a visual revision of 0.2.2. Installation retains the previous bundles and does not restart the current host session.

Second refinement: heavier italic logo, condensed hardware legends, warmer sculpted walnut rails, metallic cross-head screws, subtler charcoal grain, polished radial-reflection MUTATE face with knurled rim, and quieter lock controls. Dial and panel textures are cached rather than regenerated during animation. Build, editor preview and all 3665 core checks pass.

Hardware-finish pass: fine high-resolution matte panel grain replaces the horizontal striping; narrower regular-weight legends, denser double-knurled fader caps with a bright centre ridge, softer layered shadows, inset display bezel, brighter switch lamp bloom, and a smaller MUTATE dial bring the finish and proportions closer to the original image. Separate IN/OUT lanes and all existing controls are retained.

Switch-lighting pass: softer champagne bevels, recessed dark housings, translucent amber faces, cream-hot lamp cores and warm light spill onto the surrounding panel. Panel-level spill avoids clipping the glow at button bounds. Glow fades retain their warm RGB hue at zero alpha rather than fading through black. Added rendered-image checks for lamp and face brightness; audio and controls are unchanged.

Signal-lighting pass: pale green input and amber output traces now use layered phosphor bloom with narrow luminous cores. Each halo is clipped to its own lane so the signals remain separate and labels stay clear. The waveform data, amplitude scales and audio processing are unchanged. Render checks cover both halos and core contrast.

Lower-right redesign: a compact VARIATION section with an eight-lamp amber depth indicator, percentage readout, explicit Wander status and a square metal NEW SEED switch. The lamp row follows the existing MUTATE amount; no additional audio parameters or state are introduced. The square seed switch uses the same hardware rendering as the transport controls.

Wander status lighting: active lettering and a recessed amber indicator breathe gently over 3.6 seconds. HOLD replaces the active indication with steady, subdued MOTION HELD lettering and lamp; OFF stays dark. Animation is editor-only and follows the existing repaint timer; it does not alter audio, motion parameters or persistence. Render tests cover active, held and off lamp states.

Renamed the top MOTION switch to WANDER. The existing motion parameter ID, automation, saved settings and behaviour are unchanged; the UI interaction test now locates the WANDER label.

Fader-lock redesign: recessed miniature latch switches replace the FREE/LOCK lettering. A champagne thumb with an open-padlock icon sits left when unlocked; an amber thumb with a closed-padlock icon sits right when locked. Hover highlights, click feedback, descriptive accessible titles/state and the existing tooltips preserve usability. All six existing lock parameters and their persistence/behaviour are unchanged.

Waveform-detail update: narrower bloom and sharp luminous cores preserve fine detail. A small VIEW control cycles shared x1/x2/x4/x8 display magnification, defaulting to x2 on editor open. Both signals always use the same scale. Magnification is editor-only, is not persisted or automated, never affects audio, and can flatten peaks at the visual lane limits without clipping the sound.

Display FREEZE: a latched button beside VIEW holds the last displayed IN/OUT frame pair while audio and history capture continue. VIEW can magnify the frozen pair. Unfreezing jumps to the latest live history rather than replaying a backlog. Freeze is editor-local, defaults off when reopened, and does not change audio, recording, Wander or saved state.

Transport identities: HOLD has an amber pause symbol, REC a recessed red lamp with a gentle two-second recording pulse, PLAY a glowing green triangle, and RECALL a return arrow with a 450 ms amber recall flash. Shift-click still stores without a recall flash. All lighting is editor-only; audio, transport commands, recall semantics and saved parameters are unchanged. Render regression checks compare all four active/inactive symbols.

NEW SEED spark: engraved branching symbol with an amber burst that eases back to metal over 900 ms. The eight variation lamps briefly ripple from the centre outward, then resume the depth indication. Repeated clicks restart the animation and each invokes the existing mutation operation once. The light effect adds no parameters or saved audio, and does not change lock or mutation semantics.

Top-row symbols: LOOP repeat arrows, LITTLE FOREVER infinity, WANDER winding path and SOFT FAILURE fractured waveform replace the generic lamp dots. Legends stay above their switches. Active symbols glow amber; WANDER breathes over 3.6 seconds while the other top lights remain steady. All parameter IDs, actions and persistence remain unchanged.

0.3.0 musical mutation: NEW SEED now selects a coordinated Cloud, Fragments or Bloom gesture instead of six independent random offsets. Depth scales a relative gesture with small related variations, restrained MIX travel and gentler positive feedback in Little Forever. The selected gesture is named under VARIATION. UNDO SEED restores the immediately previous mutation in the current bank, skipping currently locked faders and values edited since the mutation. Each bank has one transient undo record; no-ops preserve it, recall invalidates it and state loading clears both records. There are no new host parameters, audio-thread allocations or saved audio. WANDER and the existing audio engine are unchanged.

0.3.1 mutation-bias correction: the 0.3.0 relative gestures had positive average steps on several faders and clipped excess travel at the endpoints. Repeated seeds therefore accumulated at high settings. Recipe steps now sum to zero for each fader; Forever scales both feedback directions equally. Overshoots reflect inward, so a nominally upward gesture may move down near the top (and vice versa near the bottom). No automatic recentering or hidden preset loading is introduced. Tests cover 21,000 consecutive seeds at two depths in both modes, endpoint escape, range occupancy and all existing undo/lock behaviour.

Mode grouping: LOOP and LITTLE FOREVER now share a subtle outlined MODE enclosure and an explicit radio group. A wider gap separates WANDER and SOFT FAILURE, whose tooltips identify them as independent switches usable together in either mode. Mode banks, parameters, audio and persistence are unchanged.
