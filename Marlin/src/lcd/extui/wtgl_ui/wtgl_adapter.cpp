/*********************
 * example.cpp *
 *********************/

/****************************************************************************
 *   Written By Marcio Teixeira 2018 - Aleph Objects, Inc.                  *
 *                                                                          *
 *   This program is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation, either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   To view a copy of the GNU General Public License, go to the following  *
 *   location: <https://www.gnu.org/licenses/>.                             *
 ****************************************************************************/

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(WTGL_LCD)

#include "../ui_api.h"
#include "../../wtgl/WTGL_Manager.h"
#include "../../wtlib/WTHardware.h"
#include "../../wtlib/WTCMD.h"
#include "../../wtgl/WTGL_Screen_Post.h"
#include "../../feature/powerloss.h"
#include "../../gcode/queue.h"
#include "../../module/temperature.h"
#include "../../module/endstops.h"

// To implement a new UI, complete the functions below and
// read or update Marlin's state using the methods in the
// ExtUI methods in "../ui_api.h"
//
// Although it may be possible to access other state
// variables from Marlin, using the API here possibly
// helps ensure future compatibility.

extern uint8_t wtgl_ahit, wtgl_bhit, wtgl_chit;

namespace ExtUI {
  void onStartup() {
    /* Initialize the display module here. The following
     * routines are available for access to the GPIO pins:
     *
     *   SET_OUTPUT(pin)
     *   SET_INPUT_PULLUP(pin)
     *   SET_INPUT(pin)
     *   WRITE(pin,value)
     *   READ(pin)
     */
    SERIAL_ECHOLNPGM("WTGL LCD started...");
    /*
    // init sd control
    pinMode(STM_SD_CS, OUTPUT);
    pinMode(STM_SD_BUSY, OUTPUT);
    digitalWrite(STM_SD_CS,LOW);
    digitalWrite(STM_SD_BUSY, LOW);

    recovery.enable(wtgl.wtvar_enablepowerloss);

    queue.enqueue_one_now("M203 Z300");
    if (wtgl.wtvar_gohome == 1)
    {
      queue.enqueue_one_now("G28 F1000");
      queue.enqueue_one_now("M18");
      #if ENABLED(POWER_LOSS_RECOVERY)
        card.removeJobRecoveryFile();
      #endif
    }

    wtgl.Init(LCD_BAUDRATE);
    safe_delay(200);
    wtgl.Update();
    safe_delay(200);
    wtgl.nowtime = millis();
    wtgl.GotoBootMenu();
    safe_delay(200);

    thermalManager.auto_reporter.report_interval = (uint8_t) 500;
    //*/
  }

  void onIdle() {
    /* 
    wtgl.Update();
    //*/
  }
  void onPrinterKilled(FSTR_P const error, FSTR_P const component) {
    /*
    wtgl.EndStopError();
    wtgl.ErrorID(3);
    wtgl.ShowErrorMessage((const char *) error);
    //*/
  }
  void onMediaInserted() {
    // TODO MPMDv2: maybe set SDIO pins here?
  }
  void onMediaError() {}
  void onMediaRemoved() {
    // TODO MPMDv2: maybe set SDIO pins here?
  }
  void onPlayTone(const uint16_t frequency, const uint16_t duration) {}
  void onPrintTimerStarted() {
    /*
    wt_machineStatus = WS_PRINTING;
    wtgl.GotoPrintingMenu();
    //*/
  }
  void onPrintTimerPaused() {
    /*
    wt_machineStatus = WS_PAUSE;
    //*/
  }
  void onPrintTimerStopped() {
    /*
    wt_machineStatus = WS_ABORT;
    //*/
  }
  void onFilamentRunout(const extruder_t extruder) {}
  void onUserConfirmRequired(const char * const msg) {
    // TODO: allow user confirm
  }
  void onStatusChanged(const char * const msg) {
    // TODO: handle status changes for MPMDv2

    /* MSG_LCD_PROBING_FAILED
    if (wt_machineStatus == WS_PRINTING)
      wtgl.ErrorID(4);
    else
      wtgl.ShowLog(STR_ERR_PROBING_FAILED);
    */

    /* MSG_ERR_MAXTEMP
    if (heater >= 0)
    {
      if (wtgl.isTesting())
      {
        wtgl.NozzleTempError();
      }
      else
      {
          wtgl.ErrorID(1);
      }
    }
    else
    {
        if (wtgl.isTesting())
        {
			    wtgl.BedTempError();
        }
		    else
        {
          wtgl.ErrorID(2);
        }
    }
   */

  /* MSG_ERR_MINTEMP
  if (heater >= 0)
  {
    if (wtgl.isTesting())
    {
      wtgl.NozzleTempError();
    }
    else
    {
      wtgl.ErrorID(0);
    }
  }
  else
  {
    if (wtgl.isTesting())
    {
      wtgl.BedTempError();
    }
    else
    {
      wtgl.ErrorID(2);
    }
  }
  */

  }

  void onHomingStart() {
    /*
    wtgl_ahit = 0;
    wtgl_bhit = 0;
    wtgl_chit = 0;
    //*/
  }
  void onHomingDone() {
    /*
    if (TEST(endstops.trigger_state(), X_MAX))
      wtgl_ahit = 1;
    if (TEST(endstops.trigger_state(), Y_MAX))
      wtgl_bhit = 1;
    if (TEST(endstops.trigger_state(), Z_MAX))
      wtgl_chit = 1;
    //*/
  }
  void onPrintDone() {
    /*
    wt_machineStatus = WS_FINISH;
    //*/
  }

  void onFactoryReset() {
    /*
    wtgl.wtvar_gohome = 0;
    wtgl.wtvar_showWelcome = 1;
    wtgl.wtvar_enablepowerloss = 0;
    wtgl.wtvar_enableselftest = 1;
    //*/
  }

  void onStoreSettings(char *buff) {
    // Called when saving to EEPROM (i.e. M500). If the ExtUI needs
    // permanent data to be stored, it can write up to eeprom_data_size bytes
    // into buff.

    // Example:
    //  static_assert(sizeof(myDataStruct) <= eeprom_data_size);
    //  memcpy(buff, &myDataStruct, sizeof(myDataStruct));

    /*

    // store go home
    memcpy(buff, &wtgl.wtvar_gohome, sizeof(wtgl.wtvar_gohome));

    // store show welcome
    memcpy(buff, &wtgl.wtvar_showWelcome, sizeof(wtgl.wtvar_showWelcome));

    // store enable power loss
    memcpy(buff, &wtgl.wtvar_enablepowerloss, sizeof(wtgl.wtvar_enablepowerloss));

    // store enable self test
    memcpy(buff, &wtgl.wtvar_enableselftest, sizeof(wtgl.wtvar_enableselftest));

    //*/
  }

  void onLoadSettings(const char *buff) {
    // Called while loading settings from EEPROM. If the ExtUI
    // needs to retrieve data, it should copy up to eeprom_data_size bytes
    // from buff

    // Example:
    //  static_assert(sizeof(myDataStruct) <= eeprom_data_size);
    //  memcpy(&myDataStruct, buff, sizeof(myDataStruct));

    /*

    // restore go home
    memcpy(&wtgl.wtvar_gohome, buff, sizeof(wtgl.wtvar_gohome));
    if (wtgl.wtvar_gohome != 1)
      wtgl.wtvar_gohome = 0;

    // restore show welcome
    memcpy(&wtgl.wtvar_showWelcome, buff, sizeof(wtgl.wtvar_showWelcome));
    if (wtgl.wtvar_showWelcome != 0)
      wtgl.wtvar_showWelcome = 1;

    // restore enable power loss
    memcpy(&wtgl.wtvar_enablepowerloss, buff, sizeof(wtgl.wtvar_enablepowerloss));
    if (wtgl.wtvar_enablepowerloss > 1)
      wtgl.wtvar_enablepowerloss = 0;

    // restore enable self test
    memcpy(&wtgl.wtvar_enableselftest, buff, sizeof(wtgl.wtvar_enableselftest));
    if (wtgl.wtvar_enableselftest != 0)
      wtgl.wtvar_enableselftest = 1;

    //*/
  }

  void onPostprocessSettings() {
    // Called after loading or resetting stored settings
  }

  void onSettingsStored(bool success) {
    // Called after the entire EEPROM has been written,
    // whether successful or not.
  }

  void onSettingsLoaded(bool success) {
    // Called after the entire EEPROM has been read,
    // whether successful or not.
  }

  #if HAS_MESH
    void onLevelingStart() {}
    void onLevelingDone() {}

    void onMeshUpdate(const int8_t xpos, const int8_t ypos, const_float_t zval) {
      // Called when any mesh points are updated
    }

    void onMeshUpdate(const int8_t xpos, const int8_t ypos, const probe_state_t state) {
      // Called to indicate a special condition
    }
  #endif

  #if ENABLED(POWER_LOSS_RECOVERY)
    void onPowerLossResume() {
      // Called on resume from power-loss
    }
  #endif

  #if HAS_PID_HEATING
    void onPidTuning(const result_t rst) {
      // Called for temperature PID tuning result
      switch (rst) {
        case PID_STARTED:          break;
        case PID_BAD_EXTRUDER_NUM: break;
        case PID_TEMP_TOO_HIGH:    break;
        case PID_TUNING_TIMEOUT:   break;
        case PID_DONE:             break;
      }
    }
  #endif

  void onSteppersDisabled() {}
  void onSteppersEnabled()  {}
}

#endif // ENABLED(WTGL_LCD)
