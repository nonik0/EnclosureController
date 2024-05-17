/**
 * @file view.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-01-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "../enclosure_controller/enclosure_controller.h"
#include "assets/assets.h"
#include <Arduino.h>
#include <smooth_ui_toolkit.h>

static EnclosureController *_encCtl = nullptr;

using namespace SmoothUIToolKit;
using namespace SmoothUIToolKit::SelectMenu;

struct AppOptionRenderProps_t
{
    std::uint32_t theme_color;
    std::uint32_t tag_color;
    const char *tag;
    const std::uint16_t *icon;
};
constexpr int _app_render_props_list_size = 6;
constexpr AppOptionRenderProps_t _app_render_props_list[] = {
    // {0xB8DBD9, 0x385B59, "DISPLAY TEST", image_data_icon_display},
    // {0x87C38F, 0x07430F, "BRIGHTNESS", image_data_icon_brightness},
    // {0xC9C9EE, 0x49496E, "RTC TIME", image_data_icon_rtc},
    // {0xF6A4A4, 0x762424, "WIFI SCAN", image_data_icon_wifi},
    // {0x6AB8A0, 0x163820, "ENCODER TEST", image_data_icon_encoder},
    // {0xC2C1A5, 0x424125, "MENU DEMO", image_data_icon_menu},
    // {0xF5C396, 0x754316, "ARKANOID", image_data_icon_game},
    // {0xC6D5EF, 0x46556F, "SLEEP&WAKEUP", image_data_icon_sleep},
    // {0xCEDBB8, 0x4E5B38, "POWER OFF", image_data_icon_poweroff},
    {0xF5C396, 0x754316, "ARKANOID", image_data_icon_game},
    {0xB8DBD9, 0x385B59, "CURRENT PRINT", image_data_icon_display},
    {0xF5C396, 0x754316, "EXTRUDER TEMP", image_data_icon_game},
    {0xF5C396, 0x754316, "BED TEMP", image_data_icon_game},
    {0x87C38F, 0x07430F, "LED BRIGHTNESS", image_data_icon_brightness},
    {0x87C38F, 0x07430F, "LED PATTERN", image_data_icon_brightness},
};

static Transition2D *_batv_panel_transition = nullptr;
static std::uint32_t _batv_time_count = 0;
static char _batv[10] = {0};
static int _last_enc_postion = 0;
static bool _is_just_boot_in = true;

class LauncherMenu : public SmoothOptions
{
    bool _wait_button_released = false;
    bool _is_pressing = false;
    int _matching_index = 0;

    void onReadInput() override
    {
        if (isOpening())
            return;

        // Update navigation
        _encCtl->_check_encoder(true);
        if (_encCtl->_enc.getPosition() != _last_enc_postion)
        {
            if (_encCtl->_enc.getPosition() < _last_enc_postion)
            {
                goLast();
            }
            else if (_encCtl->_enc.getPosition() > _last_enc_postion)
            {
                goNext();
            }

            _last_enc_postion = _encCtl->_enc.getPosition();
        }

        // If just boot in, lock until button released
        if (_is_just_boot_in)
        {
            // If not pressing
            if (_encCtl->_enc_btn.read())
            {
                _is_just_boot_in = false;
            }
        }

        // If select
        else if (!_encCtl->_enc_btn.read())
        {
            if (!_wait_button_released)
            {
                _encCtl->_tone(2500, 50);

                _wait_button_released = true;
                _is_pressing = true;

                // Squeeze it
                press({0, 12, 240, 52});
            }
        }

        // Unlock if no button is pressing
        else
        {
            _wait_button_released = false;
            if (_is_pressing)
            {
                _is_pressing = false;
                release();
            }
        }
    }

    void onRender() override
    {
        // Clear
        _encCtl->_canvas->fillScreen(TFT_BLACK);

        // Render batv panel
        _encCtl->_canvas->setTextDatum(top_left);
        _encCtl->_canvas->setTextColor(0x7F5845);
        _encCtl->_canvas->setFont(&fonts::efontCN_16);
        _encCtl->_canvas->setTextColor(TFT_SILVER);
        _encCtl->_canvas->drawString("Bat:", _batv_panel_transition->getValue().x + 6, _batv_panel_transition->getValue().y + 13);
        _encCtl->_canvas->setFont(&fonts::efontCN_24);
        _encCtl->_canvas->drawString(_batv, _batv_panel_transition->getValue().x + 4, _batv_panel_transition->getValue().y + 29);

        // Render options
        int y_offset = 6;
        _encCtl->_canvas->setTextDatum(top_right);
        for (int i = getKeyframeList().size() - 1; i >= 0; i--)
        {
            getMatchingOptionIndex(i, _matching_index);

            // Render cards
            _encCtl->_canvas->fillSmoothRoundRect(getOptionCurrentFrame(_matching_index).x,
                                                  getOptionCurrentFrame(_matching_index).y,
                                                  getOptionCurrentFrame(_matching_index).w,
                                                  getOptionCurrentFrame(_matching_index).h,
                                                  20,
                                                  _app_render_props_list[_matching_index].theme_color);

            // Render icons
            if (!isOpening())
            {
                if (i <= 1)
                {
                    y_offset = getOptionCurrentFrame(_matching_index).y + 16 -
                               std::abs(getOptionCurrentFrame(_matching_index).y - getKeyframe(0).y) * 10 / 75;
                    if (isPressing())
                        y_offset = i == 0 ? getKeyframe(0).y + 16 : getKeyframe(1).y + 6;
                }
                else
                    y_offset = getOptionCurrentFrame(_matching_index).y + 6;

                _encCtl->_canvas->pushImage(getOptionCurrentFrame(_matching_index).x + 13,
                                            y_offset,
                                            32,
                                            32,
                                            _app_render_props_list[_matching_index].icon);
            }

            // Render tags
            if (i == 0 && !isOpening())
            {
                _encCtl->_canvas->setTextColor(_app_render_props_list[_matching_index].tag_color);
                _encCtl->_canvas->drawString(_app_render_props_list[_matching_index].tag, 218, 26);
            }
        }

        // Push
        _encCtl->_canvas_update();
    }

    void onPress() override
    {
        // Set press anim
        setDuration(200);
        setTransitionPath(EasingPath::easeOutQuad);
    }

    void onClick() override
    {
        // Set open anim
        setDuration(300);
        setTransitionPath(EasingPath::easeOutQuad);

        open({-20, -20, 280, 175});
    }

    void onOpenEnd() override
    {
        _open_app();

        // Reset anim
        setPositionDuration(600);
        setPositionTransitionPath(EasingPath::easeOutBack);
        setShapeDuration(400);

        // Close option
        close();
        _encCtl->_enc.setPosition(_last_enc_postion);
        _encCtl->_canvas->setFont(&fonts::efontCN_24);
        _encCtl->_canvas->setTextSize(1);
    }

    void _open_app()
    {
        int matching_index = getSelectedOptionIndex();
        if (matching_index == 0)
            _encCtl->_arkanoid_start();
        else if (matching_index == 1)
            _encCtl->_arkanoid_start(); // current print
        else if (matching_index == 2)
            _encCtl->_printer_set_extruder_temp();
        else if (matching_index == 3)
            _encCtl->_printer_set_bed_temp();
        else if (matching_index == 4)
            _encCtl->_disp_set_brightness(); // led brightness
        else if (matching_index == 5)
            _encCtl->_disp_set_brightness(); // led pattern
    }
};

static LauncherMenu *_launcher_menu = nullptr;

void view_create(EnclosureController *encCtl)
{
    _encCtl = encCtl;
    _encCtl->_enc.setPosition(_last_enc_postion);
    _encCtl->_canvas->setFont(&fonts::efontCN_24);
    _encCtl->_canvas->setTextSize(1);

    // Create menu
    _launcher_menu = new LauncherMenu;

    // Selected one
    _launcher_menu->addOption();
    _launcher_menu->setLastKeyframe({6, 6, 228, 64});

    // Waiting line
    for (int i = 0; i < _app_render_props_list_size - 2; i++)
    {
        // I'm too lazy to use userdata to paas the props
        _launcher_menu->addOption();
        _launcher_menu->setLastKeyframe({88 + 65 * i, 81, 58, 44});
    }

    // Set the last one next to selected one to smooth the loop
    _launcher_menu->addOption();
    _launcher_menu->setLastKeyframe({6 + 228 + 24, 6, 58, 44});

    // Config
    _launcher_menu->setConfig().renderInterval = 20;
    _launcher_menu->setConfig().readInputInterval = 50;
    _launcher_menu->setPositionDuration(600);
    _launcher_menu->setPositionTransitionPath(EasingPath::easeOutBack);
    _launcher_menu->setShapeDuration(400);

    // Create bat va panel transition
    _batv_panel_transition = new Transition2D(-83, 135);
    _batv_panel_transition->moveTo(0, 81);
    _batv_panel_transition->setDelay(300);
    _batv_panel_transition->setDuration(800);
    float bat_v = (float)analogReadMilliVolts(10) * 2 / 1000;
    snprintf(_batv, 10, "%.1fV", bat_v);
}

void view_update()
{
    _launcher_menu->update(millis());
    _batv_panel_transition->update(millis());

    // Read bat voltage
    if (millis() - _batv_time_count > 3000)
    {
        float bat_v = (float)analogReadMilliVolts(10) * 2 / 1000;
        snprintf(_batv, 10, "%.1fV", bat_v);
        _batv_time_count = millis();
    }
}
