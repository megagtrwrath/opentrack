/* Copyright (c) 2023, Khoa Nguyen <khoanguyen@3forcom.com>
 * Linux gaze_origin port (c) 2026
 *
 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#include "tobii.h"
#include "compat/math-imports.hpp"

#include <QMutexLocker>
#include <QThread>
#include <cmath>
#include <dlfcn.h>

static constexpr double rad_to_deg = 180.0 * M_1_PI;
static constexpr double mm_to_cm = 0.1;

static void url_receiver(char const* url, void* user_data)
{
    char* buffer = (char*)user_data;
    if (*buffer != '\0')
        return; // only keep first value

    if (strlen(url) < 256)
        strcpy(buffer, url);
}

static void gaze_origin_callback(tobii_gaze_origin_t const* gaze_origin, void* user_data)
{
    tobii_gaze_origin_t* storage = (tobii_gaze_origin_t*)user_data;
    *storage = *gaze_origin;
}

tobii_tracker::tobii_tracker() = default;

tobii_tracker::~tobii_tracker()
{
    QMutexLocker lck(&mtx);
    if (device)
    {
        tobii_gaze_origin_unsubscribe(device);
        tobii_device_destroy(device);
    }
    if (api)
    {
        tobii_api_destroy(api);
    }
}

module_status tobii_tracker::start_tracker(QFrame*)
{
    QMutexLocker lck(&mtx);
    tobii_error_t tobii_error = tobii_api_create(&api, nullptr, nullptr);
    if (tobii_error != TOBII_ERROR_NO_ERROR)
    {
        return error("Failed to initialize the Tobii Stream Engine API.");
    }

    char url[256] = { 0 };
    tobii_error = tobii_enumerate_local_device_urls(api, url_receiver, url);
    if (tobii_error != TOBII_ERROR_NO_ERROR || url[0] == '\0')
    {
        tobii_api_destroy(api);
        api = nullptr;
        return error("No stream engine compatible device(s) found.");
    }

    // The installed header declares tobii_device_create with 3 args, but the
    // actual Tobii Stream Engine library requires 4 args including field_of_use.
    // Use dlsym to call with the correct 4-arg signature.
    typedef tobii_error_t (*device_create_fn_t)(tobii_api_t*, char const*, int, tobii_device_t**);
    auto real_device_create = (device_create_fn_t)dlsym(RTLD_DEFAULT, "tobii_device_create");
    if (!real_device_create)
    {
        tobii_api_destroy(api);
        api = nullptr;
        return error("Failed to resolve tobii_device_create via dlsym.");
    }
    tobii_error = real_device_create(api, url, 1 /* FIELD_OF_USE_INTERACTIVE */, &device);
    if (tobii_error != TOBII_ERROR_NO_ERROR)
    {
        // Retry once after a short delay
        QThread::msleep(500);
        tobii_error = real_device_create(api, url, 1, &device);
    }
    if (tobii_error != TOBII_ERROR_NO_ERROR)
    {
        const char* msg = tobii_error_message(tobii_error);
        tobii_api_destroy(api);
        api = nullptr;
        return error(QString("Failed to connect to %1 (error %2: %3).")
                     .arg(url).arg((int)tobii_error).arg(msg ? msg : "unknown"));
    }

    // Use gaze_origin instead of head_pose -- head_pose is not supported
    // on Tobii Eye Tracker 5 under Linux stream engine.
    tobii_error = tobii_gaze_origin_subscribe(device, gaze_origin_callback, &latest_gaze_origin);
    if (tobii_error != TOBII_ERROR_NO_ERROR)
    {
        tobii_device_destroy(device);
        device = nullptr;
        tobii_api_destroy(api);
        api = nullptr;
        return error("Failed to subscribe to gaze origin stream.");
    }

    return status_ok();
}

void tobii_tracker::data(double* data)
{
    QMutexLocker lck(&mtx);
    tobii_error_t tobii_error = tobii_device_process_callbacks(device);
    if (tobii_error != TOBII_ERROR_NO_ERROR)
    {
        return;
    }

    const auto& g = latest_gaze_origin;
    const bool left_ok  = g.left_validity  == TOBII_VALIDITY_VALID;
    const bool right_ok = g.right_validity == TOBII_VALIDITY_VALID;

    if (!left_ok && !right_ok)
        return;

    // Compute midpoint of available eyes (in mm, Tobii coords)
    float mx, my, mz;
    if (left_ok && right_ok)
    {
        mx = (g.left_xyz[0] + g.right_xyz[0]) * 0.5f;
        my = (g.left_xyz[1] + g.right_xyz[1]) * 0.5f;
        mz = (g.left_xyz[2] + g.right_xyz[2]) * 0.5f;
    }
    else if (left_ok)
    {
        mx = g.left_xyz[0]; my = g.left_xyz[1]; mz = g.left_xyz[2];
    }
    else
    {
        mx = g.right_xyz[0]; my = g.right_xyz[1]; mz = g.right_xyz[2];
    }

    // Capture baseline on first valid binocular frame
    if (!baseline_set && left_ok && right_ok)
    {
        baseline_xyz[0] = mx;
        baseline_xyz[1] = my;
        baseline_xyz[2] = mz;
        float dx = g.right_xyz[0] - g.left_xyz[0];
        float dy = g.right_xyz[1] - g.left_xyz[1];
        float dz = g.right_xyz[2] - g.left_xyz[2];
        baseline_ipd = std::sqrt(dx*dx + dy*dy + dz*dz);
        baseline_set = true;
    }

    // Translation: displacement from baseline in mm, convert to cm
    // Tobii: +x right, +y up, +z toward user
    // OpenTrack: +x right, +y up, +z forward (into screen)
    data[TX] = -(mx - baseline_xyz[0]) * mm_to_cm;
    data[TY] =  (my - baseline_xyz[1]) * mm_to_cm;
    data[TZ] =  (mz - baseline_xyz[2]) * mm_to_cm;

    // Derive yaw and roll from inter-eye vector when both eyes valid
    if (left_ok && right_ok)
    {
        float dx = g.right_xyz[0] - g.left_xyz[0];
        float dy = g.right_xyz[1] - g.left_xyz[1];
        float dz = g.right_xyz[2] - g.left_xyz[2];

        // Yaw: rotation around Y axis -- when head turns, one eye moves
        // forward/back relative to the other
        data[Yaw] = -std::atan2(dz, dx) * rad_to_deg;

        // Roll: tilt of the inter-eye line
        data[Roll] = std::atan2(dy, dx) * rad_to_deg;

        // Pitch: approximate from Z displacement relative to baseline
        // (moving head forward/back changes the midpoint Z)
        if (baseline_set && baseline_xyz[2] > 1.f)
        {
            float dz_mid = mz - baseline_xyz[2];
            // Map Z displacement to a pitch angle -- rough approximation
            // 100mm forward ~ looking down ~15 degrees
            data[Pitch] = -(dz_mid / baseline_xyz[2]) * 60.0;
        }
    }
}

OPENTRACK_DECLARE_TRACKER(tobii_tracker, tobii_dialog, tobii_metadata)
