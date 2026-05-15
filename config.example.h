#pragma once

// ------------------------------------------------------------
// M5Dial Visual Timer
// ------------------------------------------------------------

// 基本タイマー設定
#define DEFAULT_TIMER_MINUTES 25
#define MIN_TIMER_MINUTES 1
#define MAX_TIMER_MINUTES 199

// 半分通知
#define HALF_TIME_NOTIFY true

// 半分通知音パターン
// 0: 単音 / 1: 2音 / 2: 3音（上昇）
#define HALF_NOTIFY_PATTERN 1

// M5Dialのロータリーエンコーダ設定
// 反応が速すぎる場合は 4 や 6 に増やす
#define ENCODER_STEPS_PER_MINUTE 2

// 長押しリセット
#define LONG_PRESS_RESET_MS 1200

// READY状態での長押しでミュート切り替え
#define ENABLE_LONG_PRESS_MUTE_TOGGLE true

// 初期ミュート状態
#define MUTE_MODE_DEFAULT false

// 描画更新間隔
#define DRAW_INTERVAL_MS 100

// DONE状態の自動挙動
// 0で無効（手動リセットのみ）
#define DONE_AUTO_RESET_MS 0
#define DONE_DIM_START_MS 6000
#define DONE_DIM_EVERY_MS 2000
#define DONE_DIM_STEP 12
#define DONE_MIN_BRIGHTNESS 40

// スピーカー通知
#define USE_SPEAKER true

// 画面設定
#define DISPLAY_ROTATION 0
#define DISPLAY_BRIGHTNESS 180

// M5Dial Display
// M5Dialは240x240の円形LCD
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

#define CENTER_X 120
#define CENTER_Y 120

// ------------------------------------------------------------
// VBT20風リング設定
// ------------------------------------------------------------

// VBT20風に20分割
#define RING_SEGMENTS 20

// プリセット刻み（5/15/25/50分）
// true の場合、エンコーダ回転でプリセット値を巡回
#define USE_PRESET_STEPS true

// セグメント間の隙間
#define SEGMENT_GAP_DEG 1.6f

// リング太さ
#define RING_OUTER_RADIUS 112
#define RING_INNER_RADIUS 94

// 内側補助円
#define DRAW_INNER_GUIDE_ARC true
#define INNER_GUIDE_RADIUS 82

// 操作ヒント表示
#define DRAW_HELP_TEXT true

// 最後に設定した分数をNVSに保存して次回起動時に復元
#define PERSIST_LAST_DURATION true