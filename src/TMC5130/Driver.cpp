// ----------------------------------------------------------------------------
// Driver.cpp
//
// Authors:
// Peter Polidoro peter@polidoro.io
// ----------------------------------------------------------------------------
#include "Driver.h"


Driver::Driver()
{
  hardware_serial_ptr_ = nullptr;
#if SOFTWARE_SERIAL_IMPLEMENTED
  software_serial_ptr_ = nullptr;
#endif
  serial_baud_rate_ = 115200;
  serial_address_ = SERIAL_ADDRESS_0;
  hardware_enable_pin_ = -1;
  cool_step_enabled_ = false;
}

#ifdef ESP32
void Driver::setup(HardwareSerial & serial,
  long serial_baud_rate,
  SerialAddress serial_address,
  int16_t alternate_rx_pin,
  int16_t alternate_tx_pin)
{
  hardware_serial_ptr_ = &serial;
  if ((alternate_rx_pin < 0) || (alternate_tx_pin < 0))
  {
    hardware_serial_ptr_->end(false);
    hardware_serial_ptr_->begin(serial_baud_rate);
  }
  else
  {
    hardware_serial_ptr_->end();
    hardware_serial_ptr_->begin(serial_baud_rate, SERIAL_8N1, alternate_rx_pin, alternate_tx_pin);
  }

  initialize(serial_baud_rate, serial_address);
}
#else
void Driver::setup(HardwareSerial & serial,
  long serial_baud_rate,
  SerialAddress serial_address)
{
  hardware_serial_ptr_ = &serial;
  hardware_serial_ptr_->end();
  hardware_serial_ptr_->begin(serial_baud_rate);

  initialize(serial_baud_rate, serial_address);
}
#endif

#if SOFTWARE_SERIAL_IMPLEMENTED
void Driver::setup(SoftwareSerial & serial,
  long serial_baud_rate,
  SerialAddress serial_address)
{
  software_serial_ptr_ = &serial;
  software_serial_ptr_->end();
  software_serial_ptr_->begin(serial_baud_rate);

  initialize(serial_baud_rate, serial_address);
}
#endif

// unidirectional methods

void Driver::setHardwareEnablePin(uint8_t hardware_enable_pin)
{
  hardware_enable_pin_ = hardware_enable_pin;
  pinMode(hardware_enable_pin_, OUTPUT);
  digitalWrite(hardware_enable_pin_, HIGH);
}

void Driver::enable()
{
  if (hardware_enable_pin_ >= 0)
  {
    digitalWrite(hardware_enable_pin_, LOW);
  }
  chopper_config_.toff = toff_;
  writeStoredChopperConfig();
}

void Driver::disable()
{
  if (hardware_enable_pin_ >= 0)
  {
    digitalWrite(hardware_enable_pin_, HIGH);
  }
  chopper_config_.toff = TOFF_DISABLE;
  writeStoredChopperConfig();
}

void Driver::setMicrostepsPerStep(uint16_t microsteps_per_step)
{
  uint16_t microsteps_per_step_shifted = constrain_(microsteps_per_step,
    MICROSTEPS_PER_STEP_MIN,
    MICROSTEPS_PER_STEP_MAX);
  microsteps_per_step_shifted = microsteps_per_step >> 1;
  uint16_t exponent = 0;
  while (microsteps_per_step_shifted > 0)
  {
    microsteps_per_step_shifted = microsteps_per_step_shifted >> 1;
    ++exponent;
  }
  setMicrostepsPerStepPowerOfTwo(exponent);
}

void Driver::setMicrostepsPerStepPowerOfTwo(uint8_t exponent)
{
  switch (exponent)
  {
    case 0:
    {
      chopper_config_.mres = MRES_001;
      break;
    }
    case 1:
    {
      chopper_config_.mres = MRES_002;
      break;
    }
    case 2:
    {
      chopper_config_.mres = MRES_004;
      break;
    }
    case 3:
    {
      chopper_config_.mres = MRES_008;
      break;
    }
    case 4:
    {
      chopper_config_.mres = MRES_016;
      break;
    }
    case 5:
    {
      chopper_config_.mres = MRES_032;
      break;
    }
    case 6:
    {
      chopper_config_.mres = MRES_064;
      break;
    }
    case 7:
    {
      chopper_config_.mres = MRES_128;
      break;
    }
    case 8:
    default:
    {
      chopper_config_.mres = MRES_256;
      break;
    }
  }
  writeStoredChopperConfig();
}

void Driver::setRunCurrent(uint8_t percent)
{
  uint8_t run_current = percentToCurrentSetting(percent);
  driver_current_.irun = run_current;
  writeStoredDriverCurrent();
}

void Driver::setHoldCurrent(uint8_t percent)
{
  uint8_t hold_current = percentToCurrentSetting(percent);

  driver_current_.ihold = hold_current;
  writeStoredDriverCurrent();
}

void Driver::setHoldDelay(uint8_t percent)
{
  uint8_t hold_delay = percentToHoldDelaySetting(percent);

  driver_current_.iholddelay = hold_delay;
  writeStoredDriverCurrent();
}

void Driver::setAllCurrentValues(uint8_t run_current_percent,
  uint8_t hold_current_percent,
  uint8_t hold_delay_percent)
{
  uint8_t run_current = percentToCurrentSetting(run_current_percent);
  uint8_t hold_current = percentToCurrentSetting(hold_current_percent);
  uint8_t hold_delay = percentToHoldDelaySetting(hold_delay_percent);

  driver_current_.irun = run_current;
  driver_current_.ihold = hold_current;
  driver_current_.iholddelay = hold_delay;
  writeStoredDriverCurrent();
}

void Driver::enableInverseMotorDirection()
{
  global_config_.shaft = 1;
  writeStoredGlobalConfig();
}

void Driver::disableInverseMotorDirection()
{
  global_config_.shaft = 0;
  writeStoredGlobalConfig();
}

void Driver::setStandstillMode(Driver::StandstillMode mode)
{
  pwm_config_.freewheel = mode;
  writeStoredPwmConfig();
}

void Driver::enableAutomaticCurrentScaling()
{
  pwm_config_.pwm_autoscale = STEPPER_DRIVER_FEATURE_ON;
  writeStoredPwmConfig();
}

void Driver::disableAutomaticCurrentScaling()
{
  pwm_config_.pwm_autoscale = STEPPER_DRIVER_FEATURE_OFF;
  writeStoredPwmConfig();
}

void Driver::enableAutomaticGradientAdaptation()
{
  pwm_config_.pwm_autograd = STEPPER_DRIVER_FEATURE_ON;
  writeStoredPwmConfig();
}

void Driver::disableAutomaticGradientAdaptation()
{
  pwm_config_.pwm_autograd = STEPPER_DRIVER_FEATURE_OFF;
  writeStoredPwmConfig();
}

void Driver::setPwmOffset(uint8_t pwm_amplitude)
{
  pwm_config_.pwm_offset = pwm_amplitude;
  writeStoredPwmConfig();
}

void Driver::setPwmGradient(uint8_t pwm_amplitude)
{
  pwm_config_.pwm_grad = pwm_amplitude;
  writeStoredPwmConfig();
}

void Driver::setPowerDownDelay(uint8_t power_down_delay)
{
  write(ADDRESS_TPOWERDOWN, power_down_delay);
}

void Driver::setReplyDelay(uint8_t reply_delay)
{
  if (reply_delay > REPLY_DELAY_MAX)
  {
    reply_delay = REPLY_DELAY_MAX;
  }
  ReplyDelay reply_delay_data;
  reply_delay_data.bytes = 0;
  reply_delay_data.replydelay = reply_delay;
  write(ADDRESS_REPLYDELAY, reply_delay_data.bytes);
}

void Driver::moveAtVelocity(int32_t microsteps_per_period)
{
  write(ADDRESS_VACTUAL, microsteps_per_period);
}

void Driver::moveUsingStepDirInterface()
{
  write(ADDRESS_VACTUAL, VACTUAL_STEP_DIR_INTERFACE);
}

void Driver::enableStealthChop()
{
  global_config_.enable_spread_cycle = 0;
  writeStoredGlobalConfig();
}

void Driver::disableStealthChop()
{
  global_config_.enable_spread_cycle = 1;
  writeStoredGlobalConfig();
}

void Driver::setCoolStepDurationThreshold(uint32_t duration_threshold)
{
  write(ADDRESS_TCOOLTHRS, duration_threshold);
}

void Driver::setStealthChopDurationThreshold(uint32_t duration_threshold)
{
  write(ADDRESS_TPWMTHRS, duration_threshold);
}

void Driver::setStallGuardThreshold(uint8_t stall_guard_threshold)
{
  write(ADDRESS_SGTHRS, stall_guard_threshold);
}

void Driver::enableCoolStep(uint8_t lower_threshold,
    uint8_t upper_threshold)
{
  lower_threshold = constrain_(lower_threshold, SEMIN_MIN, SEMIN_MAX);
  cool_config_.semin = lower_threshold;
  upper_threshold = constrain_(upper_threshold, SEMAX_MIN, SEMAX_MAX);
  cool_config_.semax = upper_threshold;
  write(ADDRESS_COOLCONF, cool_config_.bytes);
  cool_step_enabled_ = true;
}

void Driver::disableCoolStep()
{
  cool_config_.semin = SEMIN_OFF;
  write(ADDRESS_COOLCONF, cool_config_.bytes);
  cool_step_enabled_ = false;
}

void Driver::setCoolStepCurrentIncrement(CurrentIncrement current_increment)
{
  cool_config_.seup = current_increment;
  write(ADDRESS_COOLCONF, cool_config_.bytes);
}

void Driver::setCoolStepMeasurementCount(MeasurementCount measurement_count)
{
  cool_config_.sedn = measurement_count;
  write(ADDRESS_COOLCONF, cool_config_.bytes);
}

void Driver::enableAnalogCurrentScaling()
{
  global_config_.i_scale_analog = 1;
  writeStoredGlobalConfig();
}

void Driver::disableAnalogCurrentScaling()
{
  global_config_.i_scale_analog = 0;
  writeStoredGlobalConfig();
}

void Driver::useExternalSenseResistors()
{
  global_config_.internal_rsense = 0;
  writeStoredGlobalConfig();
}

void Driver::useInternalSenseResistors()
{
  global_config_.internal_rsense = 1;
  writeStoredGlobalConfig();
}

// bidirectional methods

uint8_t Driver::getVersion()
{
  Input input;
  input.bytes = read(ADDRESS_IOIN);

  return input.version;
}

bool Driver::isCommunicating()
{
  return (getVersion() == VERSION);
}

bool Driver::isSetupAndCommunicating()
{
  return serialOperationMode();
}

bool Driver::isCommunicatingButNotSetup()
{
  return (isCommunicating() && (not isSetupAndCommunicating()));
}

bool Driver::hardwareDisabled()
{
  Input input;
  input.bytes = read(ADDRESS_IOIN);

  return input.enn;
}

uint16_t Driver::getMicrostepsPerStep()
{
  uint16_t microsteps_per_step_exponent;
  switch (chopper_config_.mres)
  {
    case MRES_001:
    {
      microsteps_per_step_exponent = 0;
      break;
    }
    case MRES_002:
    {
      microsteps_per_step_exponent = 1;
      break;
    }
    case MRES_004:
    {
      microsteps_per_step_exponent = 2;
      break;
    }
    case MRES_008:
    {
      microsteps_per_step_exponent = 3;
      break;
    }
    case MRES_016:
    {
      microsteps_per_step_exponent = 4;
      break;
    }
    case MRES_032:
    {
      microsteps_per_step_exponent = 5;
      break;
    }
    case MRES_064:
    {
      microsteps_per_step_exponent = 6;
      break;
    }
    case MRES_128:
    {
      microsteps_per_step_exponent = 7;
      break;
    }
    case MRES_256:
    default:
    {
      microsteps_per_step_exponent = 8;
      break;
    }
  }
  return 1 << microsteps_per_step_exponent;
}

Driver::Settings Driver::getSettings()
{
  Settings settings;
  settings.is_communicating = isCommunicating();

  if (settings.is_communicating)
  {
    readAndStoreRegisters();

    settings.is_setup = global_config_.pdn_disable;
    settings.software_enabled = (chopper_config_.toff > TOFF_DISABLE);
    settings.microsteps_per_step = getMicrostepsPerStep();
    settings.inverse_motor_direction_enabled = global_config_.shaft;
    settings.stealth_chop_enabled = not global_config_.enable_spread_cycle;
    settings.standstill_mode = pwm_config_.freewheel;
    settings.irun_percent = currentSettingToPercent(driver_current_.irun);
    settings.irun_register_value = driver_current_.irun;
    settings.ihold_percent = currentSettingToPercent(driver_current_.ihold);
    settings.ihold_register_value = driver_current_.ihold;
    settings.iholddelay_percent = holdDelaySettingToPercent(driver_current_.iholddelay);
    settings.iholddelay_register_value = driver_current_.iholddelay;
    settings.automatic_current_scaling_enabled = pwm_config_.pwm_autoscale;
    settings.automatic_gradient_adaptation_enabled = pwm_config_.pwm_autograd;
    settings.pwm_offset = pwm_config_.pwm_offset;
    settings.pwm_gradient = pwm_config_.pwm_grad;
    settings.cool_step_enabled = cool_step_enabled_;
    settings.analog_current_scaling_enabled = global_config_.i_scale_analog;
    settings.internal_sense_resistors_enabled = global_config_.internal_rsense;
  }
  else
  {
    settings.is_setup = false;
    settings.software_enabled = false;
    settings.microsteps_per_step = 0;
    settings.inverse_motor_direction_enabled = false;
    settings.stealth_chop_enabled = false;
    settings.standstill_mode = pwm_config_.freewheel;
    settings.irun_percent = 0;
    settings.irun_register_value = 0;
    settings.ihold_percent = 0;
    settings.ihold_register_value = 0;
    settings.iholddelay_percent = 0;
    settings.iholddelay_register_value = 0;
    settings.automatic_current_scaling_enabled = false;
    settings.automatic_gradient_adaptation_enabled = false;
    settings.pwm_offset = 0;
    settings.pwm_gradient = 0;
    settings.cool_step_enabled = false;
    settings.analog_current_scaling_enabled = false;
    settings.internal_sense_resistors_enabled = false;
  }

  return settings;
}

Driver::Status Driver::getStatus()
{
  DriveStatus drive_status;
  drive_status.bytes = 0;
  drive_status.bytes = read(ADDRESS_DRV_STATUS);
  return drive_status.status;
}

uint8_t Driver::getInterfaceTransmissionCounter()
{
  return read(ADDRESS_IFCNT);
}

uint32_t Driver::getInterstepDuration()
{
  return read(ADDRESS_TSTEP);
}

uint16_t Driver::getStallGuardResult()
{
  return read(ADDRESS_SG_RESULT);
}

uint8_t Driver::getPwmScaleSum()
{
  PwmScale pwm_scale;
  pwm_scale.bytes = read(ADDRESS_PWM_SCALE);

  return pwm_scale.pwm_scale_sum;
}

int16_t Driver::getPwmScaleAuto()
{
  PwmScale pwm_scale;
  pwm_scale.bytes = read(ADDRESS_PWM_SCALE);

  return pwm_scale.pwm_scale_auto;
}

uint8_t Driver::getPwmOffsetAuto()
{
  PwmAuto pwm_auto;
  pwm_auto.bytes = read(ADDRESS_PWM_AUTO);

  return pwm_auto.pwm_offset_auto;
}

uint8_t Driver::getPwmGradientAuto()
{
  PwmAuto pwm_auto;
  pwm_auto.bytes = read(ADDRESS_PWM_AUTO);

  return pwm_auto.pwm_gradient_auto;
}

uint16_t Driver::getMicrostepCounter()
{
  return read(ADDRESS_MSCNT);
}

// private
void Driver::initialize(long serial_baud_rate,
  SerialAddress serial_address)
{
  serial_baud_rate_ = serial_baud_rate;

  setOperationModeToSerial(serial_address);
  setRegistersToDefaults();

  minimizeMotorCurrent();
  disable();
  disableAutomaticCurrentScaling();
  disableAutomaticGradientAdaptation();
}

int Driver::serialAvailable()
{
  if (hardware_serial_ptr_ != nullptr)
  {
    return hardware_serial_ptr_->available();
  }
#if SOFTWARE_SERIAL_IMPLEMENTED
  else if (software_serial_ptr_ != nullptr)
  {
    return software_serial_ptr_->available();
  }
#endif
  return 0;
}

size_t Driver::serialWrite(uint8_t c)
{
  if (hardware_serial_ptr_ != nullptr)
  {
    return hardware_serial_ptr_->write(c);
  }
#if SOFTWARE_SERIAL_IMPLEMENTED
  else if (software_serial_ptr_ != nullptr)
  {
    return software_serial_ptr_->write(c);
  }
#endif
  return 0;
}

int Driver::serialRead()
{
  if (hardware_serial_ptr_ != nullptr)
  {
    return hardware_serial_ptr_->read();
  }
#if SOFTWARE_SERIAL_IMPLEMENTED
  else if (software_serial_ptr_ != nullptr)
  {
    return software_serial_ptr_->read();
  }
#endif
  return 0;
}

void Driver::serialFlush()
{
  if (hardware_serial_ptr_ != nullptr)
  {
    return hardware_serial_ptr_->flush();
  }
}

void Driver::setOperationModeToSerial(SerialAddress serial_address)
{
  serial_address_ = serial_address;

  global_config_.bytes = 0;
  global_config_.i_scale_analog = 0;
  global_config_.pdn_disable = 1;
  global_config_.mstep_reg_select = 1;
  global_config_.multistep_filt = 1;

  writeStoredGlobalConfig();
}

void Driver::setRegistersToDefaults()
{
  driver_current_.bytes = 0;
  driver_current_.ihold = IHOLD_DEFAULT;
  driver_current_.irun = IRUN_DEFAULT;
  driver_current_.iholddelay = IHOLDDELAY_DEFAULT;
  write(ADDRESS_IHOLD_IRUN, driver_current_.bytes);

  chopper_config_.bytes = CHOPPER_CONFIG_DEFAULT;
  chopper_config_.tbl = TBL_DEFAULT;
  chopper_config_.hend = HEND_DEFAULT;
  chopper_config_.hstart = HSTART_DEFAULT;
  chopper_config_.toff = TOFF_DEFAULT;
  write(ADDRESS_CHOPCONF, chopper_config_.bytes);

  pwm_config_.bytes = PWM_CONFIG_DEFAULT;
  write(ADDRESS_PWMCONF, pwm_config_.bytes);

  cool_config_.bytes = COOLCONF_DEFAULT;
  write(ADDRESS_COOLCONF, cool_config_.bytes);

  write(ADDRESS_TPOWERDOWN, TPOWERDOWN_DEFAULT);
  write(ADDRESS_TPWMTHRS, TPWMTHRS_DEFAULT);
  write(ADDRESS_VACTUAL, VACTUAL_DEFAULT);
  write(ADDRESS_TCOOLTHRS, TCOOLTHRS_DEFAULT);
  write(ADDRESS_SGTHRS, SGTHRS_DEFAULT);
  write(ADDRESS_COOLCONF, COOLCONF_DEFAULT);
}

void Driver::readAndStoreRegisters()
{
  global_config_.bytes = readGlobalConfigBytes();
  chopper_config_.bytes = readChopperConfigBytes();
  pwm_config_.bytes = readPwmConfigBytes();
}

bool Driver::serialOperationMode()
{
  GlobalConfig global_config;
  global_config.bytes = readGlobalConfigBytes();

  return global_config.pdn_disable;
}

void Driver::minimizeMotorCurrent()
{
  driver_current_.irun = CURRENT_SETTING_MIN;
  driver_current_.ihold = CURRENT_SETTING_MIN;
  writeStoredDriverCurrent();
}

uint32_t Driver::reverseData(uint32_t data)
{
  uint32_t reversed_data = 0;
  uint8_t right_shift;
  uint8_t left_shift;
  for (uint8_t i=0; i<DATA_SIZE; ++i)
  {
    right_shift = (DATA_SIZE - i - 1) * BITS_PER_BYTE;
    left_shift = i * BITS_PER_BYTE;
    reversed_data |= ((data >> right_shift) & BYTE_MAX_VALUE) << left_shift;
  }
  return reversed_data;
}

template<typename Datagram>
uint8_t Driver::calculateCrc(Datagram & datagram,
  uint8_t datagram_size)
{
  uint8_t crc = 0;
  uint8_t byte;
  for (uint8_t i=0; i<(datagram_size - 1); ++i)
  {
    byte = (datagram.bytes >> (i * BITS_PER_BYTE)) & BYTE_MAX_VALUE;
    for (uint8_t j=0; j<BITS_PER_BYTE; ++j)
    {
      if ((crc >> 7) ^ (byte & 0x01))
      {
        crc = (crc << 1) ^ 0x07;
      }
      else
      {
        crc = crc << 1;
      }
      byte = byte >> 1;
    }
  }
  return crc;
}

template<typename Datagram>
void Driver::sendDatagramUnidirectional(Datagram & datagram,
  uint8_t datagram_size)
{
  uint8_t byte;

  for (uint8_t i=0; i<datagram_size; ++i)
  {
    byte = (datagram.bytes >> (i * BITS_PER_BYTE)) & BYTE_MAX_VALUE;
    serialWrite(byte);
  }
}

template<typename Datagram>
void Driver::sendDatagramBidirectional(Datagram & datagram,
  uint8_t datagram_size)
{
  uint8_t byte;

  // Wait for the transmission of outgoing serial data to complete
  serialFlush();

  // clear the serial receive buffer if necessary
  while (serialAvailable() > 0)
  {
    byte = serialRead();
  }

  // write datagram
  for (uint8_t i=0; i<datagram_size; ++i)
  {
    byte = (datagram.bytes >> (i * BITS_PER_BYTE)) & BYTE_MAX_VALUE;
    serialWrite(byte);
  }

  // Wait for the transmission of outgoing serial data to complete
  serialFlush();

  // wait for bytes sent out on TX line to be echoed on RX line
  uint32_t echo_delay = 0;
  while ((serialAvailable() < datagram_size) and
    (echo_delay < ECHO_DELAY_MAX_MICROSECONDS))
  {
    delayMicroseconds(ECHO_DELAY_INC_MICROSECONDS);
    echo_delay += ECHO_DELAY_INC_MICROSECONDS;
  }

  if (echo_delay >= ECHO_DELAY_MAX_MICROSECONDS)
  {
    return;
  }

  // clear RX buffer of echo bytes
  for (uint8_t i=0; i<datagram_size; ++i)
  {
    byte = serialRead();
  }
}

void Driver::write(uint8_t register_address,
  uint32_t data)
{
  WriteReadReplyDatagram write_datagram;
  write_datagram.bytes = 0;
  write_datagram.sync = SYNC;
  write_datagram.serial_address = serial_address_;
  write_datagram.register_address = register_address;
  write_datagram.rw = RW_WRITE;
  write_datagram.data = reverseData(data);
  write_datagram.crc = calculateCrc(write_datagram, WRITE_READ_REPLY_DATAGRAM_SIZE);

  sendDatagramUnidirectional(write_datagram, WRITE_READ_REPLY_DATAGRAM_SIZE);
}

uint32_t Driver::read(uint8_t register_address)
{
  ReadRequestDatagram read_request_datagram;
  read_request_datagram.bytes = 0;
  read_request_datagram.sync = SYNC;
  read_request_datagram.serial_address = serial_address_;
  read_request_datagram.register_address = register_address;
  read_request_datagram.rw = RW_READ;
  read_request_datagram.crc = calculateCrc(read_request_datagram, READ_REQUEST_DATAGRAM_SIZE);

  sendDatagramBidirectional(read_request_datagram, READ_REQUEST_DATAGRAM_SIZE);

  uint32_t reply_delay = 0;
  while ((serialAvailable() < WRITE_READ_REPLY_DATAGRAM_SIZE) and
    (reply_delay < REPLY_DELAY_MAX_MICROSECONDS))
  {
    delayMicroseconds(REPLY_DELAY_INC_MICROSECONDS);
    reply_delay += REPLY_DELAY_INC_MICROSECONDS;
  }

  if (reply_delay >= REPLY_DELAY_MAX_MICROSECONDS)
  {
    return 0;
  }

  uint64_t byte;
  uint8_t byte_count = 0;
  WriteReadReplyDatagram read_reply_datagram;
  read_reply_datagram.bytes = 0;
  for (uint8_t i=0; i<WRITE_READ_REPLY_DATAGRAM_SIZE; ++i)
  {
    byte = serialRead();
    read_reply_datagram.bytes |= (byte << (byte_count++ * BITS_PER_BYTE));
  }

  return reverseData(read_reply_datagram.data);
}

uint8_t Driver::percentToCurrentSetting(uint8_t percent)
{
  uint8_t constrained_percent = constrain_(percent,
    PERCENT_MIN,
    PERCENT_MAX);
  uint8_t current_setting = map(constrained_percent,
    PERCENT_MIN,
    PERCENT_MAX,
    CURRENT_SETTING_MIN,
    CURRENT_SETTING_MAX);
  return current_setting;
}

uint8_t Driver::currentSettingToPercent(uint8_t current_setting)
{
  uint8_t percent = map(current_setting,
    CURRENT_SETTING_MIN,
    CURRENT_SETTING_MAX,
    PERCENT_MIN,
    PERCENT_MAX);
  return percent;
}

uint8_t Driver::percentToHoldDelaySetting(uint8_t percent)
{
  uint8_t constrained_percent = constrain_(percent,
    PERCENT_MIN,
    PERCENT_MAX);
  uint8_t hold_delay_setting = map(constrained_percent,
    PERCENT_MIN,
    PERCENT_MAX,
    HOLD_DELAY_MIN,
    HOLD_DELAY_MAX);
  return hold_delay_setting;
}

uint8_t Driver::holdDelaySettingToPercent(uint8_t hold_delay_setting)
{
  uint8_t percent = map(hold_delay_setting,
    HOLD_DELAY_MIN,
    HOLD_DELAY_MAX,
    PERCENT_MIN,
    PERCENT_MAX);
  return percent;
}

void Driver::writeStoredGlobalConfig()
{
  write(ADDRESS_GCONF, global_config_.bytes);
}

uint32_t Driver::readGlobalConfigBytes()
{
  return read(ADDRESS_GCONF);
}

void Driver::writeStoredDriverCurrent()
{
  write(ADDRESS_IHOLD_IRUN, driver_current_.bytes);

  if (driver_current_.irun >= SEIMIN_UPPER_CURRENT_LIMIT)
  {
    cool_config_.seimin = SEIMIN_UPPER_SETTING;
  }
  else
  {
    cool_config_.seimin = SEIMIN_LOWER_SETTING;
  }
  if (cool_step_enabled_)
  {
    write(ADDRESS_COOLCONF, cool_config_.bytes);
  }
}

void Driver::writeStoredChopperConfig()
{
  write(ADDRESS_CHOPCONF, chopper_config_.bytes);
}

uint32_t Driver::readChopperConfigBytes()
{
  return read(ADDRESS_CHOPCONF);
}

void Driver::writeStoredPwmConfig()
{
  write(ADDRESS_PWMCONF, pwm_config_.bytes);
}

uint32_t Driver::readPwmConfigBytes()
{
  return read(ADDRESS_PWMCONF);
}

uint32_t Driver::constrain_(uint32_t value, uint32_t low, uint32_t high)
{
  return ((value)<(low)?(low):((value)>(high)?(high):(value)));
}
