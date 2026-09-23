#include "MOS6532.hh"

MOS6532::MOS6532() { reset(); }

void MOS6532::reset() {
  ram.fill(0);          // Clear RAM
  io_ports.fill(0xFF);  // Clear I/O ports
  ddrs.fill(0);         // Clear Data Direction Registers

  timer_counter = 0;   // Reset timer counter
  timer_interval = 1;  // Reset timer interval

  timer_expired = false;  // Reset timer expired flag
}

void MOS6532::set_timer(uint8_t value, int interval) {
  timer_counter =
      (value + 1) *
      interval;  // Set the timer counter based on the value and interval
  timer_interval = interval;  // Set the timer interval (1, 8, 64, or 1024)

  timer_expired = false;  // Reset the timer expired flag
}

uint8_t MOS6532::get_timer_value() {
  if (timer_counter > 0) {
    return static_cast<uint8_t>((timer_counter - 1) / timer_interval);
  } else {
    return static_cast<uint8_t>(-(timer_counter + 1) & 0xFF);
  }
}

uint8_t MOS6532::read(uint16_t address) {
  if (address < 0x80) {
    return ram[address];  // Read from RAM
  }
  switch (address) {
    case 0x80:  // SWCHA
      return io_ports[0];
    case 0x82:  // SWCHB
      return io_ports[1];
    case 0x83:  // SWBCNT
      return ddrs[1];
    case 0x84:  // INTIM
      return get_timer_value();
    case 0x85:  // TIMINT
    {
      uint8_t status = (timer_expired ? 0x80 : 0x00);
      timer_expired = false;
      return status;
    }
    default:
      return 0;  // Invalid address, return 0
  }
}

void MOS6532::write(uint16_t address, uint8_t value) {
  if (address < 0x80) {
    ram[address] = value;  // Write to RAM
    return;
  }
  switch (address) {
    case 0x80:  // SWCHA
      io_ports[0] = (value & ddrs[0]) | (io_ports[0] & ~ddrs[0]);
      break;
    case 0x81:  // DDRA
      ddrs[0] = value;
      break;
    case 0x83:  // DDRB
      ddrs[1] = value;
      break;
    case 0x94:  // TIM1T
      set_timer(value, 1);
      break;
    case 0x95:  // TIM8T
      set_timer(value, 8);
      break;
    case 0x96:  // TIM64T
      set_timer(value, 64);
      break;
    case 0x97:  // T1024T
      set_timer(value, 1024);
      break;
    default:
      break;  // Invalid address, do nothing
  }
}

void MOS6532::update(int cpu_cycles) {
  if (timer_counter > 0) {
    timer_counter -= cpu_cycles;
    if (timer_counter <= 0) {
      timer_expired = true;

      timer_interval = 1;
      timer_counter = 0;
    }
  }
}

void MOS6532::setSwcha(uint8_t v) { io_ports[0] = v; }

void MOS6532::setSwchb(uint8_t v) { io_ports[1] = v; }
