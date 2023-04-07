/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/**
 * lcd/extui/wtgl_ui/wtgl_adapter.cpp
 * 
 * Adapter for MonoPrice Mini Delta V2 display
 */

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

struct eeprom_data_t
{
    uint8_t gohome = 0,
     showWelcome = 1,
     enablepowerloss = 0,
     enableselftest = 1;
} eeprom_data;

namespace ExtUI {
  void onStartup() {
    SERIAL_ECHOLNPGM("WTGL LCD starting...");
    
    // SD card init
    SET_OUTPUT(STM_SD_CS);
    SET_OUTPUT(STM_SD_BUSY);
    WRITE(STM_SD_CS, 0);
    WRITE(STM_SD_BUSY, 0);
    
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

    wtgl.Init(LCD_BAUDRATE); // baudrate is not used here as DMA is already initialized
    safe_delay(200);
    wtgl.Update();
    safe_delay(200);
    wtgl.nowtime = millis();
    wtgl.GotoBootMenu();
    safe_delay(200);

    thermalManager.auto_reporter.report_interval = (uint8_t) 10; // every 10 seconds

    SERIAL_ECHOLNPGM("WTGL LCD started!");
  }

  void onIdle() {
    wtgl.Update();
    wtgl.setPrintProgress(getProgress_percent());
    wtgl.setRemainingTime(getProgress_seconds_remaining());
  }

  void onPrinterKilled(FSTR_P const error, FSTR_P const component) {
    /*
    wtgl.EndStopError();
    wtgl.ErrorID(3);
    wtgl.ShowErrorMessage((const char *) error);
    //*/
  }
  void onMediaInserted() {}
  void onMediaError() {}
  void onMediaRemoved() {}
  void onPlayTone(const uint16_t frequency, const uint16_t duration) {}
  void onPrintTimerStarted() {
    if (card.isPrinting())
      wtgl.jobinfo.filesize = card.getFileSize();
    wt_machineStatus = WS_PRINTING;
    wtgl.GotoPrintingMenu();
  }
  void onPrintTimerPaused() {
    wt_machineStatus = WS_PAUSE;
  }
  void onPrintTimerStopped() {
    wt_machineStatus = WS_ABORT;
    wtgl.GotoMain();
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
    wtgl_ahit = 0;
    wtgl_bhit = 0;
    wtgl_chit = 0;
  }
  void onHomingDone() {
    wtgl_ahit = 1;
    wtgl_bhit = 1;
    wtgl_chit = 1;
  }
  void onPrintDone() {
    wt_machineStatus = WS_FINISH;
  }

  void onFactoryReset() {
    wtgl.wtvar_gohome = 0;
    wtgl.wtvar_showWelcome = 1;
    wtgl.wtvar_enablepowerloss = 0;
    wtgl.wtvar_enableselftest = 1;
  }

  void onStoreSettings(char *buff) {
    eeprom_data.gohome = wtgl.wtvar_gohome;
    eeprom_data.showWelcome = wtgl.wtvar_showWelcome;
    eeprom_data.enablepowerloss = wtgl.wtvar_enablepowerloss;
    eeprom_data.enableselftest = wtgl.wtvar_enableselftest;
    memcpy(buff, &eeprom_data, sizeof(eeprom_data));
  }

  void onLoadSettings(const char *buff) {
    memcpy(&eeprom_data, buff, sizeof(eeprom_data));

    // restore go home
    wtgl.wtvar_gohome = eeprom_data.gohome;
    if (wtgl.wtvar_gohome != 1)
      wtgl.wtvar_gohome = 0;

    // restore show welcome
    wtgl.wtvar_showWelcome = eeprom_data.showWelcome;
    if (wtgl.wtvar_showWelcome != 0)
      wtgl.wtvar_showWelcome = 1;

    // restore enable power loss
    wtgl.wtvar_enablepowerloss = eeprom_data.enablepowerloss;
    if (wtgl.wtvar_enablepowerloss > 1)
      wtgl.wtvar_enablepowerloss = 0;

    // restore enable self test
    wtgl.wtvar_enableselftest = eeprom_data.enableselftest;
    if (wtgl.wtvar_enableselftest != 0)
      wtgl.wtvar_enableselftest = 1;
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
