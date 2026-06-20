# Chordcraft Studio - Developer Handoff Guide

Welcome, Antigravity instance. You are picking up the development of **Chordcraft Studio**, a premium interactive chord arranger and synthesiser built with JUCE targeting Windows Desktop and Android.

## Project Setup & Build Environment
- **JUCE Framework**: Managed via Projucer (`ChordcraftStudio.jucer`). If you add any source files, regenerate the IDE solutions first.
- **Windows IDE**: Visual Studio 2022/2026.
- **MSBuild Tool Location**: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`
- **Build Configurations**: Target **x64 Release**.
- **Android Integration**: Guarded by `#if JUCE_ANDROID` compiler directives (Material You wallpaper accent extraction, native JNI haptic patterns, and Android AICore Gemini Nano suggestions).

---

## Technical Architecture & File Layout

### 1. Audio Processing & Real-time Thread Safety
- [ChordcraftAudioProcessor.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/ChordcraftAudioProcessor.h) | [ChordcraftAudioProcessor.cpp](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/ChordcraftAudioProcessor.cpp)
  - **Constraints**: 100% lock-free, heap-allocation-free, and file I/O-free inside `processBlock()`.
  - **Synthesiser**: Uses [SynthVoices.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/SynthVoices.h) configured with **32 polyphonic voices** to handle extended chords and decay tails.
  - **Sequencer**: Plays chords using absolute ticks (960 ticks per quarter note) for sample-accurate scheduling.
  - **SPSC Queue**: Uses `juce::AbstractFifo` to pass events from UI MessageThread to the AudioThread.

### 2. Chord Database & Arrangement Data
- [ChordDatabase.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/ChordDatabase.h) | [ChordDatabase.cpp](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/ChordDatabase.cpp)
  - Parses `BinaryData::chords_json` (2,160 chords) on startup into an $O(1)$ hash map with keys like `Db_Maj_i0` (flat spell canonicalization).
- [ChordArrangement.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/ChordArrangement.h)
  - Holds list of `ChordBlock` structs (root, quality, inversion, duration, octave, color, selection).

### 3. Studio Export Suite (Offline Render)
- [ExportEngine.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/ExportEngine.h)
  - **MIDI**: Generates Type 1 multitrack MIDI files (tempo track, bass track, and upper voicings track).
  - **WAV**: Renders WAV files faster-than-realtime using a background thread (`OfflineRenderThread`) to keep audio processing allocation-free.

### 4. Interactive UI Components
- [TimelineComponent.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/TimelineComponent.h) | [TimelineComponent.cpp](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/TimelineComponent.cpp)
  - Draws chord blocks with chord names, time signature duration, and octave/inversion labels.
  - sweeps a horizontal playhead progress sweep wipe in time with playback using a 30ms repaint timer.
  - Long-press (400ms) or drag-selection triggers the **Clipboard Drawer** mode.
  - On mouse release (`mouseUp`), `dragStartIndex` is reset to `-1` to prevent drag-timer hijacking.
- [InspectorComponent.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/InspectorComponent.h) | [InspectorComponent.cpp](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/InspectorComponent.cpp)
  - Proportional top menu (Key, Bass, Category, INV).
  - Decoupled column scrolling (scrolling Roots, Qualities, or Durations independently).
  - Sleek modal $3 \times 3$ Octave/Inversion overlay drawer activated by the "INV" button.
  - Mode-isolated `mouseDown` handler with clipboard buttons (Copy, Delete, Paste, Next Section, Close Selection) checked early to avoid click hijacking.
  - Clicking the BPM box opens an async modal alert input box for typing exact BPM values.

### 5. Intelligent Harmonic Advice
- [AiCoreManager.h](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/Source/AiCoreManager.h)
  - Implements Android JNI bindings to Google's AICore Gemini Nano.
  - Implements a C++ music-theory-driven suggestion generator (Safe/Modal/Chromatic) matching the active key. Runs asynchronously in a background thread.

---

## Prompt to start the next Antigravity Instance
Copy and paste the following prompt when initializing your next Antigravity developer session:

```text
Initialize development on Chordcraft Studio. I have checked out the repository. Please review the project structure, specifically [DEVELOPER_PROMPT.md](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/DEVELOPER_PROMPT.md) and [.agents/AGENTS.md](file:///c:/Users/Administrator/.gemini/antigravity/scratch/Chordcraft-Studio/ChordcraftStudio/.agents/AGENTS.md) for full context. 

Verify that the latest fixes compile on Visual Studio using MSBuild targeting x64 Release configuration. Once verified, ask me how you should proceed.
```
