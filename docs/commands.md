# Exposed Variables

 - `particle get <device> state` will provide device settings information (FIXME currently incomplete because of the 600 char limit)
 - `particle get <device> data` will provide device data information

# Available Commands

The following commands can be used directly through the CLI call `particle call <deviceID> device "<cmd>"`.

## [`LoggerController`](/src/modules/logger/LoggerController.h) commands:

The following commands are available for all loggers. Additional commands are provided by individual components listed hereafter.

  - `state-log on` to turn web logging of state changes on (letter `S` shown in the state overview)
  - `state-log off` to turn web logging of state changes off (no letter `S` in state overview)
  - `data-log on` to turn web logging of data on (letter `D` in state overview)
  - `data-log off` to turn web logging of data off
  - `sd-log on` to turn logging to SD card on (FIXME not actually implemented)
  - `sd-log off` to turn logging to SD card off
  - `sd-test` to test whether writing to the SD card works (writes a test file to the card and reads it back)
  - `log-period <options>` to specify how frequently data should be logged (after letter `D` in state overview, although the `D` only appears if data logging is actually enabled), `<options>`:
    - `3 x` log after every 3rd (or any other number) successful data read (`D3x`), works with `manual` or time based `read-period`, set to `1 x` in combination with `manual` to log every externally triggered data event immediately (**FIXME**: not fully implemented)
    - `2 s` log every 2 seconds (or any other number), must exceed the `read-period` (`D2s` in state overview)
    - `8 m` log every 8 minutes (or any other number)
    - `1 h` log every hour (or any other number)
  - `read-period <options>` to specify how frequently data should be read (letter `R` + subsequent in state overview), only applicable if the controller is set up to be a data reader, `<options>`:
    - `manual` don't read data unless externally triggered in some way (device specific) - `RM` in state overview
    - `200 ms` read data every 200 (or any other number) milli seconds (`R200ms` in state overview)
    - `5 s` read data every 5 (or any other number) seconds (`R5s` in state overview)
  - `lock on` to safely lock the device (i.e. no commands will be accepted until `lock off` is called) - letter `L` in state overview
  - `lock off` to unlock the device if it is locked
  - `debug on` to turn debug mode on which leads to more data reporting in newer loggers (older loggers don't have this option yet)
  - `debug off` to turn debug mode off
  - `tz <x>` to set the timezone to the value of x (the offset from UTC/GMT, e.g. -6 or 4) - this is purely for command processing, all data logs are always with UTC time stamps
  - `restart` to force a restart
  - `reset state` to completely reset the state back to the default values (forces a restart after reset is complete)
  - `reset data` to reset the data currently being collected
  - `page` to switch to the next page on the LCD screen (**FIXME**: not fully implemented)

## [`LoggerDisplay`](/src/modules/logger/LoggerDisplay.h) commands:

The following commands only work for newer generation screens and will have no effect on older screens.

 - `lcd power on` to turn the LCD display on (the default)
 - `lcd power off` to turn the LCD display off (can be useful for saving energy when running on battery)
 - `lcd reset` to reset the LCD in case any of the letters got corrupted (can happen sometimes when running for a long time)
 - `lcd contrast <x>` to set the LCD contrast to the value of `<x>` (in % from 0 to 100)
 - `lcd color <r>,<g>,<b>` to set the LCD color to the standard RGB combination of red  (`<r>`), green (`<g>`) and blue (`<b>`) - each as a number from 0 to 255
 - `lcd color <x>` to set the LCD color to the color `<x>` - works only for a handful of useful colors that are preprogrammed such as *red*, *green*, *cyan*, *pink*, *orange*, *yellow*, *purple*


## [`RelayLoggerComponent`](/src/modules/relay/RelayLoggerComponent.h) commands:

  - `<id> on` to turn the relay with the name `<id>`> on (takes into account whether the relay is normally closed or normally open)
  - `<id> off` to turn the relay off

# [`ValveLoggerComponent`](/src/modules/valve/ValveLoggerComponent.h) commands:

  - `<id> pos x` to move the valve with the name `<id>` to position x
  - `<id> dir cw` to set the valve to turning clockwise whenever it goes to a new position
  - `<id> dir cc` to set the valve to turning counterclockwise whenever it goes to a new position

# [`SchedulerLoggerComponent`](/src/modules/scheduler/SchedulerLoggerComponent.h) commands:

  - `<id> set YYYY-MM-DD HH:MM` to set the scheduler with name `<id>` to start its events scheduler at the specific time (must be in this format!)
  - `<id> reset` to reset the scheduler with name `<id>` to be unscheduled
  - `<id> test x` to test the events schedule (does not need to have a set time). The `x` is optionally and if provided sets all wait times between events to `x` number of seconds to allow for fast testing of schedules that usually span much longer wait times.
