# Small Timer 0.2.2 — sound engine and waveform validation

Date: November 17, 2026

Environment: Apple silicon, macOS 26.6.2, Apple Clang 21, JUCE 8.0.10.
Release bundles target arm64 and macOS 12 or newer.

## Latest interface regression pass

The current top-symbol build passes 3,728 checks when generating the preview and
loading both AU/VST3 wrappers (3,727 without preview generation). Coverage includes
display freeze/resume with ongoing audio, shared VIEW gain, unchanged saved state,
variation lamps, Wander status lighting, fader latches, transport symbols and the
NEW SEED spark/ripple. Both plugin formats load, process audio, save/restore state
and open their native editors in the test host. Earlier sections retain the
historical sound-engine and waveform validation details.

## Automated behavior checks

- Loop and Little Forever retain independent fader banks.
- Motor recall advances gradually and settles at the stored target.
- Wander stays within 0–1, respects locks and yields to manual dragging.
- Drag override works while held; Wander waits after release.
- Recall and mutation respect locks; recall snapshots survive state restoration.
- Both banks, switches, lock states and mutation depth survive state restoration.
- Invalid state data is ignored.
- MIX zero passes stereo input through exactly; RAM recording and playback work.
- Recording stops correctly at the 30-second capacity boundary.
- A fresh processor restores parameters with an empty recording and stopped transport.
- The editor creates at 660 × 750; mode, Motion, Hold and fader callbacks are connected.

## Sound engine checks

- Impulse tests verify TIME's 40 ms minimum and its exponential delay mapping at
  44.1, 48 and 96 kHz.
- FEED creates extra echoes; Little Forever retains more late energy than Loop.
- SPACE spreads a left-only impulse into a stereo diffusion tail.
- DRIFT and GRAIN independently change a multitone input's rendered output.
- SOFT FAILURE changes the feedback/saturation character.
- Full wet output remains finite and bounded with maximum feedback, space,
  drift, grain and hot input in both modes.
- Large parameter jumps are smoothed; resetting the engine clears its history.
- Wander advances without an editor, leaves saved targets unchanged, respects
  locks and HOLD, and yields to manual overrides.
- Audio output is identical across fixed 256-sample blocks and a repeating
  sequence of 1, 17, 511 and 1024 samples, including audible Wander.
- Mono dry audio works. Recording and effect tails leave the state blob exactly
  unchanged, and a fresh restored processor produces silence with no input.

## Stacked display update

Version 0.2.2 replaces the overlaid monitor with separate gray IN and amber OUT
lanes. The monitor is slightly taller, with a horizontal divider, independent
baselines and identical time/amplitude scales. Matched signals stay separately
visible. DSP, source selection, waveform capture and parameter IDs are unchanged.
The AU/VST3 regression suite passed 3,686 checks including the native preview.
The rendered panel was inspected, then the user's current setup was saved and
Unfold restarted with permission to clear temporary audio. The saved piece was
reloaded and the running Small Timer editor was visually confirmed to show both
separate lanes. Previous bundles were retained by the staged installer.

## Bundle checks

The 0.2.1 overlay tests verify chronological ordering after the monitor wraps,
clearing both traces on reset, exact input/output overlap at MIX zero, the RAM
playback source and distinct wet output. Source/output floats are published as
one lock-free atomic pair and read with a bounded revision check. Monitoring
does not allocate or wait on the audio thread. The two traces share a fixed
scale and timeline and monitor the left channel.

The user requested that this update be installed without restarting the active
Unfold session. Its existing instance therefore keeps 0.2.0 until the next
reload; the 0.2.1 editor is validated through native rendering and test hosts.
The 0.2.1 AU/VST3 regression run passed 3,686 checks including preview rendering.
The rendered overlay was visually inspected for trace contrast, shared alignment
and a readable IN/OUT legend. Both installed plugin formats and the standalone
were updated by retaining old bundles and swapping in staged, verified bundles.

The built AU and VST3 were loaded into a JUCE test host. Each was scanned,
instantiated, prepared at 48 kHz with 256-sample blocks, processed stereo input,
saved/restored state, created its custom editor and released resources. The 0.2.0
tests also automate MIX from dry to wet and verify an actual delayed impulse
through each wrapper. Plugin version checks prevent accidentally testing an
older installed AU. Stereo buses are explicitly enabled before processing.
The AU test dynamically registers the bundle in the test process without installation.

Unfold integration exposed a symbol collision when creating the editor in a host
that exports JUCE symbols. The build now hides symbols in C++, C and Objective-C++
for shared code and every wrapper. The regression host intentionally retains its
default symbol visibility and verifies editor construction for both formats.
The bundle tests explicitly activate and verify stereo buses before processing.

Unfold 0.9.2's isolated VST3 scanner detected version 0.2.0. Apple auval reported
AU VALIDATION SUCCEEDED for the installed Audio Unit. The final AU/VST3 host
regression run passed 3,680 checks, including actual wet echoes through each wrapper.

With user approval to clear the temporary recording, the current Deep Dust piece
was saved, Unfold restarted, and that piece reloaded with its Pigments instrument
and Small Timer in track effect slot 1. The updated editor opened and displayed
the new DSP tooltips. A playback check through Pigments and Small Timer produced
nonzero audio with zero reported dropouts, nonfinite samples or over-range samples
in the inspected diagnostic windows. Small Timer took about 0.21 ms per 512-frame
block in those windows. Playback was stopped and the plugin editor left open.

All three bundles are locally ad-hoc signed. The build explicitly signs complete
bundles after resources are generated to avoid invalid linker-created signatures.

## Visual inspection

A 2× native JUCE editor snapshot was rendered and inspected. It includes charcoal
panel texture, walnut rails, six metallic faders, all requested buttons, the Mutate
knob, a waveform from generated test audio and lock indicators. The earlier chat's
image attachment was unavailable; the supplied written design guided this panel.

## Limits

No hardware audio device listening test, Intel build,
older macOS installation or notarization was tested. The standalone app was built;
its shared processor/editor were tested through the automated harness.
The sound engine is a first implementation, not a measured hardware emulation.
Moving TIME intentionally glides pitch. Little Forever decays rather than
sustaining indefinitely. RAM playback has no seam crossfade or tempo sync.
The dry path preserves input level; wet limiting does not limit the dry source.
Audio is excluded from plugin state and never written to disk by the plugin.
