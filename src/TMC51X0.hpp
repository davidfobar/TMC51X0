// ----------------------------------------------------------------------------
// TMC51X0.hpp
//
// Authors:
// Peter Polidoro peter@polidoro.io
// ----------------------------------------------------------------------------

#ifndef TMC51X0_HPP
#define TMC51X0_HPP
#include <Arduino.h>
#include <SPI.h>

#include "Registers.hpp"
#include "Driver.hpp"
#include "Controller.hpp"
#include "Encoder.hpp"


struct TMC51X0
{
  void setup(SPIClass & spi,
    size_t chip_select_pin);

  tmc51x0::Registers registers;
  tmc51x0::Driver driver;
  tmc51x0::Controller controller;
  tmc51x0::Encoder encoder;

  tmc51x0::Registers::Gstat readAndClearGlobalStatus();

private:
  // const static uint8_t VERSION_TMC5130 = 0x11;
  // const static uint8_t VERSION_TMC5160 = 0x30;

};

#endif
