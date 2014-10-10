/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

// STL headers
// C headers
#include <avr/pgmspace.h>
// Framework headers
// Library headers
#include <SPI.h>
// Project headers
// This component's header
#include <VS1053.h>

const uint8_t vs1053_chunk_size = 32;

#undef PROGMEM
#define PROGMEM __attribute__ ((section (".progmem.data"))) 
#undef PSTR 
#define PSTR(s) (__extension__({static char __c[] PROGMEM = (s); &__c[0];}))

/****************************************************************************/

// SCI Registers

const uint8_t SCI_MODE = 0x0;
const uint8_t SCI_STATUS = 0x1;
const uint8_t SCI_BASS = 0x2;
const uint8_t SCI_CLOCKF = 0x3;
const uint8_t SCI_DECODE_TIME = 0x4;
const uint8_t SCI_AUDATA = 0x5;
const uint8_t SCI_WRAM = 0x6;
const uint8_t SCI_WRAMADDR = 0x7;
const uint8_t SCI_HDAT0 = 0x8;
const uint8_t SCI_HDAT1 = 0x9;
const uint8_t SCI_AIADDR = 0xa;
const uint8_t SCI_VOL = 0xb;
const uint8_t SCI_AICTRL0 = 0xc;
const uint8_t SCI_AICTRL1 = 0xd;
const uint8_t SCI_AICTRL2 = 0xe;
const uint8_t SCI_AICTRL3 = 0xf;
const uint8_t SCI_num_registers = 0xf;

// SCI_MODE bits

const uint8_t SM_DIFF = 0;
const uint8_t SM_LAYER12 = 1;
const uint8_t SM_RESET = 2;
const uint8_t SM_OUTOFWAV = 3;
const uint8_t SM_EARSPEAKER_LO = 4;
const uint8_t SM_TESTS = 5;
const uint8_t SM_STREAM = 6;
const uint8_t SM_EARSPEAKER_HI = 7;
const uint8_t SM_DACT = 8;
const uint8_t SM_SDIORD = 9;
const uint8_t SM_SDISHARE = 10;
const uint8_t SM_SDINEW = 11;
const uint8_t SM_ADPCM = 12;
const uint8_t SM_ADCPM_HP = 13;
const uint8_t SM_LINE_IN = 14;
const uint8_t SM_CLK_RANGE = 15;

// Register names

char reg_name_MODE[] PROGMEM = "MODE";
char reg_name_STATUS[] PROGMEM  = "STATUS";
char reg_name_BASS[] PROGMEM  = "BASS";
char reg_name_CLOCKF[] PROGMEM  = "CLOCKF";
char reg_name_DECODE_TIME[] PROGMEM  = "DECODE_TIME";
char reg_name_AUDATA[] PROGMEM  = "AUDATA";
char reg_name_WRAM[] PROGMEM  = "WRAM";
char reg_name_WRAMADDR[] PROGMEM  = "WRAMADDR";
char reg_name_HDAT0[] PROGMEM  = "HDAT0";
char reg_name_HDAT1[] PROGMEM  = "HDAT1";
char reg_name_AIADDR[] PROGMEM  = "AIADDR";
char reg_name_VOL[] PROGMEM  = "VOL";
char reg_name_AICTRL0[] PROGMEM  = "AICTRL0";
char reg_name_AICTRL1[] PROGMEM  = "AICTRL1";
char reg_name_AICTRL2[] PROGMEM  = "AICTRL2";
char reg_name_AICTRL3[] PROGMEM  = "AICTRL3";

static PGM_P register_names[] PROGMEM =
{
  reg_name_MODE,
  reg_name_STATUS,
  reg_name_BASS,
  reg_name_CLOCKF,
  reg_name_DECODE_TIME,
  reg_name_AUDATA,
  reg_name_WRAM,
  reg_name_WRAMADDR,
  reg_name_HDAT0,
  reg_name_HDAT1,
  reg_name_AIADDR,
  reg_name_VOL,
  reg_name_AICTRL0,
  reg_name_AICTRL1,
  reg_name_AICTRL2,
  reg_name_AICTRL3,
};

/****************************************************************************/

/**
 * Spi saver
 *
 * Easy way to pop the SPI config registers onto the stack and pop
 * them off again
 */
struct spi_saver_t
{
  uint8_t saved_SPCR;
  uint8_t saved_SPSR;

  spi_saver_t(void)
  {
    saved_SPCR = SPCR;
    saved_SPSR = SPSR;
  }
  ~spi_saver_t()
  {
    SPCR = saved_SPCR;
    SPSR = saved_SPSR;
  }
};

/****************************************************************************/

uint16_t VS1053::read_register(uint8_t _reg) const
{
  uint16_t result;
  control_mode_on();
  delayMicroseconds(1); // tXCSS
  SPI.transfer(B11); // Read operation
  SPI.transfer(_reg); // Which register
  result = SPI.transfer(0xff) << 8; // read high byte
  result |= SPI.transfer(0xff); // read low byte
  delayMicroseconds(1); // tXCSH
  await_data_request();
  control_mode_off();
  return result;
}

/****************************************************************************/

void VS1053::write_register(uint8_t _reg,uint16_t _value) const
{
  control_mode_on();
  delayMicroseconds(1); // tXCSS
  SPI.transfer(B10); // Write operation
  SPI.transfer(_reg); // Which register
  SPI.transfer(_value >> 8); // Send hi byte
  SPI.transfer(_value & 0xff); // Send lo byte
  delayMicroseconds(1); // tXCSH
  await_data_request();
  control_mode_off();
}

/****************************************************************************/

void VS1053::sdi_send_buffer(const uint8_t* data, size_t len)
{
  data_mode_on();
  while ( len )
  {
    await_data_request();
    delayMicroseconds(3);

    size_t chunk_length = min(len,vs1053_chunk_size);
    len -= chunk_length;
    while ( chunk_length-- )
      SPI.transfer(*data++);
  }
  data_mode_off();
}

/****************************************************************************/

void VS1053::sdi_send_zeroes(size_t len)
{
  data_mode_on();
  while ( len )
  {
    await_data_request();

    size_t chunk_length = min(len,vs1053_chunk_size);
    len -= chunk_length;
    while ( chunk_length-- )
      SPI.transfer(0);
  }
  data_mode_off();
}

/****************************************************************************/

VS1053::VS1053( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t _reset_pin):
  cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin), reset_pin(_reset_pin)
{
}

/****************************************************************************/

void VS1053::begin(void)
{
  spi_saver_t spi_saver;

  // Keep the chip in reset until we are ready
  pinMode(reset_pin,OUTPUT);
  digitalWrite(reset_pin,LOW);

  // The SCI and SDI will start deselected
  pinMode(cs_pin,OUTPUT);
  digitalWrite(cs_pin,HIGH);
  pinMode(dcs_pin,OUTPUT);
  digitalWrite(dcs_pin,HIGH);

  // DREQ is an input
  pinMode(dreq_pin,INPUT);

  // Boot VS1053D
  printf_P(PSTR("Booting VS1053...\r\n"));

  //Mp3PutInReset();
  //Done above

  //Delay(1);
  delay(1);

  //InitSPI();
  SPI.setClockDivider(SPI_CLOCK_DIV64); // Slow!

  /* Un-reset MP3 chip */
  //Mp3DeselectControl();
  //Mp3DeselectData();
  //These are done above.

  //Mp3ReleaseFromReset();
  digitalWrite(reset_pin,HIGH);
  
  //Mp3SetVolume(0xff,0xff); //Declick: Immediately switch analog off
  write_register(SCI_VOL,0xffff); // VOL

  /* Declick: Slow sample rate for slow analog part startup */
  //Mp3WriteRegister(SPI_AUDATA, 0, 10); /* 10 Hz */
  write_register(SCI_AUDATA,10);

  //Delay(100);
  delay(100);

  /* Switch on the analog parts */
  //Mp3SetVolume(0xfe,0xfe);
  write_register(SCI_VOL,0xfefe); // VOL
  
  printf_P(PSTR("VS1053 still booting\r\n"));

  //Mp3WriteRegister (SPI_AUDATA, 31, 64); /* 8kHz */
  write_register(SCI_AUDATA,44101); // 44.1kHz stereo

  //Mp3SetVolume(20,20); // Set initial volume (20 = -10dB)
  write_register(SCI_VOL,0x2020); // VOL
  
  //Mp3SoftReset();
  write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET));
  delay(1);
  await_data_request();
  write_register(SCI_CLOCKF,0xB800); // Experimenting with higher clock settings
  delay(1);
  await_data_request();

  //SPISetFastClock(); // Now you can set high speed SPI clock
  SPI.setClockDivider(SPI_CLOCK_DIV4); // Fastest available

  printf_P(PSTR("VS1053 Set\r\n"));
  printDetails();
  printf_P(PSTR("VS1053 OK\r\n"));

  // Having set up our SPI state just the way we like it, save it for next time
  save_our_spi();
}

/****************************************************************************/

void VS1053::setVolume(uint8_t vol) const
{
  uint16_t value = vol;
  value <<= 8;
  value |= vol;

  write_register(SCI_VOL,value); // VOL
}

/****************************************************************************/

void VS1053::startSong(void)
{
  spi_saver_t spi_saver;
  set_our_spi();
  sdi_send_zeroes(10);
}

/****************************************************************************/

void VS1053::playChunk(const uint8_t* data, size_t len)
{
  spi_saver_t spi_saver;
  set_our_spi();
  sdi_send_buffer(data,len);
}

/****************************************************************************/

void VS1053::stopSong(void)
{
  spi_saver_t spi_saver;
  set_our_spi();
  sdi_send_zeroes(2048);
}

/****************************************************************************/

void VS1053::print_byte_register(uint8_t reg) const
{
  const char *name = reinterpret_cast<const char*>(pgm_read_word( register_names + reg ));
  char extra_tab = strlen_P(name) < 5 ? '\t' : 0;
  printf_P(PSTR("%02x %S\t%c = 0x%02x\r\n"),reg,name,extra_tab,read_register(reg));
}

/****************************************************************************/

void VS1053::printDetails(void) const
{
  spi_saver_t spi_saver;
  printf_P(PSTR("VS1053 Configuration:\r\n"));
  int i = 0;
  while ( i <= SCI_num_registers )
    print_byte_register(i++);
}

/****************************************************************************/

void VS1053::loadUserCode(const uint16_t* buf, size_t len) const
{
  while (len)
  {
    uint16_t addr = pgm_read_word(buf++); len--;
    uint16_t n = pgm_read_word(buf++); len--;
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      uint16_t val = pgm_read_word(buf++); len--;
      while (n--) {
	printf_P(PSTR("W %02x: %04x\r\n"),addr,val);
        write_register(addr, val);
      }
    } else {           /* Copy run, copy n samples */
      while (n--) {
	uint16_t val = pgm_read_word(buf++); len--;
	printf_P(PSTR("W %02x: %04x\r\n"),addr,val);
        write_register(addr, val);
      }
    }
  }
}

/****************************************************************************/

// vim:cin:ai:sts=2 sw=2 ft=cpp
