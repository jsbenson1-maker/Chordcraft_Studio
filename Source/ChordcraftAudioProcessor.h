#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>

// Forward declarations
struct ChordBlock;

// Custom single-producer, single-consumer lock-free queue wrapping juce::AbstractFifo
template <typename T, int Size = 256>
class LockFreeQueue
{
public:
    LockFreeQueue() : fifo (Size) {}

    bool push (const T& item)
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite (1, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            buffer[start1] = item;
            fifo.finishedWrite (1);
            return true;
        }
        
        return false; // Queue full
    }

    bool pop (T& item)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead (1, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            item = buffer[start1];
            fifo.finishedRead (1);
            return true;
        }
        
        return false; // Queue empty
    }

private:
    juce::AbstractFifo fifo;
    T buffer[Size];
};

struct tsf;
struct TrackSettings;
using TrackLane = TrackSettings;
struct SongSection;

struct SequencerEvent
{
    int tick = 0;
    int midiNote = 0;
    int velocity = 0;
    int channel = 0; // 0-15
    bool isNoteOn = true;
};

struct PlaybackSection
{
    std::string name;
    int startTick = 0;
    int durationTicks = 0;
    double bpm = 120.0;
    int loopCount = 1;
    double startSample = 0.0;
    double durationSamples = 0.0;
    int programs[16] = { 0 };
    float volumes[16] = { 0.6f };
};

struct SongPlaybackData
{
    std::vector<PlaybackSection> sections;
    std::vector<SequencerEvent> events;
    double totalDurationSamples = 0.0;
};

struct PlaybackInstruction
{
    enum Type
    {
        NoteOn,
        NoteOff,
        AllNotesOff,
        SetTempo,
        SetPlayState,
        ClearProgression,
        UpdateProgressionBlock,
        SetChannelProgram,
        SetChannelVolume,
        SwapProgression
    };

    Type type = AllNotesOff;
    int midiNote = 0;
    int velocity = 0;
    float tempo = 120.0f;
    bool isPlaying = false;
    
    // For progression block updates
    int blockIndex = 0;
    char chordId[32] = { 0 }; // Fixed-size char array for O(1) database lookup without heap allocations
    int startTick = 0;
    int durationTicks = 0;
    int octave = 1;
};

class ChordcraftAudioProcessor : public juce::AudioProcessor
{
public:
    ChordcraftAudioProcessor();
    ~ChordcraftAudioProcessor() override;

    // Standard juce::AudioProcessor method overrides
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    const juce::String getName() const override { return "Chordcraft Engine"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

    // Lock-free queue interface (UI thread pushes)
    void queueInstruction (const PlaybackInstruction& instruction);
    
    // Helper helper methods for sending commands from the UI thread
    void auditionNoteOn (int midiNote, int velocity);
    void auditionNoteOff (int midiNote);
    void clearAudition();
    void setTempo (float bpm);
    void setPlayState (bool play);
    void sendProgressionToAudioThread (const std::vector<SongSection>& sections, int activeSectionIndex);
    void setChannelProgram (int channel, int programNumber);
    void updateActiveMidiOutputs();
    void playChordPreview (const ChordBlock& cb, const std::vector<TrackLane>& lanes);

    // Atomic getters for UI synchronization (safe to read from UI thread)
    bool isEnginePlaying() const { return isPlayingAtomic.get() != 0; }
    float getEngineTempo() const { return tempoAtomic.get(); }
    int getPlayheadTick() const { return playheadTickAtomic.get(); }
    int getPlayingSectionIndex() const { return playingSectionIndexAtomic.get(); }
    bool isPreviewActive() const { return isSingleShotPreview.load(); }

private:
    // Lock-free queue for instructions - enlarged to 4096 to prevent overflow with sequencer updates
    LockFreeQueue<PlaybackInstruction, 4096> instructionQueue;

    // TinySoundFont sampler instance
    tsf* tsfInstance = nullptr;
    std::vector<float> interpBuffer;

    // Audio thread local state (only accessed/modified on audio thread)
    bool isPlayingLocal = false;
    float tempoLocal = 120.0f;
    double currentSamplePosition = 0;
    double sampleRateLocal = 44100.0;
    double samplesPerTick = 0.0;
    
    // Double-buffered song playback data
    SongPlaybackData activePlaybackData;
    SongPlaybackData stagingPlaybackData;
    juce::CriticalSection stagingLock;
    int lastSectionIndexPlayed = -1;

    // Atomic values updated by audio thread for UI visibility
    juce::Atomic<int> isPlayingAtomic { 0 };
    juce::Atomic<float> tempoAtomic { 120.0f };
    juce::Atomic<int> playheadTickAtomic { 0 };
    juce::Atomic<int> playingSectionIndexAtomic { 0 };
    std::atomic<bool> isSingleShotPreview { false };

    void processQueueInstructions();
    void runSequencer (int numSamples, juce::MidiBuffer& midiMessages);
    void updateSequencerTiming();

    juce::CriticalSection midiOutputLock;
    juce::OwnedArray<juce::MidiOutput> activeMidiOutputs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordcraftAudioProcessor)
};
