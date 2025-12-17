#include "detector_defs.h"
#include "config.h"
#include "display/display_manager.hpp"
#include <math.h>

#define GRAVITY_G 9.81f

// Check functions
static float calculate_dynamic_magnitude(const sensor_reading_t *data)
{
    float z_dynamic = data->z - GRAVITY_G;
    return sqrtf(data->x * data->x + data->y * data->y + z_dynamic * z_dynamic);
}

static bool check_crash(const sensor_reading_t *data, float threshold)
{
    return calculate_dynamic_magnitude(data) >= threshold;
}

static bool check_harsh_braking(const sensor_reading_t *data, float threshold)
{
    return data->y <= -threshold;
}

static bool check_harsh_accel(const sensor_reading_t *data, float threshold)
{
    return data->y >= threshold;
}

static bool check_harsh_cornering(const sensor_reading_t *data, float threshold)
{
    return fabsf(data->x) >= threshold;
}

// Value functions (for logging)
static float get_crash_value(const sensor_reading_t *data)
{
    return calculate_dynamic_magnitude(data);
}

static float get_braking_value(const sensor_reading_t *data)
{
    return data->y;
}

static float get_accel_value(const sensor_reading_t *data)
{
    return data->y;
}

static float get_cornering_value(const sensor_reading_t *data)
{
    return data->x;
}

// Detector table
detector_config_t detectors[DETECTOR_COUNT] = {
    [DETECTOR_CRASH] = {
        .name = "crash",
        .default_threshold = DEFAULT_CRASH_THRESHOLD_G,
        .is_crash = true,
        .check = check_crash,
        .get_value = get_crash_value,
        .on_trigger = triggerWarningCountdown,
    },
    [DETECTOR_HARSH_BRAKING] = {
        .name = "harsh_braking",
        .default_threshold = DEFAULT_HARSH_BRAKING_THRESHOLD_G,
        .is_crash = false,
        .warning_event = WARNING_HARSH_BRAKING,
        .check = check_harsh_braking,
        .get_value = get_braking_value,
        .on_trigger = triggerNormalWarning,
    },
    [DETECTOR_HARSH_ACCEL] = {
        .name = "harsh_accel",
        .default_threshold = DEFAULT_HARSH_ACCEL_THRESHOLD_G,
        .is_crash = false,
        .warning_event = WARNING_HARSH_ACCEL,
        .check = check_harsh_accel,
        .get_value = get_accel_value,
        .on_trigger = triggerNormalWarning,
    },
    [DETECTOR_HARSH_CORNERING] = {
        .name = "harsh_cornering",
        .default_threshold = DEFAULT_HARSH_CORNERING_THRESHOLD_G,
        .is_crash = false,
        .warning_event = WARNING_HARSH_CORNERING,
        .check = check_harsh_cornering,
        .get_value = get_cornering_value,
        .on_trigger = triggerNormalWarning,
    },
};
