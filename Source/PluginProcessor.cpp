#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>
#include <cstring>

namespace
{
const char* kName[TR909AudioProcessor::kVoices] = {
    "bd", "sd", "lt", "mt", "ht", "rs", "cp", "ch", "oh", "cr", "rd"
};
const float kTrim[TR909AudioProcessor::kVoices] = {
    1.0f, 0.82f, 0.74f, 0.72f, 0.70f, 0.58f, 0.66f, 0.34f, 0.38f, 0.42f, 0.32f
};
const float kLevel[TR909AudioProcessor::kVoices] = {
    86, 74, 70, 68, 66, 62, 70, 55, 58, 48, 42
};

juce::String stepId(int voice, int step)
{
    return juce::String(kName[voice]) + juce::String(step);
}
juce::String levelId(int voice) { return juce::String("lvl") + kName[voice]; }
juce::String accentId(int step) { return "ac" + juce::String(step); }

int mod(int n, int m)
{
    if (m <= 0) return 0;
    int r = n % m;
    return r < 0 ? r + m : r;
}
}

TR909AudioProcessor::TR909AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "TR909", makeLayout())
{
    loadSamples();
}

juce::AudioProcessorValueTreeState::ParameterLayout TR909AudioProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "swing", 1 }, "Swing", 0, 100, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "last", 1 }, "Last Step", 1, 16, 16));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "accent", 1 }, "Accent", 0.0f, 1.0f, 0.72f));
    for (int v = 0; v < kVoices; ++v)
    {
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { levelId(v), 1 }, juce::String(kName[v]).toUpperCase() + " Level", 0, 100, (int) kLevel[v]));
        for (int s = 0; s < kSteps; ++s)
            layout.add(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID { stepId(v, s), 1 },
                juce::String(kName[v]).toUpperCase() + " " + juce::String(s + 1),
                0, 2, houseCell(v, s)));
    }
    for (int s = 0; s < kSteps; ++s)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { accentId(s), 1 }, "Accent " + juce::String(s + 1), houseAccent(s)));
    return layout;
}

int TR909AudioProcessor::houseCell(int voice, int step)
{
    auto on = [](const char* spec, int i, int hot) {
        if (i < 0 || i >= 16 || spec[i] == '.' || spec[i] == 0) return 0;
        return (spec[i] == 'X' || hot) ? 2 : 1;
    };
    const char* spec = "................";
    switch (voice)
    {
        case 0: spec = "X...X...X...X..."; break;
        case 1: spec = "....X.......X..."; break;
        case 6: spec = "....X.......X..."; break;
        case 7: spec = "x.x.x.x.x.x.x.x."; break;
        case 8: spec = "..x...x...x...x."; break;
        case 5: spec = "........x......."; break;
        default: break;
    }
    int cell = on(spec, step, voice == 0 || voice == 1 || voice == 6);
    if (voice == 7)
    {
        const char* oh = "..x...x...x...x.";
        if (oh[step] != '.') cell = 0;
    }
    return cell;
}

bool TR909AudioProcessor::houseAccent(int step) { return step == 0; }

void TR909AudioProcessor::loadSamples()
{
    struct Blob { const char* data; int size; };
    const Blob blobs[kVoices] = {
        { BinaryData::bd_wav, BinaryData::bd_wavSize },
        { BinaryData::sd_wav, BinaryData::sd_wavSize },
        { BinaryData::lt_wav, BinaryData::lt_wavSize },
        { BinaryData::mt_wav, BinaryData::mt_wavSize },
        { BinaryData::ht_wav, BinaryData::ht_wavSize },
        { BinaryData::rs_wav, BinaryData::rs_wavSize },
        { BinaryData::cp_wav, BinaryData::cp_wavSize },
        { BinaryData::ch_wav, BinaryData::ch_wavSize },
        { BinaryData::oh_wav, BinaryData::oh_wavSize },
        { BinaryData::cr_wav, BinaryData::cr_wavSize },
        { BinaryData::rd_wav, BinaryData::rd_wavSize },
    };
    juce::WavAudioFormat wav;
    for (int v = 0; v < kVoices; ++v)
    {
        auto stream = std::make_unique<juce::MemoryInputStream>(blobs[v].data, (size_t) blobs[v].size, false);
        std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(stream.release(), true));
        if (reader == nullptr) continue;
        const int n = (int) reader->lengthInSamples;
        samples[(size_t) v].assign((size_t) n, 0.0f);
        juce::AudioBuffer<float> buf(1, n);
        reader->read(&buf, 0, n, 0, true, false);
        std::memcpy(samples[(size_t) v].data(), buf.getReadPointer(0), sizeof(float) * (size_t) n);
    }
}

void TR909AudioProcessor::prepareToPlay(double rate, int)
{
    sampleRate = rate > 0.0 ? rate : 44100.0;
    hasLastBeat = false;
    for (auto& shot : shots) shot.on = false;
}

bool TR909AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

float TR909AudioProcessor::param(const juce::String& id) const
{
    if (auto* raw = apvts.getRawParameterValue(id)) return raw->load();
    return 0.0f;
}

int TR909AudioProcessor::voiceForNote(int note) const
{
    switch (note)
    {
        case 36: return 0;
        case 38: return 1;
        case 41: return 2;
        case 45: return 3;
        case 47: return 4;
        case 37: return 5;
        case 39: return 6;
        case 42: return 7;
        case 46: return 8;
        case 49: return 9;
        case 51: return 10;
        default: return -1;
    }
}

void TR909AudioProcessor::trigger(int voice, float amp, int)
{
    if (voice < 0 || voice >= kVoices) return;
    const auto& data = samples[(size_t) voice];
    if (data.empty() || amp < 0.008f) return;
    if (voice == 7)
        for (auto& shot : shots)
            if (shot.on && shot.voice == 8) shot.on = false;

    Shot* slot = nullptr;
    for (auto& shot : shots)
        if (shot.on && shot.voice == voice) { slot = &shot; break; }
    if (slot == nullptr)
        for (auto& shot : shots)
            if (!shot.on) { slot = &shot; break; }
    if (slot == nullptr) slot = &shots[0];

    slot->data = data.data();
    slot->length = (int) data.size();
    slot->pos = 0.0;
    slot->inc = 44100.0 / sampleRate;
    slot->gain = amp;
    slot->voice = voice;
    slot->on = true;
}

void TR909AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    bool playing = false;
    double ppq = 0.0;
    double bpm = 120.0;
    if (auto* head = getPlayHead())
    {
        if (auto pos = head->getPosition())
        {
            playing = pos->getIsPlaying();
            ppq = pos->getPpqPosition().orFallback(0.0);
            bpm = pos->getBpm().orFallback(120.0);
        }
    }
    if (bpm < 20.0) bpm = 120.0;

    const int n = buffer.getNumSamples();
    const double beatsPerSample = (bpm / 60.0) / sampleRate;
    const double blockStart = ppq;
    const int length = juce::jlimit(1, kSteps, (int) std::round(param("last")));
    const double amount = (juce::jlimit(0.0f, 100.0f, param("swing")) / 100.0) * 0.55;
    const double longBeats = 0.25 * (1.0 + amount);
    const float accentAmt = juce::jlimit(0.0f, 1.0f, param("accent"));

    auto fireStep = [&](int step, int)
    {
        if (step < 0 || step >= length) return;
        playStep.store(step);
        const bool accented = param(accentId(step)) > 0.5f;
        const int ch = (int) std::round(param(stepId(7, step)));
        for (int v = 0; v < kVoices; ++v)
        {
            const int cell = (int) std::round(param(stepId(v, step)));
            if (cell <= 0) continue;
            if (v == 8 && ch > 0) continue;
            float vel = cell >= 2 ? 0.96f : 0.74f;
            if (accented) vel += 0.28f * accentAmt;
            const float level = juce::jlimit(0.0f, 100.0f, param(levelId(v))) / 100.0f;
            const float amp = std::min(1.25f, vel * level * kTrim[v]);
            trigger(v, amp, 0);
        }
    };

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (!msg.isNoteOn()) continue;
        const int voice = voiceForNote(msg.getNoteNumber());
        if (voice < 0) continue;
        const float vel = msg.getFloatVelocity();
        const float level = juce::jlimit(0.0f, 100.0f, param(levelId(voice))) / 100.0f;
        trigger(voice, std::min(1.25f, vel * level * kTrim[voice]), meta.samplePosition);
    }

    if (playing)
    {
        if (!hasLastBeat) lastBeat = blockStart - beatsPerSample;
        for (int i = 0; i < n; ++i)
        {
            const double beat = blockStart + (double) i * beatsPerSample;
            const double prev = (i == 0) ? lastBeat : (blockStart + (double) (i - 1) * beatsPerSample);
            const auto secNow = (long long) std::floor(beat * 4.0 + 1.0e-9);
            const auto secPrev = (long long) std::floor(prev * 4.0 + 1.0e-9);
            if (secNow != secPrev && (secNow % 2) == 0)
                fireStep(mod((int) secNow, length), i);

            const long long pair = (long long) std::floor(beat / 0.5);
            const double oddBeat = (double) pair * 0.5 + longBeats;
            if (length > 1 && prev < oddBeat && beat >= oddBeat)
            {
                const int step = mod((int) (pair * 2 + 1), length);
                if ((step % 2) == 1) fireStep(step, i);
            }
        }
        lastBeat = blockStart + (double) (n - 1) * beatsPerSample;
        hasLastBeat = true;
    }
    else
    {
        hasLastBeat = false;
        playStep.store(-1);
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        float sum = 0.0f;
        for (auto& shot : shots)
        {
            if (!shot.on || shot.data == nullptr) continue;
            const int i0 = (int) shot.pos;
            if (i0 >= shot.length - 1) { shot.on = false; continue; }
            const float frac = (float) (shot.pos - (double) i0);
            const float s = shot.data[i0] * (1.0f - frac) + shot.data[i0 + 1] * frac;
            sum += s * shot.gain;
            shot.pos += shot.inc;
        }
        sum = std::tanh(sum * 0.9f) * 0.8f;
        left[i] = sum;
        if (right != nullptr) right[i] = sum;
    }
}

juce::AudioProcessorEditor* TR909AudioProcessor::createEditor()
{
    return new TR909AudioProcessorEditor(*this);
}

void TR909AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void TR909AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TR909AudioProcessor();
}
