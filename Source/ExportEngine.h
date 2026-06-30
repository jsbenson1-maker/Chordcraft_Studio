#pragma once

#include <JuceHeader.h>
#include "tsf.h"
#include "ChordDatabase.h"
#include "PatternDatabase.h"
#include "ChordArrangement.h"
#include "ChordcraftAudioProcessor.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <optional>
#include <sstream>
#include <map>

inline juce::File getExportDirectory()
{
#if JUCE_ANDROID || JUCE_IOS
    return juce::File::getSpecialLocation (juce::File::tempDirectory);
#else
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
#endif
}

inline void saveExportedFile (const juce::File& generatedFile)
{
    juce::MessageManager::callAsync ([generatedFile]() {
        if (! generatedFile.existsAsFile()) return;

        static std::unique_ptr<juce::FileChooser> fc;
        fc = std::make_unique<juce::FileChooser> (
            "Save Export", 
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(generatedFile.getFileName()), 
            "*" + generatedFile.getFileExtension()
        );

        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
        
        fc->launchAsync (flags, [generatedFile] (const juce::FileChooser& chooser) {
            juce::URL targetURL = chooser.getURLResult();
            if (! targetURL.isEmpty())
            {
                if (std::unique_ptr<juce::OutputStream> out = targetURL.createOutputStream())
                {
                    juce::FileInputStream in (generatedFile);
                    out->writeFromInputStream (in, -1);
                }
            }
            fc.reset();
        });
    });
}

inline juce::String wrapPath (const juce::String& path)
{
    juce::String result;
    int charsSinceNewline = 0;
    for (int i = 0; i < path.length(); ++i)
    {
        auto c = path[i];
        result += c;
        charsSinceNewline++;
        if (c == '/' || c == '\\')
        {
            result += "\n";
            charsSinceNewline = 0;
        }
        else if (charsSinceNewline >= 24)
        {
            result += "\n";
            charsSinceNewline = 0;
        }
    }
    return result;
}

inline juce::String getGMInstrumentName (int programNum)
{
    static const char* gmInstruments[] = {
        "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano",
        "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavi",
        "Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
        "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
        "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)",
        "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar harmonics",
        "Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass",
        "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
        "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
        "String Ensemble 1", "String Ensemble 2", "SynthStrings 1", "SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
        "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "SynthBrass 1", "SynthBrass 2",
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
        "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
        "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)", "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass+lead)",
        "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)", "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
        "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)", "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)",
        "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bag pipe", "Fiddle", "Shanai",
        "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
        "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot"
    };
    if (programNum >= 0 && programNum < 128)
        return gmInstruments[programNum];
    return "Unknown Instrument";
}

class FastOfflineBouncer : public juce::Thread
{
public:
    FastOfflineBouncer (const ChordArrangement& arr, bool exportAsMidi)
        : juce::Thread ("Fast Offline Bouncer"), sections (arr.sections), isMidi (exportAsMidi) {}

    ~FastOfflineBouncer() override { stopThread (4000); }

    void run() override
    {
        double sampleRate = 44100.0;
        int blockSize = 512;
        double totalSamples = 0.0;
        const int ticksPerQuarterNote = 960;

        for (auto& sec : sections) {
            double secSamplesPerTick = sampleRate / (16.0 * sec.bpm);
            int secDurationTicks = 0;
            for (auto& cb : sec.blocks) secDurationTicks += cb.durationBeats * ticksPerQuarterNote;
            totalSamples += (secDurationTicks * secSamplesPerTick) * sec.loopCount;
        }
        if (totalSamples <= 0.0) return;

        // Spawn a headless clone of the main DSP engine
        ChordcraftAudioProcessor offlineProcessor;
        offlineProcessor.prepareToPlay (sampleRate, blockSize);
        offlineProcessor.sendProgressionToAudioThread (sections, 0);
        offlineProcessor.setPlayState (true);

        juce::File exportFile = getExportDirectory().getChildFile (isMidi ? "Chordcraft_Studio_Export.mid" : "Chordcraft_Studio_Export.wav");
        if (exportFile.existsAsFile()) exportFile.deleteFile();

        juce::MidiMessageSequence capturedMidi;
        std::unique_ptr<juce::AudioFormatWriter> wavWriter;
        
        if (! isMidi) {
            juce::WavAudioFormat wavFormat;
            if (auto* rawStream = exportFile.createOutputStream().release())
                wavWriter.reset (wavFormat.createWriterFor (rawStream, sampleRate, 2, 16, {}, 0));
        }

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midiBuffer;
        int samplesRendered = 0;

        // Uncapped DSP pump
        while (samplesRendered < totalSamples && ! threadShouldExit())
        {
            int numSamplesThisBlock = juce::jmin (blockSize, static_cast<int>(totalSamples) - samplesRendered);
            buffer.setSize (2, numSamplesThisBlock, false, false, true);
            buffer.clear();
            midiBuffer.clear();

            offlineProcessor.processBlock (buffer, midiBuffer);

            if (isMidi) {
                for (const auto meta : midiBuffer) {
                    auto msg = meta.getMessage();
                    msg.setTimeStamp (samplesRendered + meta.samplePosition);
                    capturedMidi.addEvent (msg);
                }
            } else if (wavWriter != nullptr) {
                wavWriter->writeFromAudioSampleBuffer (buffer, 0, numSamplesThisBlock);
            }
            samplesRendered += numSamplesThisBlock;
        }

        offlineProcessor.releaseResources();
        wavWriter.reset(); // Flush WAV to disk

        if (isMidi && ! threadShouldExit())
        {
            if (auto stream = std::unique_ptr<juce::FileOutputStream>(exportFile.createOutputStream()))
            {
                juce::MidiFile mf;
                mf.setTicksPerQuarterNote (960);
                capturedMidi.updateMatchedPairs();
                
                // Dynamic sample-to-tick mapper (handles tempo changes perfectly)
                auto sampleToTick = [&](double samplePos) -> double {
                    double accumSamples = 0.0, accumTicks = 0.0;
                    for (auto& sec : sections) {
                        double secSamplesPerTick = sampleRate / (16.0 * sec.bpm);
                        int secDurationTicks = 0;
                        for (auto& cb : sec.blocks) secDurationTicks += cb.durationBeats * ticksPerQuarterNote;
                        for (int loop = 0; loop < sec.loopCount; ++loop) {
                            double secSamples = secDurationTicks * secSamplesPerTick;
                            if (samplePos <= accumSamples + secSamples) return accumTicks + ((samplePos - accumSamples) / secSamplesPerTick);
                            accumSamples += secSamples; accumTicks += secDurationTicks;
                        }
                    }
                    return accumTicks;
                };
                
                // Split into Multitrack Format 1
                for (int ch = 1; ch <= 16; ++ch) {
                    juce::MidiMessageSequence trackSeq;
                    if (ch == 1) {
                        trackSeq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
                        double accumTicks = 0.0;
                        for (auto& sec : sections) {
                            int secDurationTicks = 0;
                            for (auto& cb : sec.blocks) secDurationTicks += cb.durationBeats * ticksPerQuarterNote;
                            for (int loop = 0; loop < sec.loopCount; ++loop) {
                                trackSeq.addEvent (juce::MidiMessage::tempoMetaEvent (static_cast<int>(60000000.0 / sec.bpm)), accumTicks);
                                accumTicks += secDurationTicks;
                            }
                        }
                    }
                    for (int i = 0; i < capturedMidi.getNumEvents(); ++i) {
                        auto msg = capturedMidi.getEventPointer(i)->message;
                        if (msg.getChannel() == ch || msg.isMetaEvent()) {
                            msg.setTimeStamp (sampleToTick (msg.getTimeStamp()));
                            trackSeq.addEvent (msg);
                        }
                    }
                    if (trackSeq.getNumEvents() > (ch == 1 ? 2 : 0)) mf.addTrack (trackSeq);
                }
                mf.writeTo (*stream);
            }
        }

        if (! threadShouldExit()) saveExportedFile (exportFile);
        else exportFile.deleteFile();
    }
private:
    std::vector<SongSection> sections;
    bool isMidi;
};

class OfflineRenderThread : public juce::Thread
{
public:
    OfflineRenderThread (const ChordArrangement&) : juce::Thread ("") {}
    void run() override {}
};

class SimplePdfWriter
{
public:
    SimplePdfWriter (int width, int height) : w (width), h (height) {}

    void newPage() { pagesContents.push_back (stream.str()); stream.str (""); stream.clear(); }
    void setFillColor (uint32_t argb) { stream << (((argb >> 16) & 0xff) / 255.0f) << " " << (((argb >> 8) & 0xff) / 255.0f) << " " << ((argb & 0xff) / 255.0f) << " rg\n"; }
    void setStrokeColor (uint32_t argb) { stream << (((argb >> 16) & 0xff) / 255.0f) << " " << (((argb >> 8) & 0xff) / 255.0f) << " " << ((argb & 0xff) / 255.0f) << " RG\n"; }
    void setLineWidth (float width) { stream << width << " w\n"; }
    void fillRect (float x, float y, float width, float height) { stream << x << " " << (h - (y + height)) << " " << width << " " << height << " re f\n"; }
    void drawLine (float x1, float y1, float x2, float y2) { stream << x1 << " " << (h - y1) << " m " << x2 << " " << (h - y2) << " l S\n"; }

    void fillEllipse (float cx, float cy, float rx, float ry)
    {
        float pdfCy = h - cy; float k = 0.55228475f;
        stream << (cx + rx) << " " << pdfCy << " m\n"
               << (cx + rx) << " " << (pdfCy + ry * k) << " " << (cx + rx * k) << " " << (pdfCy + ry) << " " << cx << " " << (pdfCy + ry) << " c\n"
               << (cx - rx * k) << " " << (pdfCy + ry) << " " << (cx - rx) << " " << (pdfCy + ry * k) << " " << (cx - rx) << " " << pdfCy << " c\n"
               << (cx - rx) << " " << (pdfCy - ry * k) << " " << (cx - rx * k) << " " << (pdfCy - ry) << " " << cx << " " << (pdfCy - ry) << " c\n"
               << (cx + rx * k) << " " << (pdfCy - ry) << " " << (cx + rx) << " " << (pdfCy - ry * k) << " " << (cx + rx) << " " << pdfCy << " c f\n";
    }

    void drawText (const juce::String& text, float x, float y, const juce::String& font, float fontSize, bool bold = false, bool italic = false)
    {
        juce::String fontRef = "/F1";
        if (font.containsIgnoreCase ("Georgia") || font.containsIgnoreCase ("Times")) {
            if (bold && italic) fontRef = "/F6"; else if (bold) fontRef = "/F4"; else if (italic) fontRef = "/F5"; else fontRef = "/F3";
        } else if (bold) fontRef = "/F2";

        stream << "BT\n" << fontRef << " " << fontSize << " Tf\n" << x << " " << (h - y) << " Td\n(" << escapePdfString (text) << ") Tj\nET\n";
    }

    void startPath (float x, float y) { pathStream.str (""); pathStream.clear(); pathX = x; pathY = h - y; pathStream << pathX << " " << pathY << " m\n"; }
    void lineTo (float x, float y) { pathX = x; pathY = h - y; pathStream << pathX << " " << pathY << " l\n"; }
    void quadraticTo (float cx, float cy, float x, float y)
    {
        float c1x = pathX + (2.0f / 3.0f) * (cx - pathX); float c1y = pathY + (2.0f / 3.0f) * ((h - cy) - pathY);
        float c2x = x + (2.0f / 3.0f) * (cx - x);         float c2y = (h - y) + (2.0f / 3.0f) * ((h - cy) - (h - y));
        pathStream << c1x << " " << c1y << " " << c2x << " " << c2y << " " << x << " " << (h - y) << " c\n";
        pathX = x; pathY = h - y;
    }

    void strokePath() { stream << pathStream.str() << "S\n"; }

    juce::String getPdfData()
    {
        if (! stream.str().empty() || pagesContents.empty()) { pagesContents.push_back (stream.str()); stream.str (""); stream.clear(); }
        std::ostringstream pdf; pdf << "%PDF-1.4\n";
        std::vector<size_t> offsets; size_t numPages = pagesContents.size(); if (numPages == 0) return "";
        size_t totalObjects = 3 + 2 * numPages; offsets.resize (totalObjects + 1, 0);

        offsets[1] = pdf.str().length(); pdf << "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
        offsets[2] = pdf.str().length(); pdf << "2 0 obj\n<< /Type /Pages /Kids [ ";
        for (size_t i = 0; i < numPages; ++i) pdf << (4 + 2 * i) << " 0 R ";
        pdf << "] /Count " << numPages << " >>\nendobj\n";

        offsets[3] = pdf.str().length();
        pdf << "3 0 obj\n<< /Font <<\n  /F1 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\n  /F2 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>\n  /F3 << /Type /Font /Subtype /Type1 /BaseFont /Times-Roman >>\n  /F4 << /Type /Font /Subtype /Type1 /BaseFont /Times-Bold >>\n  /F5 << /Type /Font /Subtype /Type1 /BaseFont /Times-Italic >>\n  /F6 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica-BoldOblique >>\n>> >>\nendobj\n";

        for (size_t i = 0; i < numPages; ++i)
        {
            size_t pageObjId = 4 + 2 * i; size_t contentObjId = 5 + 2 * i;
            offsets[pageObjId] = pdf.str().length();
            pdf << pageObjId << " 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [ 0 0 " << w << " " << h << " ] /Resources 3 0 R /Contents " << contentObjId << " 0 R >>\nendobj\n";
            offsets[contentObjId] = pdf.str().length();
            const std::string& content = pagesContents[i];
            pdf << contentObjId << " 0 obj\n<< /Length " << content.length() << " >>\nstream\n" << content << "endstream\nendobj\n";
        }

        size_t xrefPos = pdf.str().length();
        pdf << "xref\n0 " << (totalObjects + 1) << "\n0000000000 65535 f \n";
        for (size_t i = 1; i <= totalObjects; ++i) { char buf[30]; sprintf (buf, "%010zu 00000 n \n", offsets[i]); pdf << buf; }
        pdf << "trailer\n<< /Size " << (totalObjects + 1) << " /Root 1 0 R >>\nstartxref\n" << xrefPos << "\n%%EOF\n";
        return juce::String (pdf.str());
    }

private:
    int w, h; std::ostringstream stream, pathStream; float pathX = 0, pathY = 0; std::vector<std::string> pagesContents;
    juce::String escapePdfString (const juce::String& s)
    {
        juce::String result;
        for (int i = 0; i < s.length(); ++i) { if (s[i] == '\\' || s[i] == '(' || s[i] == ')') result += '\\'; result += s[i]; }
        return result;
    }
};

inline bool isFlatKey (const juce::String& activeKey)
{
    juce::String keyRoot = activeKey.upToFirstOccurrenceOf (" ", false, false).trim();
    if (keyRoot.containsIgnoreCase ("b") || keyRoot.equalsIgnoreCase ("F")) return true;
    bool isMinorOrModalFlat = activeKey.containsIgnoreCase ("Min") || activeKey.containsIgnoreCase ("Dorian") || activeKey.containsIgnoreCase ("Phrygian") || activeKey.containsIgnoreCase ("Aeolian") || activeKey.containsIgnoreCase ("Locrian");
    if (isMinorOrModalFlat && (keyRoot.equalsIgnoreCase ("D") || keyRoot.equalsIgnoreCase ("G") || keyRoot.equalsIgnoreCase ("C") || keyRoot.equalsIgnoreCase ("F"))) return true;
    return false;
}

static inline void drawChordsOnlyPage (SimplePdfWriter& pdf, const juce::String& sectionName, const juce::Array<ChordBlock>& chords, float bpm, const juce::String& activeKey)
{
    pdf.setFillColor (0xfffafaf9); pdf.fillRect (0, 0, 1200, 400);
    pdf.setFillColor (0xff1e1e2e); pdf.drawText ("Chordcraft Studio - " + sectionName, 50, 52, "Georgia", 22.0f, true, false);

    juce::String infoText = "Key: " + activeKey + "   Tempo: " + juce::String (bpm, 1) + " BPM";
    float fontSize = (activeKey.length() > 6) ? 11.0f : 14.0f;
    pdf.drawText (infoText, 1150.0f - (infoText.length() * (fontSize * 0.5f)), 35.0f + fontSize, "Georgia", fontSize, false, true);

    int centerY = 210, lineSpacing = 8, startX = 140, endX = 1150, numChords = chords.size();
    if (numChords == 0) return;

    pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (1.0f);
    for (int line = -2; line <= 2; ++line) pdf.drawLine (50.0f, centerY + line * lineSpacing, 1150.0f, centerY + line * lineSpacing);

    pdf.setStrokeColor (0xff1e1e2e); pdf.setLineWidth (1.5f);
    pdf.startPath (65, centerY + 24); pdf.lineTo (65, centerY - 24); pdf.quadraticTo (65, centerY - 35, 72, centerY - 35); pdf.quadraticTo (79, centerY - 35, 79, centerY - 24); pdf.lineTo (58, centerY + 10); pdf.quadraticTo (50, centerY + 17, 58, centerY + 24); pdf.quadraticTo (65, centerY + 30, 72, centerY + 24); pdf.quadraticTo (79, centerY + 17, 65, centerY + 3); pdf.quadraticTo (58, centerY - 10, 65, centerY - 10); pdf.strokePath();
    pdf.setFillColor (0xff1e1e2e); pdf.fillEllipse (65, centerY + 24, 2.5f, 2.5f);
    pdf.drawText ("4", 103, centerY - 2, "Georgia", 24.0f, true, false); pdf.drawText ("4", 103, centerY + 16, "Georgia", 24.0f, true, false);

    int colWidth = (endX - startX) / numChords;
    pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (1.5f); pdf.drawLine (startX, centerY - 16, startX, centerY + 16);

    bool flatSpelling = isFlatKey (activeKey);
    int stepInOctave[] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 }, accidentalInOctave[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };
    int stepInOctaveFlat[] = { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 }, accidentalInOctaveFlat[] = { 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0 };

    for (int i = 0; i < numChords; ++i)
    {
        auto& cb = chords.getReference (i);
        int blockX = startX + i * colWidth + colWidth / 2, barX = startX + (i + 1) * colWidth;
        pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (1.5f); pdf.drawLine (barX, centerY - 16, barX, centerY + 16);
        pdf.setFillColor (0xff1e1e2e); pdf.drawText (cb.name, blockX - (cb.name.length() * 4.0f), centerY - 55.0f, "Georgia", 14.0f, true, false);

        juce::Array<int> notes = resolveChordMidiNotes (cb);
        if (! notes.isEmpty())
        {
            int minStep = 999, maxStep = -999, avgStep = 0; float minNoteY = 0, maxNoteY = 0;
            for (int note : notes)
            {
                int pitch = (note - 60) % 12, octave = (note - 60) / 12; if (pitch < 0) { pitch += 12; octave -= 1; }
                int step = octave * 7 + (flatSpelling ? stepInOctaveFlat[pitch] : stepInOctave[pitch]);
                int acc = flatSpelling ? accidentalInOctaveFlat[pitch] : accidentalInOctave[pitch];
                int noteY = centerY - (step - 6) * 4.0f; avgStep += step;

                if (step < minStep) { minStep = step; minNoteY = noteY; } if (step > maxStep) { maxStep = step; maxNoteY = noteY; }
                pdf.setFillColor (0xff1e1e2e); pdf.fillEllipse (blockX, noteY, 3.25f, 2.25f);
                if (acc == 1) pdf.drawText ("#", blockX - 18, noteY + 3.5f, "Helvetica-Bold", 10.0f, true, false);
                else if (acc == -1) pdf.drawText ("b", blockX - 16, noteY + 3.5f, "Times-Bold", 10.0f, true, false);

                pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (0.75f);
                if (step <= 0) for (int s = 0; s >= step; s -= 2) pdf.drawLine (blockX - 8, centerY - (s - 6) * 4.0f, blockX + 8, centerY - (s - 6) * 4.0f);
                else if (step >= 12) for (int s = 12; s <= step; s += 2) pdf.drawLine (blockX - 8, centerY - (s - 6) * 4.0f, blockX + 8, centerY - (s - 6) * 4.0f);
            }
            avgStep /= notes.size();
            pdf.setStrokeColor (0xff1e1e2e); pdf.setLineWidth (1.0f);
            if (avgStep < 6) pdf.drawLine (blockX + 3.25f, minNoteY, blockX + 3.25f, maxNoteY - 28);
            else pdf.drawLine (blockX - 3.25f, maxNoteY, blockX - 3.25f, minNoteY + 28);
        }
    }
}

static inline void drawTrackPage (SimplePdfWriter& pdf, const juce::String& sectionName, const juce::Array<ChordBlock>& chords, const TrackLane& lane, int laneIdx, float bpm, const juce::String& activeKey)
{
    pdf.setFillColor (0xfffafaf9); pdf.fillRect (0, 0, 1200, 400);
    pdf.setFillColor (0xff1e1e2e);
    juce::String trackTitle = (laneIdx == 12) ? "Track 13 (Drums) (" + sectionName + ")" : "Track " + juce::String (laneIdx + 1) + " - " + getGMInstrumentName (lane.gmProgramNumber) + " (" + sectionName + ")";
    pdf.drawText (trackTitle, 50, 52, "Georgia", 22.0f, true, false);

    juce::String infoText = "Key: " + activeKey + "   Tempo: " + juce::String (bpm, 1) + " BPM";
    float fontSize = (activeKey.length() > 6) ? 11.0f : 14.0f;
    pdf.drawText (infoText, 1150.0f - (infoText.length() * (fontSize * 0.5f)), 35.0f + fontSize, "Georgia", fontSize, false, true);

    int centerY = 210, lineSpacing = 8, startX = 140, endX = 1150, numChords = chords.size();
    if (numChords == 0) return;

    pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (1.0f);
    for (int line = -2; line <= 2; ++line) pdf.drawLine (50.0f, centerY + line * lineSpacing, 1150.0f, centerY + line * lineSpacing);

    bool useBassClef = (!lane.isDrums) && ((lane.gmProgramNumber >= 32 && lane.gmProgramNumber <= 39) || lane.gmProgramNumber == 43 || lane.gmProgramNumber == 58 || lane.gmProgramNumber == 70);

    pdf.setStrokeColor (0xff1e1e2e); pdf.setLineWidth (1.5f);
    if (useBassClef) {
        // Draw F-Clef (Bass Clef)
        pdf.setFillColor (0xff1e1e2e);
        pdf.fillEllipse (65, centerY - 8, 2.5f, 2.5f);
        pdf.startPath (65, centerY - 8);
        pdf.quadraticTo (75, centerY - 18, 80, centerY - 8);
        pdf.quadraticTo (85, centerY + 5, 60, centerY + 16);
        pdf.strokePath();
        pdf.fillEllipse (85, centerY - 12, 1.5f, 1.5f);
        pdf.fillEllipse (85, centerY - 4, 1.5f, 1.5f);
    } else {
        // Draw G-Clef (Treble Clef)
        pdf.startPath (65, centerY + 24); pdf.lineTo (65, centerY - 24); pdf.quadraticTo (65, centerY - 35, 72, centerY - 35); pdf.quadraticTo (79, centerY - 35, 79, centerY - 24); pdf.lineTo (58, centerY + 10); pdf.quadraticTo (50, centerY + 17, 58, centerY + 24); pdf.quadraticTo (65, centerY + 30, 72, centerY + 24); pdf.quadraticTo (79, centerY + 17, 65, centerY + 3); pdf.quadraticTo (58, centerY - 10, 65, centerY - 10); pdf.strokePath();
        pdf.setFillColor (0xff1e1e2e); pdf.fillEllipse (65, centerY + 24, 2.5f, 2.5f);
    }
    pdf.drawText ("4", 103, centerY - 2, "Georgia", 24.0f, true, false); pdf.drawText ("4", 103, centerY + 16, "Georgia", 24.0f, true, false);

    int totalTicks = 0; for (auto& cb : chords) totalTicks += cb.durationBeats * 960;
    pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (1.5f); pdf.drawLine (startX, centerY - 16, startX, centerY + 16);

    bool flatSpelling = isFlatKey (activeKey);
    int stepInOctave[] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 }, accidentalInOctave[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };
    int stepInOctaveFlat[] = { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 }, accidentalInOctaveFlat[] = { 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0 };

    int currentTickOffset = 0;
    for (int i = 0; i < numChords; ++i)
    {
        auto& cb = chords.getReference (i); int blockDurationTicks = cb.durationBeats * 960;
        int blockStartX = startX + (currentTickOffset / (double)totalTicks) * (endX - startX);
        int blockEndX = startX + ((currentTickOffset + blockDurationTicks) / (double)totalTicks) * (endX - startX);

        pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (1.5f); pdf.drawLine (blockEndX, centerY - 16, blockEndX, centerY + 16);
        pdf.setFillColor (0xff1e1e2e); pdf.drawText (cb.name, ((blockStartX + blockEndX) / 2.0f) - (cb.name.length() * 3.5f), centerY - 55.0f, "Georgia", 14.0f, true, false);

        juce::Array<int> chordMidiNotes = resolveChordMidiNotes (cb);
        auto* pattern = PatternDatabase::getInstance().getPatternById (lane.patternId.toStdString());
        if (pattern != nullptr)
        {
            int patternLength = 3840, maxNoteEnd = 0;
            for (auto& n : pattern->notes) maxNoteEnd = std::max (maxNoteEnd, n.tick + n.duration);
            if (maxNoteEnd > 0) patternLength = maxNoteEnd;

        for (int offset = 0; offset < blockDurationTicks; offset += patternLength)
        {
            for (auto& note : pattern->notes)
            {
                int noteStartInBlock = offset + note.tick; if (noteStartInBlock >= blockDurationTicks) continue;
                int midiNote = 60;
                if (note.drumMidi.has_value()) midiNote = note.drumMidi.value();
                else if (note.degree.has_value() && ! chordMidiNotes.isEmpty())
                {
                    int chordSize = chordMidiNotes.size();
                    int wrappedDegree = ((note.degree.value() % chordSize) + chordSize) % chordSize;
                    int octaveShift = (int)std::floor((float)note.degree.value() / chordSize);
                    midiNote = chordMidiNotes[wrappedDegree] + (octaveShift * 12);
                }

                int absNoteTick = currentTickOffset + noteStartInBlock;
                float noteX = startX + (absNoteTick / (double)totalTicks) * (endX - startX);

                int pitch = (midiNote - 60) % 12, octave = (midiNote - 60) / 12; if (pitch < 0) { pitch += 12; octave -= 1; }
                int step = octave * 7 + (flatSpelling ? stepInOctaveFlat[pitch] : stepInOctave[pitch]);
                int acc = flatSpelling ? accidentalInOctaveFlat[pitch] : accidentalInOctave[pitch];
                
                if (lane.isDrums) { step = 6; acc = 0; }
                else if (useBassClef) { step += 12; } // Shift visual position up to F-Clef staff
                
                int noteY = centerY - (step - 6) * 4.0f;

                pdf.setFillColor (0xff1e1e2e); pdf.fillEllipse (noteX, noteY, 3.25f, 2.25f);
                pdf.setStrokeColor (0xff1e1e2e); pdf.setLineWidth (1.0f);
                if (step < 6) pdf.drawLine (noteX + 3.25f, noteY, noteX + 3.25f, noteY - 28);
                else pdf.drawLine (noteX - 3.25f, noteY, noteX - 3.25f, noteY + 28);

                if (acc == 1) { pdf.setFillColor (0xff1e1e2e); pdf.drawText ("#", noteX - 18, noteY + 3.5f, "Helvetica-Bold", 10.0f, true, false); }
                else if (acc == -1) { pdf.setFillColor (0xff1e1e2e); pdf.drawText ("b", noteX - 16, noteY + 3.5f, "Times-Bold", 10.0f, true, false); }

                pdf.setStrokeColor (0xff78716c); pdf.setLineWidth (0.75f);
                if (step <= 0) for (int s = 0; s >= step; s -= 2) pdf.drawLine (noteX - 8, centerY - (s - 6) * 4.0f, noteX + 8, centerY - (s - 6) * 4.0f);
                else if (step >= 12) for (int s = 12; s <= step; s += 2) pdf.drawLine (noteX - 8, centerY - (s - 6) * 4.0f, noteX + 8, centerY - (s - 6) * 4.0f);
            }
        }
        }
        currentTickOffset += blockDurationTicks;
    }
}

// Sheet Music PDF Export
inline void performSheetMusicExport (const ChordArrangement& arrangement, bool exportFullSheet)
{
    SimplePdfWriter pdf (1200, 400); bool isFirstPage = true;

    for (size_t secIdx = 0; secIdx < arrangement.sections.size(); ++secIdx)
    {
        auto& sec = arrangement.sections[secIdx]; juce::Array<ChordBlock> chords;
        for (auto& cb : sec.blocks) chords.add (cb);
        if (chords.isEmpty()) continue;

        if (! isFirstPage) pdf.newPage(); isFirstPage = false;
        drawChordsOnlyPage (pdf, sec.sectionName, chords, (float) sec.bpm, sec.currentKey);

        if (exportFullSheet)
        {
            for (int i = 0; i < (int) sec.tracks.size(); ++i)
            {
                auto& lane = sec.tracks[i];
                if (lane.enabled && ! lane.patternId.isEmpty()) { pdf.newPage(); drawTrackPage (pdf, sec.sectionName, chords, lane, i, (float) sec.bpm, sec.currentKey); }
            }
        }
    }

    juce::File pdfFile = getExportDirectory().getChildFile ("Chordcraft_Studio_SheetMusic.pdf");
    if (pdfFile.existsAsFile()) pdfFile.deleteFile();

    std::unique_ptr<juce::FileOutputStream> stream = pdfFile.createOutputStream();
    if (stream != nullptr && stream->openedOk())
    {
        juce::String pdfData = pdf.getPdfData();
        stream->write (pdfData.toRawUTF8(), pdfData.getNumBytesAsUTF8());
        stream.reset();

        saveExportedFile (pdfFile);
    }
}