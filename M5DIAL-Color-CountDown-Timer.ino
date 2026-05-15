/*
MIT License
Copyright (c) 2026 omiya-bonsai
*/

#include <M5Dial.h>
#include <Preferences.h>
#include "config.h"

enum TimerState
{
    TIMER_READY,
    TIMER_RUNNING,
    TIMER_PAUSED,
    TIMER_DONE
};

TimerState timerState = TIMER_READY;

uint32_t durationSec = DEFAULT_TIMER_MINUTES * 60;
uint32_t remainingSec = durationSec;

uint32_t lastTickMs = 0;
uint32_t lastDrawMs = 0;
uint32_t doneEnteredMs = 0;

long lastEncoderValue = 0;
bool halfNotified = false;
bool longPressHandled = false;
bool isMuted = MUTE_MODE_DEFAULT;
uint32_t savedDefaultDurationSec = DEFAULT_TIMER_MINUTES * 60;
uint8_t currentBrightness = DISPLAY_BRIGHTNESS;

uint16_t colorReady;
uint16_t colorRun;
uint16_t colorPause;
uint16_t colorDone;
uint16_t colorDim;
uint16_t colorText;
uint16_t colorBg;

M5Canvas canvas(&M5Dial.Display);
uint16_t ringPalette[RING_SEGMENTS];
Preferences prefs;

struct BeepStep
{
    uint16_t freq;
    uint16_t ms;
    uint16_t gapMs;
};

constexpr uint8_t BEEP_QUEUE_MAX = 16;
BeepStep beepQueue[BEEP_QUEUE_MAX];
uint8_t beepHead = 0;
uint8_t beepTail = 0;
uint32_t nextBeepAtMs = 0;

bool hasDrawn = false;
uint32_t lastDrawnDurationSec = 0;
uint32_t lastDrawnRemainingSec = 0;
TimerState lastDrawnState = TIMER_READY;
bool blinkPhaseOn = true;
bool lastDrawnBlinkPhaseOn = true;

constexpr uint32_t SEGMENT_BLINK_INTERVAL_MS = 650;
constexpr int PRESET_COUNT = 4;
const int PRESET_MINUTES[PRESET_COUNT] = {5, 15, 25, 50};

bool enqueueBeep(uint16_t freq, uint16_t ms, uint16_t gapMs = 0)
{
    uint8_t nextTail = (uint8_t)((beepTail + 1) % BEEP_QUEUE_MAX);
    if (nextTail == beepHead)
        return false;

    beepQueue[beepTail] = {freq, ms, gapMs};
    beepTail = nextTail;
    return true;
}

void clearBeepQueue()
{
    beepHead = 0;
    beepTail = 0;
    nextBeepAtMs = 0;
}

void processBeeper()
{
#if USE_SPEAKER
    if (isMuted)
    {
        clearBeepQueue();
        return;
    }

    uint32_t now = millis();
    if (beepHead == beepTail || now < nextBeepAtMs)
        return;

    const BeepStep &step = beepQueue[beepHead];
    M5Dial.Speaker.tone(step.freq, step.ms);
    nextBeepAtMs = now + step.ms + step.gapMs;
    beepHead = (uint8_t)((beepHead + 1) % BEEP_QUEUE_MAX);
#endif
}

void playHalfNotify()
{
#if HALF_TIME_NOTIFY
    if (HALF_NOTIFY_PATTERN == 0)
    {
        enqueueBeep(2000, 120, 0);
    }
    else if (HALF_NOTIFY_PATTERN == 1)
    {
        enqueueBeep(1800, 80, 40);
        enqueueBeep(2200, 110, 0);
    }
    else
    {
        enqueueBeep(1500, 70, 30);
        enqueueBeep(1900, 70, 30);
        enqueueBeep(2350, 90, 0);
    }
#endif
}

void playDoneAlarm()
{
    clearBeepQueue();
    enqueueBeep(2600, 120, 40);
    enqueueBeep(2600, 120, 40);
    enqueueBeep(2600, 120, 0);
}

void setBrightnessSafe(uint8_t value)
{
    if (value == currentBrightness)
        return;
    currentBrightness = value;
    M5Dial.Display.setBrightness(currentBrightness);
}

void loadPersistedDuration()
{
#if PERSIST_LAST_DURATION
    prefs.begin("timer", true);
    uint32_t stored = prefs.getUInt("durationSec", DEFAULT_TIMER_MINUTES * 60);
    prefs.end();

    uint32_t minSec = MIN_TIMER_MINUTES * 60;
    uint32_t maxSec = MAX_TIMER_MINUTES * 60;
    stored = constrain(stored, minSec, maxSec);
    savedDefaultDurationSec = stored;
#endif
}

void persistDuration(uint32_t sec)
{
#if PERSIST_LAST_DURATION
    prefs.begin("timer", false);
    prefs.putUInt("durationSec", sec);
    prefs.end();
#endif
}

void beep(uint16_t freq, uint16_t ms)
{
    enqueueBeep(freq, ms, 0);
}

void resetTimer()
{
    timerState = TIMER_READY;
    durationSec = savedDefaultDurationSec;
    remainingSec = durationSec;
    halfNotified = false;
    doneEnteredMs = 0;
    setBrightnessSafe(DISPLAY_BRIGHTNESS);
}

void formatTime(uint32_t sec, char *out, size_t outSize)
{
    uint32_t m = sec / 60;
    uint32_t s = sec % 60;

    snprintf(out, outSize, "%02lu:%02lu", m, s);
}

float remainRatio()
{
    if (durationSec == 0)
        return 0.0f;
    return (float)remainingSec / (float)durationSec;
}

int activeSegmentCount()
{
    int count = (int)ceil(remainRatio() * RING_SEGMENTS);
    return constrain(count, 0, RING_SEGMENTS);
}

void adjustMinutes(int delta)
{
    if (timerState == TIMER_RUNNING)
        return;

    int minutes = durationSec / 60;

#if USE_PRESET_STEPS
    int steps = abs(delta);
    int direction = (delta >= 0) ? 1 : -1;

    while (steps-- > 0)
    {
        if (direction > 0)
        {
            bool moved = false;
            for (int i = 0; i < PRESET_COUNT; i++)
            {
                if (PRESET_MINUTES[i] > minutes)
                {
                    minutes = PRESET_MINUTES[i];
                    moved = true;
                    break;
                }
            }
            if (!moved)
            {
                minutes++;
            }
        }
        else
        {
            bool moved = false;
            for (int i = PRESET_COUNT - 1; i >= 0; i--)
            {
                if (PRESET_MINUTES[i] < minutes)
                {
                    minutes = PRESET_MINUTES[i];
                    moved = true;
                    break;
                }
            }
            if (!moved)
            {
                minutes--;
            }
        }
    }
#else
    minutes += delta;
#endif

    minutes = constrain(minutes, MIN_TIMER_MINUTES, MAX_TIMER_MINUTES);

    durationSec = minutes * 60;
    remainingSec = durationSec;
    halfNotified = false;
    savedDefaultDurationSec = durationSec;
    persistDuration(durationSec);
}

void handleEncoder()
{
    long value = M5Dial.Encoder.read();
    long diff = value - lastEncoderValue;

    if (diff == 0)
        return;

    long stepChange = diff / ENCODER_STEPS_PER_MINUTE;
    if (stepChange != 0)
    {
        adjustMinutes((int)stepChange);
        lastEncoderValue += stepChange * ENCODER_STEPS_PER_MINUTE;
        beep((stepChange > 0) ? 2200 : 1800, 20);
    }
}

void handleButton()
{
    if (!longPressHandled && M5Dial.BtnA.pressedFor(LONG_PRESS_RESET_MS))
    {
#if ENABLE_LONG_PRESS_MUTE_TOGGLE
        if (timerState == TIMER_READY)
        {
            isMuted = !isMuted;
            if (!isMuted)
            {
                beep(2400, 80);
            }
        }
        else
        {
            resetTimer();
            beep(1200, 120);
        }
#else
        resetTimer();
        beep(1200, 120);
#endif
        longPressHandled = true;
    }

    if (M5Dial.BtnA.wasReleased())
    {
        if (!longPressHandled)
        {
            if (timerState == TIMER_READY)
            {
                timerState = TIMER_RUNNING;
                lastTickMs = millis();
                beep(2600, 70);
            }
            else if (timerState == TIMER_RUNNING)
            {
                timerState = TIMER_PAUSED;
                beep(1800, 50);
            }
            else if (timerState == TIMER_PAUSED)
            {
                timerState = TIMER_RUNNING;
                lastTickMs = millis();
                beep(2600, 50);
            }
            else if (timerState == TIMER_DONE)
            {
                resetTimer();
                beep(2200, 100);
            }
        }

        longPressHandled = false;
    }
}

void updateTimer()
{
    if (timerState != TIMER_RUNNING)
        return;

    uint32_t now = millis();

    while (now - lastTickMs >= 1000)
    {
        lastTickMs += 1000;

        if (remainingSec == 0)
            break;

        remainingSec--;

        if (HALF_TIME_NOTIFY && !halfNotified && remainingSec <= durationSec / 2)
        {
            halfNotified = true;
            playHalfNotify();
        }

        if (remainingSec == 0)
        {
            timerState = TIMER_DONE;
            doneEnteredMs = now;
            playDoneAlarm();
            break;
        }
    }
}

void handleDoneState()
{
    if (timerState != TIMER_DONE)
        return;

    uint32_t now = millis();
    uint32_t elapsed = now - doneEnteredMs;

#if DONE_DIM_START_MS > 0
    if (elapsed >= DONE_DIM_START_MS)
    {
        uint32_t dimElapsed = elapsed - DONE_DIM_START_MS;
        uint32_t dimSteps = (DONE_DIM_EVERY_MS > 0) ? (dimElapsed / DONE_DIM_EVERY_MS) : 0;
        int target = DISPLAY_BRIGHTNESS - (int)(dimSteps * DONE_DIM_STEP);
        target = max(target, (int)DONE_MIN_BRIGHTNESS);
        setBrightnessSafe((uint8_t)target);
    }
#endif

#if DONE_AUTO_RESET_MS > 0
    if (elapsed >= DONE_AUTO_RESET_MS)
    {
        resetTimer();
        beep(1500, 80);
    }
#endif
}

void drawRingSegments()
{
    int activeSegments = activeSegmentCount();
    int firstActiveIndex = RING_SEGMENTS - activeSegments;
    bool blinkEnabled = (timerState == TIMER_RUNNING) && (activeSegments > 0) && (remainingSec > 0);
    int blinkIndex = firstActiveIndex;

    for (int i = 0; i < RING_SEGMENTS; i++)
    {
        float startAngle = -90.0f + i * (360.0f / RING_SEGMENTS) + SEGMENT_GAP_DEG;
        float endAngle = -90.0f + (i + 1) * (360.0f / RING_SEGMENTS) - SEGMENT_GAP_DEG;

        uint16_t c = (i >= firstActiveIndex) ? ringPalette[i] : colorDim;
        if (blinkEnabled && i == blinkIndex && !blinkPhaseOn)
        {
            c = colorDim;
        }

        canvas.fillArc(
            CENTER_X,
            CENTER_Y,
            RING_INNER_RADIUS,
            RING_OUTER_RADIUS,
            startAngle,
            endAngle,
            c);
    }
}

void drawInnerGuideArc()
{
#if DRAW_INNER_GUIDE_ARC
    canvas.drawCircle(CENTER_X, CENTER_Y, INNER_GUIDE_RADIUS, colorDim);
#endif
}

void drawStatusText()
{
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(colorText, colorBg);

    canvas.setTextSize(1);

    const char *label = "";

    switch (timerState)
    {
    case TIMER_READY:
        label = "READY";
        canvas.setTextColor(colorReady, colorBg);
        break;
    case TIMER_RUNNING:
        label = "RUNNING";
        canvas.setTextColor(colorRun, colorBg);
        break;
    case TIMER_PAUSED:
        label = "PAUSED";
        canvas.setTextColor(colorPause, colorBg);
        break;
    case TIMER_DONE:
        label = "TIME UP";
        canvas.setTextColor(colorDone, colorBg);
        break;
    }

    canvas.drawString(label, CENTER_X, 58);
}

void drawMainTime()
{
    canvas.setTextDatum(middle_center);

    if (timerState == TIMER_DONE)
    {
        canvas.setTextColor(colorDone, colorBg);
        canvas.setTextSize(3);
        canvas.drawString("DONE", CENTER_X, CENTER_Y - 8);
    }
    else
    {
        char timeBuf[8];
        formatTime(remainingSec, timeBuf, sizeof(timeBuf));
        canvas.setTextColor(colorText, colorBg);
        canvas.setTextSize(4);
        canvas.drawString(timeBuf, CENTER_X, CENTER_Y - 8);
    }
}

void drawSubInfo()
{
    float ratio = remainRatio();
    int percent = round(ratio * 100);

    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.setTextColor(colorText, colorBg);

    char info[40];
    snprintf(info, sizeof(info), "%lu min / %d%%%s", durationSec / 60, percent, isMuted ? " / MUTE" : "");
    canvas.drawString(info, CENTER_X, CENTER_Y + 36);
}

void drawHelpText()
{
#if DRAW_HELP_TEXT
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.setTextColor(colorDim, colorBg);

    if (timerState == TIMER_READY)
    {
        canvas.drawString("Rotate:set  Press:start  Hold:mute", CENTER_X, 183);
    }
    else if (timerState == TIMER_RUNNING)
    {
        canvas.drawString("Press:pause", CENTER_X, 183);
    }
    else if (timerState == TIMER_PAUSED)
    {
        canvas.drawString("Press:resume  Hold:reset", CENTER_X, 183);
    }
    else
    {
        canvas.drawString("Press:reset", CENTER_X, 183);
    }
#endif
}

void drawDisplay()
{
    canvas.fillScreen(colorBg);

    drawRingSegments();
    drawInnerGuideArc();
    drawStatusText();
    drawMainTime();
    drawSubInfo();
    drawHelpText();

    canvas.pushSprite(0, 0);

    hasDrawn = true;
    lastDrawnDurationSec = durationSec;
    lastDrawnRemainingSec = remainingSec;
    lastDrawnState = timerState;
    lastDrawnBlinkPhaseOn = blinkPhaseOn;
}

void setup()
{
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, true);

    M5Dial.Display.setRotation(DISPLAY_ROTATION);
    M5Dial.Display.setBrightness(DISPLAY_BRIGHTNESS);
    currentBrightness = DISPLAY_BRIGHTNESS;

    colorReady = M5Dial.Display.color565(120, 180, 255);
    colorRun = M5Dial.Display.color565(80, 220, 120);
    colorPause = M5Dial.Display.color565(255, 190, 60);
    colorDone = M5Dial.Display.color565(255, 70, 50);
    colorDim = M5Dial.Display.color565(35, 35, 35);
    colorText = M5Dial.Display.color565(245, 245, 245);
    colorBg = M5Dial.Display.color565(0, 0, 0);

    // VBT20寄りの、パキッとした離散色パレット（1色あたり2セグメント）。
    const uint8_t kPaletteRGB[RING_SEGMENTS][3] = {
        {50, 140, 255}, {50, 140, 255}, // blue
        {45, 185, 255},
        {45, 185, 255}, // sky
        {40, 225, 235},
        {40, 225, 235}, // cyan
        {65, 220, 120},
        {65, 220, 120}, // green
        {150, 220, 70},
        {150, 220, 70}, // yellow-green
        {235, 210, 55},
        {235, 210, 55}, // yellow
        {255, 180, 45},
        {255, 180, 45}, // amber
        {255, 145, 40},
        {255, 145, 40}, // orange
        {255, 105, 45},
        {255, 105, 45}, // orange-red
        {250, 75, 70},
        {250, 75, 70} // red
    };

    for (int i = 0; i < RING_SEGMENTS; i++)
    {
        ringPalette[i] = M5Dial.Display.color565(kPaletteRGB[i][0], kPaletteRGB[i][1], kPaletteRGB[i][2]);
    }

    canvas.setColorDepth(16);
    canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

    M5Dial.Encoder.write(0);
    lastEncoderValue = 0;

    loadPersistedDuration();

    resetTimer();
    drawDisplay();
}

void loop()
{
    M5Dial.update();

    handleEncoder();
    handleButton();
    updateTimer();
    handleDoneState();
    processBeeper();

    uint32_t now = millis();
    blinkPhaseOn = ((now / SEGMENT_BLINK_INTERVAL_MS) % 2) == 0;
    bool blinkDirty = (timerState == TIMER_RUNNING) &&
                      (activeSegmentCount() > 0) &&
                      (blinkPhaseOn != lastDrawnBlinkPhaseOn);

    bool dirty = !hasDrawn ||
                 (durationSec != lastDrawnDurationSec) ||
                 (remainingSec != lastDrawnRemainingSec) ||
                 (timerState != lastDrawnState) ||
                 blinkDirty;

    if (dirty && (now - lastDrawMs >= DRAW_INTERVAL_MS))
    {
        lastDrawMs = now;
        drawDisplay();
    }
}