#include <drivers/pwm.h>
#include <zephyr.h>
#include <device.h>
#include <sys/util.h>

const uint32_t h6 = 506190;
const uint32_t e7 = 379220;
const uint32_t f3 = 5727048;
const uint32_t boop_period = f3;
const uint32_t beep_period = h6;

const struct pwm_dt_spec buzzer = PWM_DT_SPEC_GET(DT_NODELABEL(buzzer_pwm));

void set_tone(uint32_t period) {
        pwm_set_dt(&buzzer, period, period/2);
}

void mute(void) {
        pwm_set_dt(&buzzer, h6, 0);
}

void coin(void) {
        set_tone(h6);
        k_sleep(K_MSEC(75));
        set_tone(e7);
        k_sleep(K_MSEC(225));
        mute();
}

void boop(void) {
        set_tone(boop_period);
        k_sleep(K_MSEC(100));
        mute();
}

void beep(void) {
        set_tone(beep_period);
        k_sleep(K_MSEC(100));
        mute();
}

void gameover(void) {
        const int steps = 4;
        for (int i = 0; i <= steps; ++i) {
                set_tone(beep_period + (boop_period-beep_period)*i/steps);
                k_sleep(K_MSEC(75));
        }
        k_sleep(K_MSEC(50));
        mute();
}