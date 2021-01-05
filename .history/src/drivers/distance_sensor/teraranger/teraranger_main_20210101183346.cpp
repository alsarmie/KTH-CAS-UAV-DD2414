/****************************************************************************
 *
 *   Copyright (c) 2013-2020 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include "TERARANGER.hpp"

#include "TERARANGER.cpp"

#include <px4_platform_common/getopt.h>
#include <px4_platform_common/module.h>

void
TERARANGER::print_usage()
{
	PRINT_MODULE_DESCRIPTION(
		R"DESCR_STR(
### Description

I2C bus driver for TeraRanger rangefinders.

The sensor/driver must be enabled using the parameter SENS_EN_TRANGER.

Setup/usage information: https://docs.px4.io/master/en/sensor/rangefinders.html#teraranger-rangefinders
)DESCR_STR");
	PRINT_MODULE_USAGE_NAME("teraranger", "driver");
	PRINT_MODULE_USAGE_SUBCATEGORY("distance_sensor");
	PRINT_MODULE_USAGE_COMMAND("start");
	PRINT_MODULE_USAGE_PARAMS_I2C_SPI_DRIVER(true, false);
	PRINT_MODULE_USAGE_PARAM_INT('R', 25, 0, 25, "Sensor rotation - downward facing by default", true);
	PRINT_MODULE_USAGE_DEFAULT_COMMANDS();
}

I2CSPIDriverBase *TERARANGER::instantiate(const BusCLIArguments &cli, const BusInstanceIterator &iterator,
				     int runtime_instance)
{
	TERARANGER *instance = new TERARANGER(iterator.configuredBusOption(), iterator.bus(), cli.orientation, cli.bus_frequency);

	if (instance == nullptr) {
		PX4_ERR("alloc failed");
		return nullptr;
	}

	if (instance->init() != PX4_OK) {
		delete instance;
		return nullptr;
	}

	instance->start();
	return instance;
}

int probe_sensor()
{
	uint8_t who_am_i = 0;

	const uint8_t cmd = TERARANGER_WHO_AM_I_REG;

	// Can't use a single transfer as Teraranger needs a bit of time for internal processing.
	if (I2C::transfer(&cmd, 1, nullptr, 0) == OK) {
		if (I2C::transfer(nullptr, 0, &who_am_i, 1) == OK && who_am_i == TERARANGER_WHO_AM_I_REG_VAL) {
			return measure();
		}
	}

	PX4_INFO("WHO_AM_I byte mismatch 0x%02x should be 0x%02x\n",
		  (unsigned)who_am_i,
		  TERARANGER_WHO_AM_I_REG_VAL);

	// Not found on any address.
	return -EIO;
}

extern "C" __EXPORT int teraranger_main(int argc, char *argv[])
{
	int ch;
	using ThisDriver = TERARANGER;
	BusCLIArguments cli{true, false};
	cli.orientation = distance_sensor_s::ROTATION_DOWNWARD_FACING;
	cli.default_i2c_frequency = 400000;
	int probing = probe_sensor();
	printf("Is there a Teraranger? %d\n",probing);

	while ((ch = cli.getopt(argc, argv, "R:")) != EOF) {
		switch (ch) {
		case 'R':
			cli.orientation = atoi(cli.optarg());
			break;
		}
	}

	const char *verb = cli.optarg();

	if (!verb) {
		ThisDriver::print_usage();
		return -1;
	}

	BusInstanceIterator iterator(MODULE_NAME, cli, DRV_DIST_DEVTYPE_TERARANGER);

	if (!strcmp(verb, "start")) {

		static int a {-1};

		//while(a == -1){
			a = ThisDriver::module_start(cli, iterator);
			printf(" Start module failed?  %d \n",a);
		//}
		return a;//ThisDriver::module_start(cli, iterator);
	}

	if (!strcmp(verb, "stop")) {
		return ThisDriver::module_stop(iterator);
	}

	if (!strcmp(verb, "status")) {
		return ThisDriver::module_status(iterator);
	}

	ThisDriver::print_usage();
	return -1;
}
