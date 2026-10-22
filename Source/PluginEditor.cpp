#include "PluginEditor.h"

namespace
{
const char* kShort[TR909AudioProcessor::kVoices] = {
    "BASS DRUM", "SNARE DRUM", "LOW TOM", "MID TOM", "HI TOM",
    "RIM SHOT", "HAND CLAP", "CLOSED HAT", "OPEN HAT", "CRASH", "RIDE"
};

juce::String stepId(int voice, int step)
{
    const char* names[] = { "bd", "sd", "lt", "mt", "ht", "rs", "cp", "ch", "oh", "cr", "rd" };
    return juce::String(names[voice]) + juce::String(step);
}
juce::String levelId(int voice)
{
    const char* names[] = { "bd", "sd", "lt", "mt", "ht", "rs", "cp", "ch", "oh", "cr", "rd" };
    return juce::String("lvl") + names[voice];
}

void setIntParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, int value)
{
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id)))
    {
        ranged->beginChangeGesture();
        ranged->setValueNotifyingHost(ranged->convertTo0to1((float) value));
        ranged->endChangeGesture();
    }
}

int getInt(juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
{
    if (auto* raw = apvts.getRawParameterValue(id))
        return (int) std::round(raw->load());
    return 0;
}
}

TR909AudioProcessorEditor::TR909AudioProcessorEditor(TR909AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1000, 700);
    setResizable(false, false);
    startTimerHz(30);
}

void TR909AudioProcessorEditor::timerCallback() { repaint(); }

juce::Rectangle<float> TR909AudioProcessorEditor::grid() const
{
    return { 158.0f, 168.0f, 820.0f, 470.0f };
}

int TR909AudioProcessorEditor::stepAt(juce::Point<float> pos) const
{
    auto g = grid();
    if (!g.contains(pos)) return -1;
    const int step = (int) ((pos.x - g.getX()) / (g.getWidth() / 16.0f));
    return juce::jlimit(0, 15, step);
}

int TR909AudioProcessorEditor::rowAt(juce::Point<float> pos) const
{
    auto g = grid();
    if (pos.x < 16.0f || pos.x > g.getRight()) return -2;
    const float top = g.getY();
    const float rowH = 34.0f;
    if (pos.y >= top && pos.y < top + 22.0f) return -3;
    if (pos.y >= top + 22.0f && pos.y < top + 22.0f + rowH) return -1;
    for (int v = 0; v < TR909AudioProcessor::kVoices; ++v)
    {
        const float y = top + 22.0f + rowH + (float) v * rowH;
        if (pos.y >= y && pos.y < y + rowH) return v;
    }
    return -2;
}

void TR909AudioProcessorEditor::editAt(juce::Point<float> pos, bool drag)
{
    const int step = stepAt(pos);
    const int row = rowAt(pos);
    if (step < 0 || row < -1) return;
    const int length = juce::jlimit(1, 16, getInt(processor.apvts, "last"));
    if (step >= length) return;

    if (row == -1)
    {
        const int cur = getInt(processor.apvts, "ac" + juce::String(step)) > 0 ? 1 : 0;
        const int next = drag ? dragValue : (cur ? 0 : 1);
        if (!drag) { dragValue = next; dragRow = -1; }
        if (drag && dragRow != -1) return;
        setIntParam(processor.apvts, "ac" + juce::String(step), next);
        return;
    }

    const auto id = stepId(row, step);
    const int cur = getInt(processor.apvts, id);
    int next = drag ? dragValue : (cur + 1) % 3;
    if (!drag) { dragValue = next; dragRow = row; }
    if (drag && dragRow != row) return;
    setIntParam(processor.apvts, id, next);
}

void TR909AudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.position;
    if (pos.y >= 92.0f && pos.y <= 132.0f && pos.x >= 760.0f && pos.x <= 980.0f)
    {
        const int swing = juce::jlimit(0, 100, (int) std::round((pos.x - 760.0f) / 220.0f * 100.0f));
        setIntParam(processor.apvts, "swing", swing);
        return;
    }
    if (pos.y >= 92.0f && pos.y <= 132.0f && pos.x >= 620.0f && pos.x <= 740.0f)
    {
        const int last = juce::jlimit(1, 16, 1 + (int) ((pos.x - 620.0f) / 120.0f * 15.0f));
        setIntParam(processor.apvts, "last", last);
        return;
    }
    editAt(pos, false);
}

void TR909AudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.position;
    if (pos.y >= 92.0f && pos.y <= 150.0f && pos.x >= 760.0f)
    {
        const int swing = juce::jlimit(0, 100, (int) std::round((pos.x - 760.0f) / 220.0f * 100.0f));
        setIntParam(processor.apvts, "swing", swing);
        return;
    }
    editAt(pos, true);
}

void TR909AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff161513));
    auto chassis = getLocalBounds().toFloat().reduced(10.0f);
    g.setColour(juce::Colour(0xffb7b3a8));
    g.fillRoundedRectangle(chassis, 12.0f);
    auto face = chassis.reduced(8.0f);
    juce::ColourGradient grad(juce::Colour(0xffefeee8), face.getX(), face.getY(),
                              juce::Colour(0xffdad6cb), face.getX(), face.getBottom(), false);
    grad.addColour(0.28, juce::Colour(0xffe4e1d8));
    g.setGradientFill(grad);
    g.fillRoundedRectangle(face, 8.0f);

    g.setColour(juce::Colour(0xfff15a22));
    g.fillRect(face.getX() + 12.0f, 78.0f, face.getWidth() - 24.0f, 2.0f);
    g.setColour(juce::Colour(0xfff15a22));
    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    g.drawText("Roland", 28, 18, 160, 22, juce::Justification::left);
    g.setColour(juce::Colour(0xff243044));
    g.setFont(juce::Font(juce::FontOptions(48.0f, juce::Font::bold)));
    g.drawText("TR-909", 24, 34, 280, 48, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText("RHYTHM COMPOSER", face.getRight() - 250.0f, 48.0f, 220.0f, 18.0f, juce::Justification::right);

    const int swing = getInt(processor.apvts, "swing");
    const int last = juce::jlimit(1, 16, getInt(processor.apvts, "last"));
    g.setColour(juce::Colour(0xff667089));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("LAST  " + juce::String(last), 620, 96, 120, 16, juce::Justification::left);
    g.drawText("SWING  " + juce::String(swing) + "%", 760, 96, 160, 16, juce::Justification::left);
    g.setColour(juce::Colour(0xffc9c4b6));
    g.fillRoundedRectangle(620, 116, 120, 8, 3);
    g.fillRoundedRectangle(760, 116, 220, 8, 3);
    g.setColour(juce::Colour(0xfff15a22));
    g.fillRoundedRectangle(620, 116, 120.0f * ((last - 1) / 15.0f), 8, 3);
    g.fillRoundedRectangle(760, 116, 220.0f * (swing / 100.0f), 8, 3);

    auto board = juce::Rectangle<float>(24.0f, 150.0f, 952.0f, 490.0f);
    g.setColour(juce::Colour(0xff2b3448));
    g.fillRoundedRectangle(board, 4.0f);

    auto area = grid();
    const float stepW = area.getWidth() / 16.0f;
    const int now = processor.playStep.load();
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    for (int s = 0; s < 16; ++s)
    {
        const float x = area.getX() + (float) s * stepW;
        g.setColour(s % 4 == 0 ? juce::Colour(0xfff4efe6) : juce::Colour(0xff8e98ad));
        g.drawText(juce::String(s + 1), x, area.getY(), stepW, 14.0f, juce::Justification::centred);
        g.setColour(now == s ? juce::Colour(0xffff3b34) : juce::Colour(0xff4a2228));
        g.fillEllipse(x + stepW * 0.5f - 4.0f, area.getY() + 14.0f, 8.0f, 8.0f);
    }

    auto drawRow = [&](int row, const juce::String& label, float y)
    {
        g.setColour(juce::Colour(0xffffd7c2));
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(label, 28.0f, y, 124.0f, 30.0f, juce::Justification::centredRight);
        for (int s = 0; s < 16; ++s)
        {
            const float x = area.getX() + (float) s * stepW + 3.0f;
            const float w = stepW - 6.0f;
            int cell = 0;
            if (row < 0) cell = getInt(processor.apvts, "ac" + juce::String(s)) > 0 ? 1 : 0;
            else cell = getInt(processor.apvts, stepId(row, s));
            const bool dead = s >= last;
            auto key = juce::Rectangle<float>(x, y + 6.0f, w, 22.0f);
            juce::Colour fill = juce::Colour(0xffddd4c4);
            if (cell == 1) fill = juce::Colour(0xfff0e2cc);
            if (cell >= 2) fill = juce::Colour(0xfff0a57a);
            if (row < 0 && cell > 0) fill = juce::Colour(0xfff0a57a);
            g.setColour(fill.withMultipliedAlpha(dead ? 0.35f : 1.0f));
            g.fillRoundedRectangle(key, 2.0f);
            g.setColour(juce::Colours::white.withAlpha(dead ? 0.15f : 0.7f));
            g.fillRect(key.getX(), key.getY(), key.getWidth(), 1.0f);
            juce::Colour lamp = juce::Colour(0xff4a2228);
            if (row < 0 && cell > 0) lamp = juce::Colour(0xfff15a22);
            else if (cell == 1) lamp = juce::Colour(0xffff3b34);
            else if (cell >= 2) lamp = juce::Colour(0xfffff4ee);
            if (now == s) lamp = juce::Colours::white;
            g.setColour(lamp);
            g.fillEllipse(x + w * 0.5f - 3.0f, y, 6.0f, 6.0f);
        }
    };

    drawRow(-1, "ACCENT", area.getY() + 24.0f);
    for (int v = 0; v < TR909AudioProcessor::kVoices; ++v)
        drawRow(v, kShort[v], area.getY() + 24.0f + 34.0f + (float) v * 34.0f);

    g.setColour(juce::Colour(0xff667089));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("Press Play in Live. Pads cycle off, hit, accent. MIDI C1 kick, D1 snare, F#1 hat.",
               24, 656, 760, 20, juce::Justification::left);

    float x = 24.0f;
    for (int v = 0; v < TR909AudioProcessor::kVoices; ++v)
    {
        const int level = getInt(processor.apvts, levelId(v));
        g.setColour(juce::Colour(0xff667089));
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        juce::ignoreUnused(level, x);
    }
}
