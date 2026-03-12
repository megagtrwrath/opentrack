/* Copyright (c) 2023, Khoa Nguyen <khoanguyen@3forcom.com>
 * Linux gaze_origin port (c) 2026
 *
 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#pragma once
#include "api/plugin-api.hpp"
#include "ui_tobii.h"

#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>

#include <QMutex>

class tobii_tracker : public ITracker
{
public:
    tobii_tracker();
    ~tobii_tracker() override;
    module_status start_tracker(QFrame*) override;
    void data(double* data) override;

private:
    tobii_api_t* api = nullptr;
    tobii_device_t* device = nullptr;

    // We use gaze_origin (eye 3D positions in mm) instead of head_pose,
    // because head_pose is not supported on Tobii Eye Tracker 5 under Linux.
    tobii_gaze_origin_t latest_gaze_origin{};

    // Baseline inter-eye distance captured during first valid frame (mm)
    float baseline_ipd = 0.f;
    // Baseline midpoint captured during first valid frame (mm)
    float baseline_xyz[3] = { 0.f, 0.f, 0.f };
    bool baseline_set = false;

    QMutex mtx;
};

class tobii_dialog : public ITrackerDialog
{
    Q_OBJECT

    Ui::tobii_ui ui;

public:
    tobii_dialog();

private slots:
    void doOK();
    void doCancel();
};

class tobii_metadata : public Metadata
{
    Q_OBJECT

    QString name() override { return tr("Tobii Eye Tracker"); }
    QIcon icon() override { return QIcon(":/images/tobii_logo.png"); }
};
