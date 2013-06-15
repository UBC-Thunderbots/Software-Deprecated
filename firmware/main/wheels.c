#include "wheels.h"
#include "control.h"
#include "encoder.h"
#include "motor.h"
#include "power.h"
#include <stdbool.h>
#include <string.h>

wheels_mode_t wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION;
wheels_setpoints_t wheels_setpoints;
int16_t wheels_encoder_counts[4] = { 0, 0, 0, 0 };
int16_t wheels_drives[4] = { 0, 0, 0, 0 };

void wheels_tick(float battery) {
	// Read optical encoders.
	for (uint8_t i = 0; i < 4; ++i) {
		wheels_encoder_counts[i] = read_encoder(i);
	}

	switch (wheels_mode) {
		case WHEELS_MODE_MANUAL_COMMUTATION:
		case WHEELS_MODE_BRAKE:
			{
				// The controller is not used here and should be cleared.
				control_clear();

				// In these modes, we send the PWM duty cycle given in the setpoint.
				// Safety interlocks normally prevent it from ever appearing outside the chip, but if interlocks are overridden, manual commutation can send PWM to a phase.
				motor_mode_t mmode = wheels_mode == WHEELS_MODE_MANUAL_COMMUTATION ? MOTOR_MODE_MANUAL_COMMUTATION : MOTOR_MODE_BRAKE;
				for (uint8_t i = 0; i < 4; ++i) {
					motor_set_wheel(i, mmode, wheels_setpoints.wheels[i]);
				}

				// For reporting purposes, the drive levels are considered to be zero here.
				memset(wheels_drives, 0, sizeof(wheels_drives));

#warning TODO update thermal model
			}
			break;

		case WHEELS_MODE_OPEN_LOOP:
		case WHEELS_MODE_CLOSED_LOOP:
			{
				// Compute drive level, either by running the controller or by clearing the controller and taking the setpoints directly.
				if (wheels_mode == WHEELS_MODE_OPEN_LOOP) {
					control_clear();
					memcpy(wheels_drives, wheels_setpoints.wheels, sizeof(wheels_drives));
				} else {
					control_tick(battery);
				}

				// Clamp all drive levels to ±255.
				for (uint8_t i = 0; i < 4; ++i) {
					if (wheels_drives[i] < -255) {
						wheels_drives[i] = -255;
					} else if (wheels_drives[i] > 255) {
						wheels_drives[i] = 255;
					}
				}

				// Construct a bitmask of which motors we will drive (by default, all of them).
				uint8_t drive_mask = 0x0F;

				// Safety interlock: if an optical encoder fails, we must coast that motor as there is no provably safe duty cycle that will avoid over-current.
				if (!interlocks_overridden()) {
					drive_mask &= ~ENCODER_FAIL;
				}

#warning TODO safety interlock: update thermal model and disable hot motors

				// Drive the motors.
				for (uint8_t i = 0; i < 4; ++i) {
					if (drive_mask & 1) {
						if (wheels_drives[i] >= 0) {
							motor_set_wheel(i, MOTOR_MODE_FORWARD, wheels_drives[i]);
						} else {
							motor_set_wheel(i, MOTOR_MODE_BACKWARD, -wheels_drives[i]);
						}
					} else {
						motor_set_wheel(i, MOTOR_MODE_MANUAL_COMMUTATION, 0);
					}
					drive_mask >>= 1;
				}
			}
			break;
	}
}

