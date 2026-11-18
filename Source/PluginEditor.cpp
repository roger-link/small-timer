#include "PluginEditor.h"

namespace
{
const juce::Colour ink { 0xffddd7c8 }, amber { 0xffe9b56a }, dim { 0xff777b78 };
void drawSignal(juce::Graphics& graphics, const juce::Path& wave, juce::Rectangle<int> lane, juce::Colour glow, juce::Colour core)
{
    juce::Graphics::ScopedSaveState saved(graphics);
    graphics.reduceClipRegion(lane);
    const std::array<float, 4> widths { 8, 5, 3, 1.4f };
    const std::array<float, 4> opacity { 0.025f, 0.05f, 0.12f, 0.65f };
    for (size_t layer = 0; layer < widths.size(); ++layer)
    {
        graphics.setColour(glow.withAlpha(opacity[layer]));
        graphics.strokePath(wave, juce::PathStrokeType(widths[layer], juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    graphics.setColour(core);
    graphics.strokePath(wave, juce::PathStrokeType(0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
const juce::Image& panelTexture()
{
    static const auto texture = []
    {
        juce::Image image(juce::Image::ARGB, 1160, 1476, true);
        juce::Graphics graphics(image);
        juce::Random random(731);
        for (int speck = 0; speck < 440000; ++speck)
        {
            graphics.setColour((speck % 2 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha(0.06f));
            graphics.fillRect(random.nextInt(1160), random.nextInt(1476), 1, 1);
        }
        return image;
    }();
    return texture;
}
const juce::Image& dialFace()
{
    static const auto face = []
    {
        juce::Image image(juce::Image::ARGB, 256, 256, true);
        juce::Graphics graphics(image);
        for (int sector = 0; sector < 720; ++sector)
        {
            const auto angle = sector * juce::MathConstants<float>::twoPi / 720;
            const auto next = angle + 0.011f;
            const auto reflection = 0.5f + 0.5f * std::cos(2 * angle + 0.6f);
            const auto shade = juce::Colour(0xff675e54).interpolatedWith(juce::Colour(0xffe4d7c7), reflection);
            graphics.setColour(shade);
            juce::Path wedge;
            wedge.addTriangle(128, 128, 128 + std::sin(angle) * 126, 128 + std::cos(angle) * 126,
                              128 + std::sin(next) * 126, 128 + std::cos(next) * 126);
            graphics.fillPath(wedge);
        }
        graphics.setColour(juce::Colours::white.withAlpha(0.07f));
        for (int ring = 4; ring < 125; ring += 2)
            graphics.drawEllipse(128.0f - ring, 128.0f - ring, ring * 2.0f, ring * 2.0f, 0.45f);
        return image;
    }();
    return face;
}
bool hardwareButton(const juce::Button& button)
{
    return button.getHeight() > 25;
}
bool topButton(const juce::Button& button)
{
    return button.getY() < 160;
}
juce::Rectangle<float> buttonCap(const juce::Button& button)
{
    return topButton(button)
        ? juce::Rectangle<float>(button.getWidth() * 0.5f - 17, 25, 34, 34)
        : juce::Rectangle<float>(3, 2, 32, 32);
}
void drawHardwareButton(juce::Graphics& graphics, juce::Button& button, bool over, bool down)
{
    if (hardwareButton(button))
    {
        auto cap = buttonCap(button);
        const bool active = button.getToggleState();
        const bool illuminated = active && button.getButtonText() != "REC" && button.getButtonText() != "PLAY";
        for (int shadow = 4; shadow > 0; --shadow)
        {
            graphics.setColour(juce::Colours::black.withAlpha(0.12f));
            graphics.fillRoundedRectangle(cap.expanded(static_cast<float>(shadow)).translated(0, 3), 5);
        }
        graphics.setColour(juce::Colour(0xff171512));
        graphics.fillRoundedRectangle(cap.expanded(1.2f), 4);
        if (down)
            cap = cap.translated(0, 1.5f);
        juce::ColourGradient surface(juce::Colour(illuminated ? 0xffffd46a : 0xffc2b5a4), cap.getTopLeft(), juce::Colour(illuminated ? 0xffa65b09 : 0xff756755), cap.getBottomRight(), false);
        surface.addColour(0.15, juce::Colour(illuminated ? 0xffffc34a : 0xffbcae9d));
        surface.addColour(0.8, juce::Colour(illuminated ? 0xffe78a12 : 0xffa39583));
        graphics.setGradientFill(surface);
        graphics.fillRoundedRectangle(cap, 4);
        if (illuminated)
        {
            juce::ColourGradient light(juce::Colour(0xffffe17a), cap.getCentre().translated(0, -2), juce::Colour(0xffffad21).withAlpha(0.0f), cap.getCentre().translated(21, 0), true);
            light.addColour(0.5, juce::Colour(0xffffcb42).withAlpha(0.8f));
            graphics.setGradientFill(light);
            graphics.fillRoundedRectangle(cap.reduced(2), 3);
        }
        if (illuminated && (button.getButtonText() == "NEW SEED" || button.getButtonText() == "WANDER"))
        {
            const auto fade = 1.0f - juce::jlimit(0.0f, 1.0f, static_cast<float>(button.getProperties().getWithDefault("lampLevel", 0.0)));
            juce::ColourGradient metal(juce::Colour(0xffc2b5a4).withAlpha(fade), cap.getTopLeft(), juce::Colour(0xff756755).withAlpha(fade), cap.getBottomRight(), false);
            metal.addColour(0.15, juce::Colour(0xffbcae9d).withAlpha(fade));
            metal.addColour(0.8, juce::Colour(0xffa39583).withAlpha(fade));
            graphics.setGradientFill(metal);
            graphics.fillRoundedRectangle(cap, 4);
        }
        graphics.setColour(juce::Colour(0xfff5e3cc).withAlpha(over ? 0.8f : 0.42f));
        graphics.drawRoundedRectangle(cap.reduced(1.2f), 2.8f, 0.65f);
        graphics.setColour(juce::Colour(0xfff9ead3).withAlpha(0.65f));
        graphics.drawLine(cap.getX() + 4, cap.getY() + 1, cap.getRight() - 4, cap.getY() + 1, 0.7f);
        graphics.setColour(juce::Colours::black.withAlpha(0.3f));
        graphics.drawLine(cap.getX() + 3, cap.getBottom() - 2, cap.getRight() - 3, cap.getBottom() - 2, 2);
        const auto centre = cap.getCentre();
        const auto name = button.getButtonText();
        if (topButton(button))
        {
            juce::Path engraving;
            if (name == "LOOP")
            {
                engraving.startNewSubPath(centre.x - 7, centre.y - 1);
                engraving.cubicTo(centre.x - 8, centre.y - 9, centre.x + 5, centre.y - 10, centre.x + 7, centre.y - 3);
                engraving.startNewSubPath(centre.x + 3, centre.y - 5);
                engraving.lineTo(centre.x + 7, centre.y - 3);
                engraving.lineTo(centre.x + 9, centre.y - 7);
                engraving.startNewSubPath(centre.x + 7, centre.y + 1);
                engraving.cubicTo(centre.x + 8, centre.y + 9, centre.x - 5, centre.y + 10, centre.x - 7, centre.y + 3);
                engraving.startNewSubPath(centre.x - 3, centre.y + 5);
                engraving.lineTo(centre.x - 7, centre.y + 3);
                engraving.lineTo(centre.x - 9, centre.y + 7);
            }
            else if (name == "LITTLE FOREVER")
            {
                for (int point = 0; point <= 80; ++point)
                {
                    const auto angle = point * juce::MathConstants<float>::twoPi / 80;
                    const auto position = centre + juce::Point<float>(std::sin(angle) * 9, std::sin(2 * angle) * 4.5f);
                    if (point == 0) engraving.startNewSubPath(position);
                    else engraving.lineTo(position);
                }
                engraving.closeSubPath();
            }
            else if (name == "WANDER")
            {
                engraving.startNewSubPath(centre.x - 8, centre.y + 6);
                engraving.cubicTo(centre.x + 5, centre.y + 8, centre.x - 5, centre.y - 1, centre.x + 3, centre.y - 1);
                engraving.cubicTo(centre.x + 12, centre.y, centre.x - 6, centre.y - 9, centre.x + 7, centre.y - 7);
            }
            else
            {
                engraving.startNewSubPath(centre.x - 10, centre.y + 2);
                engraving.lineTo(centre.x - 7, centre.y + 2);
                engraving.lineTo(centre.x - 4, centre.y - 5);
                engraving.lineTo(centre.x - 1, centre.y + 6);
                engraving.startNewSubPath(centre.x + 2, centre.y - 6);
                engraving.lineTo(centre.x + 5, centre.y + 2);
                engraving.lineTo(centre.x + 10, centre.y + 2);
            }
            juce::Path symbol;
            juce::PathStrokeType(1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath(symbol, engraving);
            const auto level = static_cast<float>(button.getProperties().getWithDefault("lampLevel", 1.0));
            graphics.setColour(juce::Colour(0xff58442c));
            graphics.strokePath(symbol, juce::PathStrokeType(1.1f));
            if (active)
                juce::DropShadow(amber.withAlpha(0.65f * level), 5, {}).drawForPath(graphics, symbol);
            graphics.setColour(active ? juce::Colour(0xffffefd0).withMultipliedBrightness(0.8f + 0.2f * level) : juce::Colour(0xff584b38));
            graphics.fillPath(symbol);
            return;
        }
        if (name == "HOLD" || name == "REC" || name == "PLAY" || name == "RECALL" || name == "NEW SEED")
        {
            juce::Path symbol;
            const auto colour = name == "REC" ? juce::Colour(0xffff6650) : name == "PLAY" ? juce::Colour(0xff81f2ac) : juce::Colour(0xffffe6a0);
            const auto level = static_cast<float>(button.getProperties().getWithDefault("lampLevel", 1.0));
            if (name == "NEW SEED")
            {
                juce::Path branches;
                branches.startNewSubPath(centre.x, centre.y + 7);
                branches.lineTo(centre.x, centre.y - 6);
                branches.startNewSubPath(centre.x - 6, centre.y - 4);
                branches.lineTo(centre.x, centre.y + 1);
                branches.lineTo(centre.x + 6, centre.y - 5);
                branches.startNewSubPath(centre.x - 6, centre.y + 3);
                branches.lineTo(centre.x, centre.y + 5);
                branches.lineTo(centre.x + 6, centre.y + 1);
                juce::PathStrokeType(1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath(symbol, branches);
                symbol.addEllipse(centre.x - 1.5f, centre.y - 8, 3, 3);
                symbol.addEllipse(centre.x - 8, centre.y - 6, 3, 3);
                symbol.addEllipse(centre.x + 5, centre.y - 7, 3, 3);
            }
            else if (name == "HOLD")
            {
                symbol.addRoundedRectangle(centre.x - 5, centre.y - 6, 3, 12, 0.7f);
                symbol.addRoundedRectangle(centre.x + 2, centre.y - 6, 3, 12, 0.7f);
            }
            else if (name == "REC")
            {
                graphics.setColour(juce::Colour(0xff571b12));
                graphics.fillEllipse(centre.x - 6, centre.y - 6, 12, 12);
                symbol.addEllipse(centre.x - 4.5f, centre.y - 4.5f, 9, 9);
            }
            else if (name == "PLAY")
                symbol.addTriangle(centre.x - 4, centre.y - 6, centre.x - 4, centre.y + 6, centre.x + 7, centre.y);
            else
            {
                juce::Path arrow;
                arrow.startNewSubPath(centre.x + 3, centre.y + 5);
                arrow.cubicTo(centre.x + 10, centre.y + 1, centre.x + 5, centre.y - 7, centre.x - 4, centre.y - 3);
                arrow.lineTo(centre.x - 6, centre.y - 2);
                arrow.startNewSubPath(centre.x - 5, centre.y - 7);
                arrow.lineTo(centre.x - 6, centre.y - 2);
                arrow.lineTo(centre.x - 1, centre.y - 1);
                juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath(symbol, arrow);
            }
            graphics.setColour(juce::Colour(0xff463c2e));
            graphics.strokePath(symbol, juce::PathStrokeType(1.2f));
            if (active)
                juce::DropShadow(colour.withAlpha(0.65f * level), 6, {}).drawForPath(graphics, symbol);
            graphics.setColour(active && name == "NEW SEED" ? juce::Colour(0xff564937).interpolatedWith(colour, level)
                                      : active ? colour.withMultipliedBrightness(0.65f + 0.35f * level)
                                      : juce::Colour(name == "REC" ? 0xffa12c1c : name == "PLAY" ? 0xff245e40 : 0xff564937));
            graphics.fillPath(symbol);
            if (name == "REC")
            {
                graphics.setColour(juce::Colours::white.withAlpha(active ? 0.65f * level : 0.16f));
                graphics.fillEllipse(centre.x - 2, centre.y - 3, 2.5f, 1.8f);
            }
            return;
        }
        if (button.getButtonText() == "PLAY")
        {
            juce::Path triangle;
            triangle.addTriangle(centre.x - 4, centre.y - 6, centre.x - 4, centre.y + 6, centre.x + 7, centre.y);
            graphics.setColour(juce::Colour(active ? 0xff71e6a0 : 0xff186143));
            graphics.fillPath(triangle);
        }
        else if (active || button.getButtonText() == "REC")
        {
            const bool recording = button.getButtonText() == "REC";
            if (illuminated)
            {
                graphics.setGradientFill(juce::ColourGradient(juce::Colour(0xffffef85).withAlpha(0.8f), centre, juce::Colour(0xffffc127).withAlpha(0.0f), centre.translated(13, 0), true));
                graphics.fillEllipse(centre.x - 13, centre.y - 13, 26, 26);
            }
            graphics.setColour(juce::Colour(recording ? 0xff7e1d10 : 0xffc18a20));
            graphics.fillEllipse(centre.x - 5, centre.y - 5, 10, 10);
            graphics.setGradientFill(juce::ColourGradient(juce::Colour(recording ? (active ? 0xffff8871 : 0xffed4b2d) : 0xfffff3a2), centre.translated(-1, -2), juce::Colour(recording ? 0xffb5220e : 0xffffb918), centre.translated(3, 4), false));
            graphics.fillEllipse(centre.x - 4, centre.y - 4, 8, 8);
            if (illuminated)
            {
                graphics.setColour(juce::Colour(0xfffff9c1));
                graphics.drawEllipse(centre.x - 3.7f, centre.y - 3.7f, 7.4f, 7.4f, 0.7f);
                graphics.setColour(juce::Colours::white.withAlpha(0.7f));
                graphics.fillEllipse(centre.x - 2, centre.y - 2.5f, 2, 1.5f);
            }
        }
        return;
    }
}
void label(juce::Graphics& graphics, const juce::String& text, juce::Rectangle<int> bounds, float size, juce::Colour colour = ink)
{
    graphics.setColour(colour);
    auto font = juce::Font(juce::FontOptions("Arial Narrow", size, juce::Font::plain));
    font.setExtraKerningFactor(0.12f);
    graphics.setFont(font);
    graphics.drawText(text, bounds, juce::Justification::centred);
}
}
void HardwareLook::drawButtonBackground(juce::Graphics& graphics, juce::Button& button, const juce::Colour&, bool over, bool down)
{
    if (button.getButtonText() == "FREE" || button.getButtonText() == "LOCK")
    {
        const bool locked = button.getToggleState();
        auto track = button.getLocalBounds().toFloat().reduced(2, 3);
        if (locked)
        {
            juce::ColourGradient halo(amber.withAlpha(0.3f), track.getCentre(), amber.withAlpha(0.0f), track.getCentre().translated(23, 0), true);
            graphics.setGradientFill(halo);
            graphics.fillRoundedRectangle(track.expanded(3), 8);
        }
        graphics.setColour(juce::Colour(0xff0e0f0e));
        graphics.fillRoundedRectangle(track, 8);
        graphics.setColour(ink.withAlpha(over ? 0.45f : 0.19f));
        graphics.drawRoundedRectangle(track, 8, 0.7f);
        const auto thumbX = locked ? track.getRight() - 17 : track.getX() + 1;
        auto thumb = juce::Rectangle<float>(thumbX, track.getY() + 1, 16, track.getHeight() - 2);
        if (down)
            thumb = thumb.translated(0, 0.6f);
        graphics.setGradientFill(juce::ColourGradient(juce::Colour(locked ? 0xffffd680 : 0xffc0b29e), thumb.getTopLeft(), juce::Colour(locked ? 0xffc38a2b : 0xff716757), thumb.getBottomRight(), false));
        graphics.fillRoundedRectangle(thumb, 5);
        graphics.setColour(ink.withAlpha(0.5f));
        graphics.drawRoundedRectangle(thumb.reduced(0.7f), 4, 0.6f);
        const auto centre = thumb.getCentre();
        graphics.setColour(juce::Colour(0xff42382b));
        graphics.fillRoundedRectangle(centre.x - 3.5f, centre.y, 7, 5, 1);
        juce::Path shackle;
        shackle.startNewSubPath(centre.x - 2.4f, centre.y);
        shackle.lineTo(centre.x - 2.4f, centre.y - 2.2f);
        shackle.cubicTo(centre.x - 2.4f, centre.y - 5.5f, centre.x + 2.4f, centre.y - 5.5f, centre.x + 2.4f, centre.y - 2.2f);
        if (locked)
            shackle.lineTo(centre.x + 2.4f, centre.y);
        graphics.strokePath(shackle, juce::PathStrokeType(1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        graphics.setColour(juce::Colour(locked ? 0xffffedb7 : 0xffc4b59c));
        graphics.fillEllipse(centre.x - 0.7f, centre.y + 1.3f, 1.4f, 1.4f);
        return;
    }
    if (hardwareButton(button))
    {
        drawHardwareButton(graphics, button, over, down);
        return;
    }
    auto bounds = button.getLocalBounds().toFloat().reduced(1.5f);
    graphics.setColour(juce::Colours::black.withAlpha(0.5f));
    graphics.fillRoundedRectangle(bounds.translated(0, 2), 4);
    const bool active = button.getToggleState();
    graphics.setGradientFill(juce::ColourGradient(juce::Colour(down ? 0xff272a28 : 0xff464945), 0, bounds.getY(), juce::Colour(0xff242725), 0, bounds.getBottom(), false));
    graphics.fillRoundedRectangle(bounds, 4);
    graphics.setColour((active ? amber : ink).withAlpha(active ? 0.8f : over ? 0.4f : 0.16f));
    graphics.drawRoundedRectangle(bounds, 4, 1);
    if (active)
    {
        graphics.setColour(amber);
        graphics.fillRoundedRectangle(bounds.getCentreX() - 7, bounds.getBottom() - 5, 14, 2, 1);
    }
}
void HardwareLook::drawButtonText(juce::Graphics& graphics, juce::TextButton& button, bool, bool)
{
    if (button.getButtonText() == "FREE" || button.getButtonText() == "LOCK")
        return;
    if (hardwareButton(button))
    {
        const auto bounds = topButton(button)
            ? juce::Rectangle<int>(0, 0, button.getWidth(), 23)
            : juce::Rectangle<int>(40, 0, button.getWidth() - 40, 36);
        auto text = button.getButtonText();
        if (text == "LITTLE FOREVER") text = "LITTLE\nFOREVER";
        if (text == "SOFT FAILURE") text = "SOFT\nFAILURE";
        graphics.setColour(ink);
        auto font = juce::Font(juce::FontOptions("Arial Narrow", 11.0f, juce::Font::plain));
        font.setExtraKerningFactor(0.08f);
        graphics.setFont(font);
        graphics.drawFittedText(text, bounds, topButton(button) ? juce::Justification::centred : juce::Justification::centredLeft, 2);
        return;
    }
    label(graphics, button.getButtonText(), button.getLocalBounds().reduced(2), button.getHeight() < 25 ? 9.0f : 11.0f, button.isEnabled() ? (button.getToggleState() ? amber : ink) : dim.withAlpha(0.45f));
}
void HardwareLook::drawLinearSlider(juce::Graphics& graphics, int x, int y, int width, int height, float position, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const auto centre = static_cast<float>(x) + width * 0.5f;
    graphics.setColour(juce::Colour(0xff101411));
    graphics.fillRoundedRectangle(centre - 4, static_cast<float>(y), 8, static_cast<float>(height), 4);
    graphics.setColour(juce::Colours::white.withAlpha(0.08f));
    graphics.drawLine(centre + 5, static_cast<float>(y + 4), centre + 5, static_cast<float>(y + height - 4));
    for (int tick = 0; tick <= 12; ++tick)
    {
        const auto tickY = static_cast<float>(y) + height * tick / 12.0f;
        graphics.setColour(ink.withAlpha(tick == 6 ? 0.6f : 0.42f));
        graphics.drawLine(centre - 23, tickY, centre - (tick % 5 == 0 ? 12.0f : 16.0f), tickY);
        graphics.drawLine(centre + 14, tickY, centre + 20, tickY);
    }
    juce::Rectangle<float> cap(centre - 21, position - 14, 42, 28);
    for (int shadow = 5; shadow > 0; --shadow)
    {
        graphics.setColour(juce::Colours::black.withAlpha(0.09f));
        graphics.fillRoundedRectangle(cap.translated(1, 5).expanded(static_cast<float>(shadow)), 3);
    }
    juce::ColourGradient metal(juce::Colour(0xffe6d5bd), 0, cap.getY(), juce::Colour(0xff78654c), 0, cap.getBottom(), false);
    metal.addColour(0.3, juce::Colour(0xffc3b198));
    metal.addColour(0.48, juce::Colour(0xfff0e2ce));
    graphics.setGradientFill(metal);
    graphics.fillRoundedRectangle(cap, 2);
    graphics.saveState();
    graphics.reduceClipRegion(cap.reduced(2).toNearestInt());
    graphics.setColour(juce::Colour(0xff4f4031).withAlpha(0.35f));
    for (float offset = -cap.getHeight(); offset < cap.getWidth(); offset += 1.8f)
    {
        graphics.drawLine(cap.getX() + offset, cap.getBottom(), cap.getX() + offset + cap.getHeight(), cap.getY(), 0.35f);
        graphics.drawLine(cap.getX() + offset, cap.getY(), cap.getX() + offset + cap.getHeight(), cap.getBottom(), 0.35f);
    }
    graphics.restoreState();
    graphics.setColour(juce::Colour(0xff524333));
    graphics.fillRect(cap.getX() + 1, position - 1, cap.getWidth() - 2, 2.5f);
    graphics.setColour(juce::Colour(0xfff6ead7));
    graphics.fillRect(cap.getX() + 1, position - 1, cap.getWidth() - 2, 1.0f);
    graphics.setColour(juce::Colours::white.withAlpha(0.4f));
    graphics.drawRoundedRectangle(cap, 2, 0.7f);
}
void HardwareLook::drawRotarySlider(juce::Graphics& graphics, int x, int y, int width, int height, float value, float start, float end, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(8);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    for (int shadow = 12; shadow > 0; --shadow)
    {
        graphics.setColour(juce::Colours::black.withAlpha(0.045f));
        graphics.fillEllipse(bounds.translated(2, 7).expanded(static_cast<float>(shadow)));
    }
    graphics.setGradientFill(juce::ColourGradient(juce::Colour(0xffd6c5ad), bounds.getTopLeft(), juce::Colour(0xff665747), bounds.getBottomRight(), false));
    graphics.fillEllipse(bounds);
    for (int tick = 0; tick < 96; ++tick)
    {
        const auto angle = tick * juce::MathConstants<float>::twoPi / 96;
        graphics.setColour(juce::Colour(tick % 2 == 0 ? 0xffe4d4bf : 0xff332a22));
        graphics.drawLine(centre.x + std::sin(angle) * (radius - 4), centre.y + std::cos(angle) * (radius - 4), centre.x + std::sin(angle) * (radius - 1), centre.y + std::cos(angle) * (radius - 1), 1);
    }
    graphics.drawImage(dialFace(), bounds.reduced(5));
    graphics.setColour(ink.withAlpha(0.6f));
    graphics.drawEllipse(bounds.reduced(5), 0.7f);
    const auto angle = start + value * (end - start);
    graphics.setColour(juce::Colour(0xff302921));
    graphics.drawLine(centre.x + std::sin(angle) * radius * 0.45f, centre.y - std::cos(angle) * radius * 0.45f, centre.x + std::sin(angle) * radius * 0.8f, centre.y - std::cos(angle) * radius * 0.8f, 2);
}

SmallTimerEditor::SmallTimerEditor(SmallTimerProcessor& owner) : AudioProcessorEditor(owner), processor(owner)
{
    setLookAndFeel(&look);
    setSize(660, 750);
    setResizable(false, false);
    motor.position = processor.targets();
    lastMode = processor.mode();
    for (auto* button : { &loop, &forever, &motion, &soft, &hold, &rec, &play, &recall, &seed })
        addAndMakeVisible(button);
    loop.onClick = [this] { setMode(0); };
    forever.onClick = [this] { setMode(1); };
    loop.setRadioGroupId(1);
    forever.setRadioGroupId(1);
    auto toggle = [this](juce::TextButton& button, juce::String id)
    {
        button.onClick = [this, id] { processor.setValue(id, 1.0f - processor.value(id)); };
    };
    toggle(motion, "motion");
    toggle(soft, "soft");
    toggle(hold, "hold");
    rec.onClick = [this] { processor.toggleRecord(); };
    play.onClick = [this] { processor.togglePlay(); };
    recall.onClick = [this]
    {
        if (juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown())
            processor.storeRecall();
        else
        {
            processor.recall();
            recallLightUntil = juce::Time::getMillisecondCounterHiRes() + 450;
            recall.getProperties().set("lampLevel", 1.0);
            recall.setToggleState(true, juce::dontSendNotification);
            repaint();
        }
    };
    seed.onClick = [this]
    {
        processor.mutate();
        undoSeed.setEnabled(processor.canUndoMutation());
        seedBurstStarted = juce::Time::getMillisecondCounterHiRes();
        seed.setToggleState(true, juce::dontSendNotification);
        seed.getProperties().set("lampLevel", 1.0);
        repaint();
    };
    addAndMakeVisible(scopeGain);
    addAndMakeVisible(undoSeed);
    undoSeed.setTooltip("Undo the last NEW SEED in this mode. Locked or subsequently edited faders stay put. One step per mode; not saved between sessions.");
    undoSeed.onClick = [this]
    {
        if (processor.undoMutation())
        {
            seedBurstStarted = 0;
            seed.setToggleState(false, juce::dontSendNotification);
            seed.getProperties().set("lampLevel", 0.0);
        }
        undoSeed.setEnabled(processor.canUndoMutation());
        repaint();
    };
    addAndMakeVisible(scopeFreeze);
    scopeFreeze.setTitle("Freeze waveform display");
    scopeFreeze.setClickingTogglesState(true);
    scopeFreeze.setTooltip("Freeze both displayed waveforms for inspection. Click again to resume the current live signals. VIEW magnification still works. Audio, recording and Wander are unaffected; freeze is not saved.");
    scopeFreeze.onClick = [this] { repaint(); };
    scopeGain.setTitle("Waveform display gain");
    scopeGain.setTooltip("Display only: cycle x1, x2, x4, x8 magnification for BOTH signals. Audio is unchanged. Large signals may reach the display limits. Resets to x2 when reopened.");
    scopeGain.onClick = [this]
    {
        scopeGainStep = (scopeGainStep + 1) % 4;
        scopeGain.setButtonText("VIEW x" + juce::String(1 << scopeGainStep));
        repaint();
    };
    recall.setTooltip("Recall this mode's snapshot. Shift-click to store. Locked faders stay put.");
    seed.setTooltip("Explore a related Cloud, Fragments or Bloom gesture in this mode. MUTATE sets distance; locks stay put. UNDO SEED restores the previous seed.");
    motion.setTooltip("Independent switch, available in either mode: slow, bounded Wander around the current settings.");
    soft.setTooltip("Independent switch, available in either mode: darker, saturated repeats with slow wear in the feedback path.");
    loop.setTooltip("Mode: choose Loop OR Little Forever. Tape-style delay with adjustable feedback.");
    forever.setTooltip("Mode: choose Little Forever OR Loop. Long, gently decaying repeats; FEED controls how long they linger.");
    hold.setTooltip("Hold motor motion. Manual fader movement still works.");
    rec.setTooltip("Record up to 30 seconds into temporary RAM. Click again to finish. Audio is never saved.");
    play.setTooltip("Feed the temporary recording through the effects. Click again for live input.");
    for (int index = 0; index < 6; ++index)
    {
        auto& fader = faders[index];
        addAndMakeVisible(fader);
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        fader.setRange(0.0, 1.0, 0.001);
        fader.setValue(motor.position[index], juce::dontSendNotification);
        fader.setTitle(juce::StringArray { "Time", "Feed", "Space", "Drift", "Grain", "Mix" }[index]);
        const juce::StringArray meanings { "Delay: 40 ms to 2 seconds.", "Feedback: repeat length.", "Diffuse stereo ambience.", "Tape wow and flutter.", "Overlapping granular delay texture.", "Dry/wet blend. Zero is unprocessed audio." };
        fader.setTooltip(meanings[index] + " Drag overrides Wander for 1.5 seconds. Double-click for this mode's default.");
        fader.onDragStart = [this, index]
        {
            motor.dragging[index] = true;
            processor.setFaderDragging(index, true);
            gestureIDs[index] = SmallTimerProcessor::faderID(processor.mode(), index);
            processor.parameters.getParameter(gestureIDs[index])->beginChangeGesture();
        };
        fader.onValueChange = [this, index]
        {
            const auto id = gestureIDs[index].isNotEmpty() ? gestureIDs[index] : SmallTimerProcessor::faderID(processor.mode(), index);
            processor.setValue(id, static_cast<float>(faders[index].getValue()));
            processor.manualFaderEdit(index);
            motor.position[index] = static_cast<float>(faders[index].getValue());
            motor.settle[index] = 1.5f;
        };
        fader.onDragEnd = [this, index] { finishDrag(index); };
        auto& lock = locks[index];
        addAndMakeVisible(lock);
        lock.setButtonText("FREE");
        lock.setTitle(fader.getTitle() + " lock");
        lock.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        lock.setTooltip("Lock prevents Wander, mutation and recall for this fader. Manual edits and mode switching remain available.");
        lock.onClick = [this, index] { processor.setValue("lock" + juce::String(index), processor.locked(index) ? 0.0f : 1.0f); };
    }
    addAndMakeVisible(mutate);
    mutate.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mutate.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mutate.setRange(0, 1, 0.001);
    mutate.onDragStart = [this] { processor.parameters.getParameter("mutate")->beginChangeGesture(); };
    mutate.onDragEnd = [this] { processor.parameters.getParameter("mutate")->endChangeGesture(); };
    mutate.onValueChange = [this] { processor.setValue("mutate", static_cast<float>(mutate.getValue())); };
    mutate.setTooltip("Wander and mutation depth. NEW SEED mutates the current mode's unlocked settings.");
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
    timerCallback();
}
SmallTimerEditor::~SmallTimerEditor()
{
    stopTimer();
    for (int index = 0; index < 6; ++index)
        finishDrag(index);
    setLookAndFeel(nullptr);
}
void SmallTimerEditor::finishDrag(int index)
{
    if (gestureIDs[index].isNotEmpty())
        processor.parameters.getParameter(gestureIDs[index])->endChangeGesture();
    gestureIDs[index].clear();
    motor.dragging[index] = false;
    processor.setFaderDragging(index, false);
}
void SmallTimerEditor::setMode(int mode)
{
    processor.setValue("mode", static_cast<float>(mode));
}
void SmallTimerEditor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto elapsed = static_cast<float>(juce::jlimit(0.001, 0.05, (now - lastTick) / 1000.0));
    lastTick = now;
    const auto currentMode = processor.mode();
    if (currentMode != lastMode)
    {
        for (int index = 0; index < 6; ++index)
            if (motor.dragging[index])
            {
                finishDrag(index);
                motor.dragging[index] = true;
                processor.setFaderDragging(index, true);
                gestureIDs[index] = SmallTimerProcessor::faderID(currentMode, index);
                processor.parameters.getParameter(gestureIDs[index])->beginChangeGesture();
                processor.setValue(gestureIDs[index], motor.position[index]);
            }
        lastMode = currentMode;
    }
    std::array<bool, 6> locked {};
    for (int index = 0; index < 6; ++index)
        locked[index] = processor.locked(index);
    const auto audioBlock = processor.processedBlocks.load(std::memory_order_relaxed);
    if (audioBlock != lastAudioBlock)
    {
        lastAudioBlock = audioBlock;
        lastAudioTick = now;
    }
    if (lastAudioTick > 0 && now - lastAudioTick < 200.0)
    {
        const auto audible = processor.motorPositions();
        for (int index = 0; index < 6; ++index)
            if (!motor.dragging[index])
                motor.position[index] = audible[index];
    }
    else
        motor.tick(processor.targets(), locked, processor.value("motion") > 0.5f, processor.value("hold") > 0.5f, processor.value("mutate"), elapsed);
    for (int index = 0; index < 6; ++index)
    {
        if (!motor.dragging[index])
            faders[index].setValue(motor.position[index], juce::dontSendNotification);
        faders[index].setDoubleClickReturnValue(true, (currentMode == 0 ? smalltimer::loopDefaults : smalltimer::foreverDefaults)[index]);
        locks[index].setToggleState(locked[index], juce::dontSendNotification);
        locks[index].setButtonText(locked[index] ? "LOCK" : "FREE");
        locks[index].setDescription(faders[index].getTitle() + (locked[index] ? ": locked against Wander, mutation and recall" : ": unlocked for Wander, mutation and recall"));
    }
    loop.setToggleState(currentMode == 0, juce::dontSendNotification);
    forever.setToggleState(currentMode == 1, juce::dontSendNotification);
    motion.setToggleState(processor.value("motion") > 0.5f, juce::dontSendNotification);
    motion.getProperties().set("lampLevel", 0.78 + 0.22 * std::sin(now * 0.001 * juce::MathConstants<double>::twoPi / 3.6));
    soft.setToggleState(processor.value("soft") > 0.5f, juce::dontSendNotification);
    hold.setToggleState(processor.value("hold") > 0.5f, juce::dontSendNotification);
    rec.setToggleState(processor.recording.load(), juce::dontSendNotification);
    play.setToggleState(processor.playing.load(), juce::dontSendNotification);
    rec.getProperties().set("lampLevel", 0.72 + 0.28 * std::sin(now * 0.001 * juce::MathConstants<double>::twoPi / 2.0));
    recall.setToggleState(now < recallLightUntil, juce::dontSendNotification);
    recall.getProperties().set("lampLevel", juce::jlimit(0.0, 1.0, (recallLightUntil - now) / 450.0));
    const auto seedLevel = seedBurstStarted > 0 ? juce::jlimit(0.0, 1.0, 1.0 - (now - seedBurstStarted) / 900.0) : 0.0;
    seed.setToggleState(seedLevel > 0, juce::dontSendNotification);
    seed.getProperties().set("lampLevel", seedLevel * seedLevel);
    undoSeed.setEnabled(processor.canUndoMutation());
    if (!mutate.isMouseButtonDown())
        mutate.setValue(processor.value("mutate"), juce::dontSendNotification);
    repaint();
}
void SmallTimerEditor::resized()
{
    loop.setBounds(300, 33, 62, 69);
    forever.setBounds(362, 33, 80, 69);
    motion.setBounds(474, 33, 58, 69);
    soft.setBounds(532, 33, 70, 69);
    for (int index = 0; index < 6; ++index)
    {
        faders[index].setBounds(73 + index * 88, 257, 66, 216);
        locks[index].setBounds(85 + index * 88, 492, 42, 25);
    }
    mutate.setBounds(270, 576, 120, 120);
    seed.setBounds(477, 669, 115, 38);
    undoSeed.setBounds(477, 710, 115, 15);
    scopeGain.setBounds(517, 146, 65, 14);
    scopeFreeze.setBounds(444, 146, 65, 14);
    hold.setBounds(83, 543, 110, 38);
    rec.setBounds(83, 587, 110, 38);
    play.setBounds(83, 631, 110, 38);
    recall.setBounds(83, 675, 110, 38);
}
void SmallTimerEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff151310));
    for (int side : { 0, 1 })
    {
        const auto left = side == 0 ? 5.0f : 624.0f;
        juce::ColourGradient walnut(juce::Colour(0xff50301d), left, 0, juce::Colour(0xff633b24), left + 31, 0, false);
        walnut.addColour(0.3, juce::Colour(0xffa76d42));
        walnut.addColour(0.7, juce::Colour(0xff845233));
        graphics.setGradientFill(walnut);
        graphics.fillRoundedRectangle(left, 5, 31, 740, 5);
        for (int grain = 0; grain < 20; ++grain)
        {
            juce::Path line;
            const auto base = left + 2 + grain * 1.4f;
            line.startNewSubPath(base, 8);
            for (int row = 10; row < 744; row += 8)
                line.lineTo(base + std::sin(row * 0.013f + grain * 2.1f) * 1.1f, static_cast<float>(row));
            graphics.setColour(juce::Colour(grain % 3 == 0 ? 0xff160e09 : 0xffd6a06d).withAlpha(0.13f));
            graphics.strokePath(line, juce::PathStrokeType(grain % 3 == 0 ? 0.8f : 0.4f));
        }
    }
    graphics.setGradientFill(juce::ColourGradient(juce::Colour(0xff343331), 38, 0, juce::Colour(0xff222221), 622, 750, false));
    graphics.fillRoundedRectangle(38, 4, 584, 742, 5);
    graphics.drawImage(panelTexture(), juce::Rectangle<float>(40, 6, 580, 738));
    graphics.setColour(ink.withAlpha(0.32f));
    graphics.drawRoundedRectangle(38.5f, 4.5f, 583, 741, 5, 1);
    for (int left : { 52, 604 })
        for (int top : { 19, 724 })
        {
            graphics.setGradientFill(juce::ColourGradient(juce::Colour(0xffc6b59b), left - 4.0f, top - 4.0f, juce::Colour(0xff34302a), left + 4.0f, top + 4.0f, false));
            graphics.fillEllipse(static_cast<float>(left - 6), static_cast<float>(top - 6), 12, 12);
            graphics.setColour(juce::Colour(0xff171614));
            graphics.fillEllipse(static_cast<float>(left - 3), static_cast<float>(top - 3), 6, 6);
            graphics.setColour(dim.withAlpha(0.7f));
            graphics.drawLine(static_cast<float>(left - 2), static_cast<float>(top + 2), static_cast<float>(left + 2), static_cast<float>(top - 2));
            graphics.drawLine(static_cast<float>(left - 2), static_cast<float>(top - 2), static_cast<float>(left + 2), static_cast<float>(top + 2));
        }
    graphics.setColour(ink);
    graphics.setFont(juce::Font(juce::FontOptions("Arial", 37.0f, juce::Font::bold | juce::Font::italic)));
    graphics.drawText("SMALL", 71, 25, 220, 37, juce::Justification::centredLeft);
    graphics.drawText("TIMER", 71, 56, 220, 37, juce::Justification::centredLeft);
    graphics.setColour(juce::Colour(0xffdf7e2d));
    graphics.drawText("/", 195, 56, 35, 37, juce::Justification::centredLeft);
    label(graphics, "E X P E R I M E N T A L  L O O P E R  /  T E X T U R E  P R O C E S S O R", { 70, 106, 322, 14 }, 9);
    graphics.setColour(juce::Colours::black.withAlpha(0.12f));
    graphics.fillRoundedRectangle(296, 28, 150, 78, 7);
    graphics.setColour(ink.withAlpha(0.23f));
    graphics.drawRoundedRectangle(296, 28, 150, 78, 7, 0.7f);
    label(graphics, "M O D E", { 336, 14, 70, 12 }, 8, ink.withAlpha(0.7f));
    graphics.setGradientFill(juce::ColourGradient(juce::Colour(0xff050706), 66, 143, juce::Colour(0xff101510), 66, 226, false));
    graphics.fillRoundedRectangle(66, 143, 528, 83, 14);
    graphics.setColour(juce::Colours::black);
    graphics.drawRoundedRectangle(65, 142, 530, 85, 15, 2);
    graphics.setColour(ink.withAlpha(0.35f));
    graphics.drawRoundedRectangle(66, 143, 528, 83, 14, 0.7f);
    graphics.setColour(ink.withAlpha(0.10f));
    graphics.drawHorizontalLine(188, 76, 584);
    graphics.setColour(ink.withAlpha(0.12f));
    graphics.drawHorizontalLine(174, 105, 578);
    graphics.setColour(amber.withAlpha(0.12f));
    graphics.drawHorizontalLine(208, 105, 578);
    juce::Path inputWave, outputWave;
    if (!scopeFreeze.getToggleState())
        displayedWaveform = processor.waveformSnapshot();
    const auto& waveform = displayedWaveform;
    for (int index = 0; index < 256; ++index)
    {
        const float left = 105 + index * 473.0f / 255;
        const auto& frame = waveform[static_cast<size_t>(index)];
        const auto displayGain = static_cast<float>(1 << scopeGainStep);
        const float inputY = 174 - juce::jlimit(-1.25f, 1.25f, frame.input * displayGain) * 8.0f;
        const float outputY = 208 - juce::jlimit(-1.25f, 1.25f, frame.output * displayGain) * 8.0f;
        if (index == 0)
        {
            inputWave.startNewSubPath(left, inputY);
            outputWave.startNewSubPath(left, outputY);
        }
        else
        {
            inputWave.lineTo(left, inputY);
            outputWave.lineTo(left, outputY);
        }
    }
    drawSignal(graphics, inputWave, { 101, 160, 481, 28 }, juce::Colour(0xffa0dfb8), juce::Colour(0xffe0f5df));
    drawSignal(graphics, outputWave, { 101, 194, 481, 28 }, juce::Colour(0xffffb642), juce::Colour(0xffffebba));
    label(graphics, "IN", { 75, 168, 26, 12 }, 8, juce::Colour(0xffb6c0bb));
    label(graphics, "OUT", { 75, 202, 26, 12 }, 8, amber);
    const auto status = processor.recording.load() ? "RECORDING / RAM ONLY" : processor.playing.load() ? "RAM LOOP / THROUGH FX" : "LIVE INPUT / DELAY + TEXTURE";
    label(graphics, scopeFreeze.getToggleState() ? "DISPLAY FROZEN" : status, { 175, 146, 260, 12 }, 7, amber.withAlpha(0.8f));
    graphics.setColour(ink);
    graphics.setFont(juce::Font(juce::FontOptions("Arial", 15.0f, juce::Font::bold | juce::Font::italic)));
    graphics.drawText("M U T A T E", 250, 542, 160, 20, juce::Justification::centred);
    graphics.setColour(juce::Colour(0xffaeb77c));
    juce::Path arc;
    arc.addCentredArc(330, 636, 68, 68, 0, -1.5f, 1.5f, true);
    graphics.strokePath(arc, juce::PathStrokeType(4));
    label(graphics, "VARIATION", { 472, 552, 114, 20 }, 11);
    label(graphics, processor.mutationName(), { 472, 570, 114, 10 }, 7, amber);
    label(graphics, juce::String(juce::roundToInt(processor.value("mutate") * 100)) + "%", { 472, 581, 114, 24 }, 18, ink);
    const auto amount = juce::jlimit(0.0f, 1.0f, processor.value("mutate"));
    const auto seedProgress = seedBurstStarted > 0 ? static_cast<float>((juce::Time::getMillisecondCounterHiRes() - seedBurstStarted) / 900.0) : 2.0f;
    for (int lamp = 0; lamp < 8; ++lamp)
    {
        const float left = 482.0f + lamp * 12.0f;
        const bool depthLit = amount >= (lamp + 1) / 8.0f;
        const auto distance = std::abs(lamp - 3.5f) / 3.5f;
        const auto ripple = seedProgress >= 0 && seedProgress < 1 ? juce::jmax(0.0f, 1.0f - std::abs(distance - seedProgress * 1.25f) / 0.4f) * (1.0f - seedProgress) : 0.0f;
        const auto lampLevel = juce::jmax(depthLit ? 0.7f : 0.0f, ripple);
        const bool lit = lampLevel > 0;
        const auto centre = juce::Point<float>(left + 3, 618);
        if (lit)
        {
            graphics.setGradientFill(juce::ColourGradient(amber.withAlpha(0.43f * lampLevel), centre, amber.withAlpha(0.0f), centre.translated(8, 0), true));
            graphics.fillEllipse(left - 5, 610, 16, 16);
        }
        graphics.setColour(juce::Colour(0xff15130f));
        graphics.fillEllipse(left - 1, 614, 8, 8);
        graphics.setGradientFill(juce::ColourGradient(juce::Colour(0xff766f5d).interpolatedWith(juce::Colour(0xffffdc80), juce::jmin(1.0f, lampLevel / 0.7f)), left, 615, juce::Colour(0xff38352d).interpolatedWith(juce::Colour(0xffd28b20), juce::jmin(1.0f, lampLevel / 0.7f)), left + 6, 621, false));
        graphics.fillEllipse(left, 615, 6, 6);
    }
    const char* names[] { "TIME", "FEED", "SPACE", "DRIFT", "GRAIN", "MIX" };
    for (int index = 0; index < 6; ++index)
    {
        label(graphics, names[index], { 73 + index * 88, 235, 66, 18 }, 12);
        const juce::uint32 colours[] { 0xffad73d3, 0xffe9943d, 0xff91b99c, 0xffe9cd62, 0xff78a9d5, 0xffda7e9d };
        graphics.setColour(juce::Colour(colours[index]));
        graphics.fillRect(84 + index * 88, 254, 44, 3);
        const auto readout = index == 0 ? juce::String(juce::roundToInt(40.0 * std::pow(50.0, motor.position[index]))) + " ms"
                                       : juce::String(juce::roundToInt(motor.position[index] * 100)) + "%";
        label(graphics, readout, { 73 + index * 88, 475, 66, 16 }, 9, dim);
    }
    graphics.setColour(ink.withAlpha(0.13f));
    graphics.drawHorizontalLine(526, 64, 596);
    graphics.drawVerticalLine(222, 548, 709);
    graphics.drawVerticalLine(447, 548, 709);
    const bool held = processor.value("hold") > 0.5f;
    const bool wandering = processor.value("motion") > 0.5f && !held;
    const auto breath = wandering ? 0.78f + 0.22f * static_cast<float>(std::sin(lastTick * 0.001 * juce::MathConstants<double>::twoPi / 3.6)) : 0.0f;
    const auto statusColour = wandering ? juce::Colour(0xffffd88a) : held ? juce::Colour(0xffbc8f49) : dim;
    juce::GlyphArrangement statusGlyphs;
    statusGlyphs.addFittedText(juce::Font(juce::FontOptions("Arial Narrow", 10.0f, juce::Font::plain)),
                               held ? "MOTION HELD" : wandering ? "WANDER ACTIVE" : "WANDER OFF",
                               492, 637, 94, 16, juce::Justification::centred, 1);
    juce::Path statusLetters;
    statusGlyphs.createPath(statusLetters);
    if (wandering)
    {
        juce::DropShadow(juce::Colour(0xffffae34).withAlpha(0.65f * breath), 5, {}).drawForPath(graphics, statusLetters);
        graphics.setColour(juce::Colour(0xffffbc51).withAlpha(0.18f * breath));
        graphics.strokePath(statusLetters, juce::PathStrokeType(1.8f));
        juce::ColourGradient halo(juce::Colour(0xffffb442).withAlpha(0.5f * breath), 483, 645,
                                  juce::Colour(0xffffb442).withAlpha(0.0f), 495, 645, true);
        graphics.setGradientFill(halo);
        graphics.fillEllipse(471, 633, 24, 24);
    }
    graphics.setColour(statusColour.withAlpha(wandering ? 0.8f + 0.2f * breath : 1.0f));
    graphics.fillPath(statusLetters);
    graphics.setColour(juce::Colour(0xff11100e));
    graphics.fillEllipse(478.5f, 640.5f, 9, 9);
    graphics.setColour(wandering ? juce::Colour(0xffffe9a0) : held ? juce::Colour(0xffbc8f49) : juce::Colour(0xff403d34));
    graphics.fillEllipse(480, 642, 6, 6);
    if (wandering)
    {
        graphics.setColour(juce::Colour(0xfffff8da).withAlpha(breath));
        graphics.fillEllipse(481.5f, 643, 2.5f, 2.5f);
    }
    label(graphics, "SHIFT + RECALL TO STORE / 30 SEC VOLATILE TAPE", { 225, 733, 365, 10 }, 7, dim);
    for (auto* button : { &loop, &forever, &motion, &soft, &hold, &rec, &play, &recall, &seed })
    {
        if (!button->getToggleState())
            continue;
        const auto cap = buttonCap(*button).translated(static_cast<float>(button->getX()), static_cast<float>(button->getY()));
        const auto colour = button == &rec ? juce::Colour(0xffff4b28) : button == &play ? juce::Colour(0xff65df99) : juce::Colour(0xffffa615);
        const auto level = static_cast<float>(button->getProperties().getWithDefault("lampLevel", 1.0));
        juce::ColourGradient spill(colour.withAlpha(0.55f * level), cap.getCentre(), colour.withAlpha(0.0f), cap.getCentre().translated(34, 0), true);
        spill.addColour(0.55, colour.withAlpha(0.22f * level));
        graphics.setGradientFill(spill);
        graphics.fillEllipse(cap.getCentreX() - 34, cap.getCentreY() - 34, 68, 68);
    }
}
