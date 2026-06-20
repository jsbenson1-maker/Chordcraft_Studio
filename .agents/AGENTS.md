# Antigravity Developer & Architecture Guide - Chordcraft Studio

Welcome to the **Chordcraft Studio** workspace. This guide outlines the project architecture, real-time safety rules, platform bridges, and UI behaviors to bring the next Antigravity instance up to speed immediately.

## 1. Project Directory & Compile Environment
- **Workspace Path**: `c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio`
- **Compiler Options**: Visual Studio 2022/2026, targeting **x64 Release** configuration.
- **MSBuild Tool Location**: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`
- **JUCE framework**: Built using Projucer (`ChordcraftStudio.jucer`). Regenerate solution files from the jucer file if source files are added.

## 2. Core Architecture Specifications

### A. Real-Time Deterministic Audio Thread
- **File**: `Source/ChordcraftAudioProcessor.cpp` | `Source/ChordcraftAudioProcessor.h`
- **Rule**: **No locks, no heap allocations, no file I/O** inside the real-time process loop (`processBlock()`).
- **Inter-Thread Communication**: Single-Producer Single-Consumer (SPSC) lock-free queue using `juce::AbstractFifo` to pass events from the UI MessageThread to the AudioThread.
- **Synthesiser**: A custom polyphonic synthesiser configured with **32 voices** (defined in `Source/SynthVoices.h`) to handle complex extended jazz chords and long decay tails.
- **Playback Sequencer**: Uses absolute tick-based calculations (960 ticks per quarter note) for sample-accurate scheduling.

### B. Core Data Models & Database
- **Files**:
  - `Source/ChordDatabase.h` / `Source/ChordDatabase.cpp`: Parses the embedded `BinaryData::chords_json` containing 2,160 chords on startup. Populates an O(1) hashmap with keys formatted as `{root}_{quality}_i{inversion}` (e.g. `Db_Maj_i0` - note flat spell canonicalization).
  - `Source/ChordArrangement.h`: Stores the list of `ChordBlock` elements (root, quality, inversion, beats duration, `durationText`, color, bounds, resolved MIDI notes).

### C. Studio Export Suite (Offline Render)
- **File**: `Source/ExportEngine.h`
- **MID Export**: Generates Type 1 multi-track MIDI files (tempo track, bass note track, upper chord voicing track).
- **WAV Export**: Performs faster-than-realtime offline audio bounces on a background thread (`OfflineRenderThread`), feeding an isolated instance of the 32-voice synthesiser to render wav files.

### D. AI Advice & Fallback Inference
- **File**: `Source/AiCoreManager.h`
- **Android AI Core**: Implements JNI bindings to Google's on-device Gemini Nano model (`com.google.android.aicore.llminference.LlmInference`) on Android 14+. Prompts are structured to return JSON suggestions.
- **Windows Fallback**: A C++ music-theory-driven suggestion generator using relative scale degrees of the active key.
- **Thread Safety**: Runs all inference asynchronously on a background thread so the UI/MessageManager remains responsive.

## 3. UI Component Implementations

### A. Timeline View
- **File**: [TimelineComponent.cpp](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/TimelineComponent.cpp) | [TimelineComponent.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/TimelineComponent.h)
- **Features**: Draws chord blocks with their chord name and time signature duration label. Drag-selection activates clipboard copy/delete drawers. Calls `resized()` in the callback to automatically align blocks on insertion/removal.

### B. Inspector View (Grid Editor)
- **File**: [InspectorComponent.cpp](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/InspectorComponent.cpp) | [InspectorComponent.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/InspectorComponent.h)
- **Top Row Menu Layout**: Key, Bass, Category, and INV buttons are distributed proportionally (25% / 25% / 30% / 20%) to prevent clipping on small portrait mobile screens.
- **Bass Note Popup Menu**: Dynamically checks database files to only present valid note values matching the selected chord's available inversions.
- **Grid Layout**: Features a dynamic scrolling grid splitting roots, qualities, and durations.
- **Accidental Modifiers**: Flat, Natural, and Sharp buttons modify selected pitches with flat spell canonicalization.
- **BPM Timing Adjustments**: Mouse drags change BPM immediately. Clicking the BPM box triggers an asynchronous text popup box (`juce::AlertWindow::enterModalState`) for manual input.

## 4. Music Theory Color-Coding Rules (Harmonic Advice)
We use a visual color coding system to advise the user on progression tensions:
- 🟢 **Green (Diatonic)**: Chords fitting strictly within the active key (e.g., I, ii, iii, IV, V, vi, vii°).
- 🟡 **Yellow (Modal Mixture)**: Off-key secondary dominants, tritone substitutions, and parallel scale borrow chords.
- 🔴 **Red (Experimental)**: Dissonant, highly chromatic chord choices.
- *Color Resolution*: Performed in `getChordTheoryColour` dynamically per quality cell. Fallback defaults to column values if rules are not hit.

## 5. Mobile Android Integration (Guarded by `#if JUCE_ANDROID`)
- **Wallpaper Extraction**: Reconstructs JNI signatures to pull wallpapers accent colors (`system_accent1_500` and `system_accent1_800`) to dynamically theme the app UI.
- **Haptic Ratchet & Impact**: Triggers Android vibrator patterns on dragging and releasing chord selections.
- **Memory Security**: Explicitly frees JNI local references (`env->DeleteLocalRef`) on the UI loop to prevent Android Out-of-Memory exceptions.
