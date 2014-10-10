/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#ifndef __VS1053_H__
#define __VS1053_H__

// STL headers
// C headers
// Framework headers
#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif
// Library headers
// Project headers
#include "rtmidistart_plg.h"

/**
 * Driver for VS1053 - Ogg Vorbis / MP3 / AAC / WMA / FLAC / MIDI Audio Codec Chip
 *
 * See http://www.vlsi.fi/en/products/vs1053.html
 */

class VS1053
{
private:
  uint8_t cs_pin; /**< Pin where CS line is connected */
  uint8_t dcs_pin; /**< Pin where DCS line is connected */
  uint8_t dreq_pin; /**< Pin where DREQ line is connected */
  uint8_t reset_pin; /**< Pin where RESET line is connected */
  uint8_t my_SPCR; /**< Value of the SPCR register how we like it. */
  uint8_t my_SPSR; /**< Value of the SPSR register how we like it. */
protected:
  inline void await_data_request(void) const
  {
    while ( !digitalRead(dreq_pin) );
  }

  inline void control_mode_on(void) const
  {
    digitalWrite(dcs_pin,HIGH);
    digitalWrite(cs_pin,LOW);
  }

  inline void control_mode_off(void) const
  {
    digitalWrite(cs_pin,HIGH);
  }

  inline void data_mode_on(void) const
  {
    digitalWrite(cs_pin,HIGH);
    digitalWrite(dcs_pin,LOW);
  }

  inline void data_mode_off(void) const
  {
    digitalWrite(dcs_pin,HIGH);
  }
  inline void save_our_spi(void)
  {
    my_SPSR = SPSR;
    my_SPCR = SPCR;
  }
  inline void set_our_spi(void)
  {
    SPSR = my_SPSR;
    SPCR = my_SPCR;
  }
  uint16_t read_register(uint8_t _reg) const;
  void write_register(uint8_t _reg,uint16_t _value) const;
  void sdi_send_buffer(const uint8_t* data,size_t len);
  void sdi_send_zeroes(size_t length);
  void print_byte_register(uint8_t reg) const;
  
  /**
   * Load a user code plugin
   *
   * @param buf Location of memory (in PROGMEM) where the code is
   * @param len Number of words to load
   */
  void loadUserCode(const uint16_t* buf, size_t len) const;

public:
  /**
   * Constructor
   *
   * Only sets pin values.  Doesn't do touch the chip.  Be sure to call begin()!
   */
  VS1053( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t _reset_pin);

  /**
   * Begin operation
   *
   * Sets pins correctly, and prepares SPI bus.
   */
  void begin(void);

  /**
   * Prepare to start playing
   *
   * Call this each time a new song starts.
   */
  void startSong(void);

  /**
   * Play a chunk of data.  Copies the data to the chip.  Blocks until complete.
   *
   * @param data Pointer to where the data lives
   * @param len How many bytes of data to play
   */
  void playChunk(const uint8_t* data, size_t len);

  /**
   * Finish playing a song.
   *
   * Call this after the last playChunk call.
   */
  void stopSong(void);

  /**
   * Print configuration details
   *
   * Dumps all registers to stdout.  Be sure to have stdout configured first
   * (see fdevopen() in avr/io.h).
   */
  void printDetails(void) const;

  /**
   * Set the player volume
   *
   * @param vol Volume level from 0-255, lower is louder.
   */
  void setVolume(uint8_t vol) const;

  /**
   * Play real-time MIDI.  Useful for using the VS1053 to make an
   * instrument.  Note that this implementation uses the SDI for
   * rtmidi commands.
   */
  class RtMidi
  {
  private:
    VS1053& player;
    uint8_t buffer[6];
  protected:
    /**
     * Write a single MIDI command out via SDI
     *
     * @param a First value (command byte)
     * @param b Second value (operand byte)
     * @param c Third value (optional 2nd operand)
     */
    void write(uint8_t a, uint8_t b, uint8_t c = 0)
    {
      buffer[1] = a;
      buffer[3] = b;
      buffer[5] = c;
      player.playChunk(buffer,sizeof(buffer));
    }
  public:
    /**
     * Construct from a VS1053 player
     *
     * @param _player Player which will play the MIDI notes
     */
    RtMidi(VS1053& _player): player(_player) 
    {
      memset(buffer,0,6);
    }
    /**
     * Enable the real-time MIDI mode.  Only call this when you now want
     * to send rtmidi, not raw music data
     */
    void begin(void)
    {
      player.loadUserCode(rtmidi_plugin,RTMIDI_PLUGIN_SIZE);
    }
    /**
     * Begin playing a note
     *
     * @param channel Which channel to play on
     * @param note Which note to play
     * @param volume How loud
     */
    void noteOn(uint8_t channel, uint8_t note, uint8_t volume)
    {
      write(channel | 0x90, note, volume);
    }
    /**
     * Stop playing a note
     *
     * @param channel Which channel to afffect 
     * @param note Which note to stop
     */
    void noteOff(uint8_t channel, uint8_t note)
    {
      write(channel | 0x80, note, 0x45);
    }
    /**
     * Choose the drums instrument
     *
     * @param channel Which channel to play the drums on
     */
    void selectDrums(uint8_t channel)
    {
      write(0xB0 | channel, 0, 0x78);
      write(0xC0 | channel ,30);
    }
  };
};

#endif // __VS1053_H__
// vim:cin:ai:sts=2 sw=2 ft=cpp
