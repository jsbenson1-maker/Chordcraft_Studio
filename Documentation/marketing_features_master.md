# Chordcraft Studio - Application Features Master

Chordcraft Studio is a professional-grade, cross-platform chord progression design suite and real-time arranger. It bridges music theory, real-time synthesiser performance, and AI-driven composition advice into a unified tool for songwriters, producers, and educators.

---

## 1. Core Architecture & Audio Engine

* **Deterministic Real-Time Audio Thread**:
  * Employs a lock-free, heap-allocation-free audio rendering thread running via Oboe (Android) or WASAPI/ASIO (Windows).
  * Lock-free communication is handled via Single-Producer Single-Consumer (SPSC) queues (`juce::AbstractFifo`) preventing glitches and dropouts.
* **Lock-Free Double-Buffered Sequencer Updates**:
  * Implements a double-buffered staging and active event pointer swapping mechanism.
  * The UI thread compiles chord progression edits into a staging buffer under a staging mutex, then posts a single swap instruction to the audio thread.
  * The audio thread swaps active and staging pointers at the top of the block using a non-blocking `juce::ScopedTryLock` on the staging mutex. If the lock is busy, the swap instruction is re-queued to retry on the next audio block, keeping the audio thread entirely lock-free.
* **Master Soft-Clipping (Distortion Protection)**:
  * Applies a high-fidelity soft-clipper waveshaper (`std::tanh`) in `processBlock()` and the WAV exporter.
  * Prevents digital clipping and harsh digital distortion when multiple polyphonic layers, patterns, and drum instruments play simultaneously.
* **Natural Note Envelope Decays (ADSR Release Triggers)**:
  * Triggers the TinySoundFont channel release function (`tsf_channel_note_off`) on MIDI note-offs, enabling natural instrument envelope decay rather than choking notes abruptly.
* **MIDI Range Clamping (Melodic Note Safety)**:
  * Restricts all melodic notes (excluding drum channels) to the General MIDI physical range `[28, 108]` (E1 to C8) using octave transpositions.
  * Prevents low-pitched chord root shifts or patterns from dropping below note 28 (which causes voice-dropping in the SoundFont synthesizer).
* **32-Voice Polyphony Synthesiser**:
  * Utilises an embedded General MIDI SoundFont (`general_midi.sf2`) rendered through a highly optimized TinySoundFont (`tsf.h`) voice-allocation engine.
  * Capable of rendering 32 simultaneous voices, accommodating complex jazz extensions, voice-leading decays, and ambient pad decays without note-stealing.
* **Sample-Accurate Sequencer with Jitter-Free Looping**:
  * Evaluates tick-based timing (960 ticks per quarter note) relative to system tempo to trigger note events with sample-accurate timing.
  * Splitting buffer processing precisely at loop boundaries maintains fractional-sample phase accuracy, preventing note hangs or micro-skips when looping playback.
  * Performs safe playhead wrapping via `std::fmod` when the loop length or active progression changes dynamically to prevent playback freezes or crashes.

---

## 2. Advanced Song Sections & Arranger

* **Advanced Song Sections**:
  * Transitions playback from a single global chord array to a vector of `SongSection`s.
  * Sequencer loops sections sequentially and dynamically inherits parameters (BPM, active key, and mixer configurations) on section boundaries.
* **Section Tab Bar Component**:
  * Adds a horizontal tab bar above the timeline to manage sections on-the-fly.
  * Allows switching the active section, renaming section tabs via double-tap, and modifying loop counts or deleting sections via a right-click or long-press context menu.
* **Horizontally Scrollable Viewport Timeline**:
  * Wraps the timeline grid in a horizontal `juce::Viewport`, allowing users to build long arrangements and scroll smoothly across blocks.
* **Multi-Select & Clipboard Drawers**:
  * Supports drag-selection of multiple chord blocks to trigger copy, paste (insert/overwrite), duplicate (next section), or batch-deletion actions.
  * Highlighted blocks glow with a pulsing blue neon border to clearly denote selection status.
* **Synchronized Playback Progress Wipe**:
  * Features a visual linear color-wipe animation sweeping across each chord block during playback.
  * The wipe speed is mathematically synced to the active block's duration and time signature (e.g. 4/4, 3/4, 7/8).
* **1-Bar Chord Audition Preview**:
  * Clicking a chord block in the timeline initiates a 1-bar preview of that specific chord using the active mixer patterns and instruments, immediately overriding standard playback.

---

## 3. The Full-Screen Track Mixer

* **Full-Screen Track Mixer**:
  * A full-screen overlay component bound to screen edges, featuring a vertical scrollable viewport for managing up to 16 tracks.
* **Horizontal Volume Sliders**:
  * Each track row features a horizontal volume slider mapping directly to the track volume, dispatching real-time DSP volume changes to the audio engine.
* **Mute/Unmute Toggles**:
  * Quickly enable or disable individual track channels during playback.
* **Instrument Presets & Pattern Routing**:
  * Click instrument dropdowns to assign any General MIDI instrument (0-127) or select from the pattern dropdown list to assign drum or melodic MIDI patterns.
* **Dynamic Track Management**:
  * Add new instrumental or drum lanes to the section using `[ + Add Melodic ]` or `[ + Add Drums ]` buttons, or delete them via `[ Remove ]`.

---

## 4. Harmonic Advice & Dynamic Key/Mode Setup

* **Nested Key & Mode Selector**:
  * Key selector features a nested 2-column popup menu displaying all 12 key roots in Column 1 and all 7 modes (Ionian/Major, Dorian, Phrygian, Lydian, Mixolydian, Aeolian/Minor, Locrian) in Column 2.
* **Music Theory Color-Coding**:
  * Evaluates active chords relative to the project key and mode to advise on harmonic tension:
    * 🟢 **Green (Diatonic)**: Fits strictly within the parent scale degrees.
    * 🟡 **Yellow (Modal Mixture/Parallel Borrow)**: Secondary dominants, parallel scale borrows, or modal substitutions.
    * 🔴 **Red (Experimental)**: Chromatic, dissonant, or experimental chord choices.
* **Interactive Passing Chord Menu**:
  * Calculates three context-aware passing chords targeting the next chord block's root: Secondary Dominants, Tritone Substitutions, and Leading Tones.
  * Dynamically inserts 1-beat transitional blocks (reducing previous block duration) or performs on-the-fly overwriting of existing 1-beat blocks.
* **Custom Slash Chords (Bass Note Shift)**:
  * Allows shifting the bass note to any chromatic pitch to create slash chords (e.g., `Cm/E`).
  * Suggests chord tones and diatonic key tones highlighted in green to guide bass-line voicing.
* **Inversion Change Warning Popups**:
  * Protects custom arrangements by popping up confirmation warnings before clearing a custom bass note during inversion changes or resets.
* **AI Advice & Gemini Nano Integration**:
  * Integrates Google's on-device Gemini Nano model (`com.google.android.aicore.llminference.LlmInference`) on Android 14+ to provide real-time chord recommendations.
  * Runs on a background thread to prevent UI stuttering, falling back to a music-theory scale degree generator on Windows.
* **Material You Dynamic Theming**:
  * Reconstructs Android OS wallpaper accents using JNI bindings (`system_accent1_500` and `system_accent1_800`) to match the user's device wallpaper theme.
  * Defaults to a premium dark-mode glassmorphic interface (`0xff1e1e2e` and `0xff111827`) on desktop environments.

---

## 5. Hardware Integration & Studio Export

* **Real-Time USB/Virtual MIDI Device Output**:
  * Scans and hot-plugs all connected MIDI devices in the background every 2 seconds.
  * Broadcasts polyphonic MIDI note messages in real time on multiple channels matching track lanes (Lanes 1-16 mapped to MIDI channels 1-16), allowing Chordcraft Studio to act as a hardware controller for external DAWs like Ableton, Logic, or Pro Tools.
* **Multi-Page Vector-Based PDF Sheet Music Exporter**:
  * Generates high-fidelity vector PDF sheet music (`Chordcraft_Studio_SheetMusic.pdf`) in the user's `Documents` folder, displaying the section name in the header, with auto-resized metadata fonts.
  * Prompts the user to choose between "Chords Only" (single page summarizing progression voicing) and "Full Sheet Music".
  * **Full Sheet Music Layout**: Renders separate staff pages for each enabled track in the arrangement, displaying the exact notes triggered by their patterns.
  * **Professional Notation**: Features an 8px staff spacing, proportional noteheads, properly oriented stems, ledger lines, and flat `b` signs in flat keys.
* **Type 1 Multi-Track MIDI Export**:
  * Exports detailed MIDI sequences with separate tracks for bass lines, chord voicings, and tempo configurations across all sections.
* **Offline Audio Bounce (WAV)**:
  * Renders arrangements directly to high-fidelity 24-bit WAV files using an offline background worker thread.
* **File Cleansing Security**:
  * Invokes `deleteFile()` on the target path to overwrite existing files cleanly and avoid stream conflicts.
* **Native Android Sharing Sheets**:
  * Successful MIDI, WAV, and PDF exports automatically trigger the native Android sharing chooser (`juce::ContentSharer`) on mobile.

---

## 6. Mobile Native Ergonomics & Dismissals

* **Hardware Back Key & Gesture Dismissals**:
  * All overlays (Mixer, Manual, BPM Panel, Global Menu) intercept Android `KeyPress::backKey` and iOS left-edge horizontal swipe to dismiss.
* **Full-Screen Help Manual Overlay**:
  * Full-screen help manual with safe-area constraints, dark rounded background, drop shadow, and horizontal swipe-to-paginate gesture.
* **BPM UX**:
  * BPM button tap triggers an input dialog for entering the BPM directly, featuring a toggle checkbox to apply the tempo globally to all sections or only to the active section.
* **Haptic Ratchet & Impact Feedback**:
  * Triggers tailored vibration patterns (Android Vibrator JNI) during block drag-selections and grid coordinate updates.
* **Touch-Drag List Scrolling**:
  * Allows smooth flick-scrolling across roots, qualities, and durations lists in the inspector, fully clamped to avoid list clipping.
