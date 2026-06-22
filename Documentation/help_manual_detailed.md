# Chordcraft Studio - User Manual & Technical Reference Guide

Welcome to the Chordcraft Studio User Manual. This guide provides comprehensive, step-by-step instructions for getting the most out of Chordcraft Studio. It covers timeline editing, song section arrangement, the track mixer, chord voicing adjustments, real-time external MIDI routing, vector sheet music compilation, and the color-coded harmonic advice system.

---

## Table of Contents
1. **Introduction & System Requirements**
2. **User Interface Overview**
3. **Advanced Song Sections & Arrangement**
4. **The Arranger Timeline & Viewport**
5. **The Inspector & Harmonic Grid**
6. **Harmonic Advice & Color Theory**
7. **The Full-Screen Track Mixer**
8. **Real-Time MIDI Controller Setup (External Routing)**
9. **Offline Bounces & Exporters (WAV, MIDI, PDF)**
10. **Help Manual Paging & Overlays**
11. **Mobile Gestures, Ergonomics, & Dismissals**
12. **Real-Time Audio Engine & DSP Stabilization (Thread Safety, Pitch Clamping, & Soft Clipping)**

---

## 1. Introduction & System Requirements

Chordcraft Studio is a professional utility designed to facilitate the rapid creation, testing, and rendering of complex chord progressions.

### System Requirements
* **Windows**: Windows 10/11/12 (x64), minimum 4GB RAM, and an available soundcard driver (WASAPI/ASIO compatible).
* **Android**: Android 14 (API level 34) or higher, with haptic actuator support.
* **MIDI Hookups**: Any class-compliant USB MIDI keyboard, external synthesizer, virtual MIDI driver (e.g. loopMIDI), or USB-to-Host connection.

---

## 2. User Interface Overview

The interface is divided into several key layout areas:
1. **Section Tab Bar (Top Row)**: Manages and displays the song's structural sections (e.g., Intro, Verse, Chorus). Contains buttons for adding sections, renaming, and setting loop counts.
2. **Arranger Timeline (Upper-Middle Area)**: Displays your chord blocks sequentially within a horizontally scrollable viewport. Shows chord names, active playback playheads, selection halos, and time signature ratios.
3. **Inspector Column & Theory Grid (Lower-Middle Area)**: Divided into three scrollable lists containing Roots (Left), Qualities/AI Advice (Middle), and Durations/Time Signatures (Right). Includes modifiers for accidentals, bass note voicing override, and inversion modal drawer access.
4. **Global Control Bar (Bottom Area)**: Controls play/stop transport, global or section-specific BPM tempo adjustment, mixer overlay toggles, help manual toggles, and master file settings.

---

## 3. Advanced Song Sections & Arrangement

Chordcraft Studio organizes compositions using **Song Sections**, allowing you to create complex multi-part arrangements (e.g., Intro -> Verse -> Chorus -> Verse -> Chorus -> Outro).

### Song Section Data Model
Each section in the arrangement is fully independent and encapsulates:
* **Section Name**: A custom string to label the section (e.g., "Verse 1").
* **Loop Count**: The number of times this specific section will repeat before the sequencer advances to the next section.
* **Blocks**: The vector of chord blocks within this section.
* **BPM**: Section-specific tempo, allowing tempo changes between parts of the song.
* **Key/Mode**: Section-specific key (e.g., "C Maj" for Verse, "A Min" for Chorus).
* **Tracks**: Section-specific instruments, pattern routings, and volume settings.

### Section Tab Bar Operations
The Section Tab Bar sits at the top of the interface, providing direct controls:
* **Switching Sections**: Single-tap a tab to switch the active section. The timeline, inspector, key, tempo, and mixer immediately reload to reflect this section's parameters.
* **Adding Sections [ + ]**: Tap the `[ + ]` button at the right of the tab bar. This creates a new section. To maintain workflow continuity, the new section inherits the BPM, active key, and mixer configurations from the previously selected section.
* **Renaming Sections**: Double-tap an active section tab to trigger an text input dialog. Enter a new name (e.g., "Chorus") and press OK.
* **Loop Count & Deletion Menu**: Right-click (PC) or long-press (Mobile) a section tab to open a context menu:
  * **Set Loop Count**: Input the number of repetitions for this section.
  * **Delete Section**: Deletes the section. (The app enforces a minimum of one section at all times).

---

## 4. The Arranger Timeline & Viewport

The Timeline is where you construct your chord progressions.

### Timeline Viewport Scrolling
To support long arrangements and complex progressions, the timeline is wrapped in a horizontal `juce::Viewport`.
* Swipe horizontally or drag the mouse scrollbar to view chord blocks beyond the boundaries of the screen.
* During sequencer playback, the viewport automatically scrolls to keep the active playhead in view.

### Playback Progress Wipe
During active sequencer playback, a visual progress wipe sweeps across the active chord block. This wipe follows the sequencer playhead position, running from left to right at a speed proportional to the block's duration and BPM. Selecting a chord block does not affect the wipe animation, which always tracks the transport's actual playhead.

### Core Timeline Gestures & Clicks
* **Block Selection & Audition Preview**: Click any chord block to select it.
  * Selecting a block triggers a 1-bar preview of that specific chord using the active mixer patterns and instruments, stopping the regular sequencer. Pressing Play in the control bar overrides the preview and resumes normal playback.
  * The selected block highlights with a neon border and updates the Inspector.
* **Adding Chord Blocks**: Click the **Insert** button in the inspector top row to append a new default chord block (C Maj, 4 beats) after your current selection.
* **Removing Chord Blocks**: Click the **Remove** button in the inspector top row to delete the selected block. If all blocks are deleted, a default C Maj placeholder is created.
* **Multi-Select & Clipboard Mode**:
  * **Long Press (Mobile) / Drag Selection (PC)**: Select multiple blocks to open the selection panel. You can copy, paste (insert/overwrite), or duplicate the selection.

---

## 5. The Inspector & Harmonic Grid

The Inspector allows you to modify the selected chord block's properties.

### Standard Grid Operations
* **Choosing a Root Note**: Select a note from the **Roots** column (C, D, E, F, G, A, B).
* **Accidental Modifiers**: Adjust the root spelling by clicking the flat (♭), natural (♮), or sharp (♯) buttons on the top left. Spellings are canonicalized to flats (e.g. Eb, Gb, Bb) to match the chord databases.
* **Selecting Qualities**: Choose from the central qualities grid. You can scroll this list vertically via mouse wheel or by dragging/flicking on touch devices.
* **Setting Durations**: Scroll the **Duration** column to choose from time signature increments (1/4 up to 12/8).

### Advanced Voicing & Octave Adjustments
* **Inversion & Octave Drawer**: Click the **INV** button to open the Inversion modal overlay.
  * The drawer contains a 3x3 matrix combining octaves (Low, Mid, High) and inversions (Root, 1st Inv, 2nd Inv).
  * Select a cell to change the chord's voicing and register.
  * Click **Auto Select** to reset to standard root position.
  * **Inversion Warning**: If you have a custom slash bass note active, changing the inversion or clicking Reset will pop up a confirmation alert warning you that the custom bass note will be cleared.
* **Custom Bass Note (Slash Chords)**: Click the **Bass** button. 
  * A dropdown menu displays all 12 chromatic pitches.
  * **Smart Advice**: Chord tones (e.g., C, E, G for a C Maj chord) and diatonic key tones are highlighted in **green** with tags like `(Chord Root)`, `(Chord Tone)`, or `(Key Tone)`.
  * Selecting a note applies it as a custom bass override, formatting the chord block as a slash chord (e.g. `C Maj/F`).
  * If the custom bass note matches a standard inversion (e.g. Eb in a Cm chord), the app automatically applies that inversion and clears the custom bass string. Otherwise, it establishes a custom slash voicing.

### Interactive Passing Chords
* **Insert Passing Chord**: When a chord block is selected, click **Insert Passing Chord** to display a popup menu with three context-aware passing chords calculated from the root of the next chord block in the arrangement:
  * **Secondary Dominant**: A dominant 7th chord built on the fifth scale degree of the target root (e.g., "G7" targeting "C").
  * **Tritone Substitution**: A dominant 7th chord built a half-step above the target root (e.g., "Db7" targeting "C").
  * **Leading Tone**: A diminished 7th chord built a half-step below the target root (e.g., "Bdim7" targeting "C").
* **Insertion & Overwrite Behavior**:
  * If the selected chord block is longer than 1 beat (e.g., 4/4 or 2/4), it is reduced in duration by 1 beat, and the passing chord is inserted as a new 1-beat block immediately after it.
  * If the selected block is already 1 beat (1/4), its root and quality are overwritten on-the-fly.

---

## 6. Harmonic Advice & Color Theory

The analysis system evaluates the harmonic function of each chord relative to your selected project **Key** and **Mode**.

### Setting Key and Mode
1. Click the **Key** button in the inspector top row.
2. A 2-column menu appears: Column 1 displays all 12 roots, and Column 2 displays all 7 modes (Ionian/Major, Dorian, Phrygian, Lydian, Mixolydian, Aeolian/Minor, Locrian).
3. Select your desired key root and scale mode.

### Color-Coded Tension Advice
The active analysis system updates the qualities grid cell colors dynamically:
* **Green Cells (Diatonic)**: These qualities fit perfectly within the selected root/mode scale. They sound stable, consonant, and resolved (e.g., D Minor in C Ionian/Major).
* **Yellow Cells (Modal Mixture / Parallel Borrow)**: These are secondary dominants, tritone substitutions, or parallel scale borrow chords. They introduce pleasing tension and voice-leading paths.
* **Red Cells (Experimental)**: These are chromatic or highly dissonant relationships, useful for jazz extensions or dramatic transitions.

---

## 7. The Full-Screen Track Mixer

The **Track Mixer** is a full-screen overlay component bound to screen edges, featuring a vertical scrollable viewport for managing up to 16 tracks.

### Features & Controls
* **Track List Viewport**: The mixer list is wrapped in a `juce::Viewport` to support vertical scrolling, ensuring smooth navigation across all track lanes on both mobile and desktop screens.
* **Horizontal Volume Sliders**: Each track row features a horizontal volume slider mapping to the track volume. Dragging a slider updates the track volume and immediately dispatches real-time DSP volume changes to the audio engine.
* **Mute/Unmute Toggles**: Quickly enable or disable individual track channels during playback.
* **Instrument Presets**: Click the instrument dropdown to assign any General MIDI instrument (0-127) to the track.
* **Pattern Assignments**: Select from the pattern dropdown list to assign drum or melodic MIDI patterns to the track.
* **Dynamic Track Management**:
  * **[ + Add Melodic ] / [ + Add Drums ]**: Add new instrumental or drum lanes to the section.
  * **[ Remove ]**: Delete the corresponding track row.

---

## 8. Real-Time MIDI Controller Setup (External Routing)

Chordcraft Studio can act as an external sequencer controlling hardware synths or virtual instruments inside your DAW (e.g., Ableton, Logic, Pro Tools).

### Setup Instructions
1. **Windows Setup**: Install a virtual MIDI port driver (such as `loopMIDI`). Open it and create a virtual port (e.g. "Chordcraft Out").
2. **Android Setup**: Connect your Android device to your computer via USB. Set the Android USB configuration to **MIDI** mode in your Android system settings.
3. **Launch Chordcraft Studio**: The application automatically scans and connects to all active MIDI ports in the background.
4. **Configure your DAW**:
   - Enable your virtual port ("loopMIDI") or the connected Android device as an active **MIDI Input** port in your DAW.
   - Create MIDI tracks in your DAW and set them to receive MIDI from different channels.
5. **Channel Mappings**: Chordcraft Studio broadcasts separate MIDI channels for different tracks (Lanes 1 to 16 map to MIDI channels 1 to 16). Arm the corresponding channels in your DAW to record separate tracks.
6. **Auditioning**: Clicking chord blocks or playing the timeline will transmit the MIDI notes in real-time.

---

## 9. Offline Bounces & Exporters

* **Vector PDF Sheet Music**:
  * Click **MENU -> EXPORT -> Export Sheet Music (.pdf)**.
  * Choose between **Chords Only** (single page summarizing progression voicing) and **Full Sheet Music** (multi-page document including individual track pattern notes).
  * **Multi-Page Section Generation**: PDF sheets are generated sequentially across all song sections. Each page displays the section name in the header, with auto-resized metadata fonts to prevent clipping.
  * **Professional Notation**: Features an 8px staff spacing, proportional noteheads, properly oriented stems, ledger lines, and flat `b` signs in flat keys.
  * The file is saved as `Chordcraft_Studio_SheetMusic.pdf` inside your default **Documents** folder.
* **MIDI File Export**:
  * Click **MENU -> EXPORT -> Export MIDI File (.mid)**.
  * Generates a Type 1 multi-track MIDI file with separate tracks for the bass notes and chord voicings across all sections, following section loop counts.
* **Offline Audio Bounce (WAV)**:
  * Click **MENU -> EXPORT -> Export WAV Audio (.wav)**.
  * Spawns an offline renderer that bounces your arrangement sequentially across all sections to a 24-bit WAV file on a background thread.
  * Applies `std::tanh` soft-clipping to prevent digital distortion.
* **File Cleansing Security**:
  * Before generating PDF, MIDI, or WAV files, the exporter invokes `deleteFile()` on the target path to overwrite existing files cleanly and avoid stream conflicts.
* **Native Android Sharing Integration**:
  * On Android, exporting PDF, MIDI, or WAV files automatically triggers the native Android sharing chooser.

---

## 10. Help Manual Paging & Overlays

The Help Manual is a full-screen, safe-area-compliant overlay with a premium dark rounded layout (`0xff1e1e2e`), a drop shadow, and horizontal pagination.
* **Swipe-to-Paginate Gesture**: Swipe horizontally left or right across the screen to navigate through the help manual pages.
* **Manual Buttons**: Click the "Previous" and "Next" buttons at the bottom to paginate.
* **Monospace Layout Clamping**: Text files are pre-formatted to ensure monospace code snippets and theory advice fit neatly without line-wrapping on small smartphone viewports.

---

## 11. Mobile Gestures, Ergonomics, & Dismissals

Chordcraft Studio provides native touch-based dismissals and feedback to streamline workflow on mobile devices:
* **Left-Edge Swipe-to-Dismiss**: Swipe horizontally from the left edge of the screen (left-to-right swipe) to instantly dismiss the **Mixer**, **Help Manual**, or **Global Menu** overlays.
* **Hardware Back Key Intercept**: On Android, pressing the hardware `Back` button (which registers as `escapeKey` in JUCE) intercepts the active overlay first, dismissing the panel and returning focus to the timeline.
* **BPM UX**: BPM button tap triggers an input dialog for entering the BPM directly, featuring a toggle checkbox to apply the tempo globally to all sections or only to the active section.
* **Haptic Actuator Integration**:
  * Tapping buttons or grid cells triggers a brief **Impact** vibration.
  * Dragging across chord blocks in multi-select mode triggers a **Ratchet** vibration pattern as each boundary is crossed.

---

## 12. Real-Time Audio Engine & DSP Stabilization

To ensure low-latency performance and professional sound, the audio engine runs under strict real-time constraints:
* **Lock-Free Pointer Swaps**: All progression modifications on the UI thread compile into a staging buffer. A `SwapProgression` instruction is pushed to the audio thread, which swaps staging and active pointers in O(1) time using a non-blocking `juce::ScopedTryLock`.
* **Master Soft-Clipping**: A soft-clipper waveshaper (`std::tanh`) is applied in `processBlock()` and the WAV renderer, limiting output gain to prevent digital clipping when combining multiple polyphonic layers.
* **ADSR Release Triggers**: Note-offs trigger the TinySoundFont channel release function (`tsf_channel_note_off`), initiating natural instrument envelope decay rather than choking notes abruptly.
* **MIDI Pitch Safety Clamping**: All melodic notes are clamped to the physical range `[28, 108]` (E1 to C8) using octave transpositions.
* **Playhead Wrapping**: Active sequencer playhead position is mathematically wrapped against the new loop duration using `std::fmod` when loop lengths change, preventing timing drift or hangs.
* **Auto-Composer DSP Syncing**: Generative composition callbacks update track properties and sync presets directly to the audio thread.
