/**
* Copyright (C) 2021 WEEDO3D Perron
*/

#include "WTCMD.h"
#include "../gcode/parser.h"
#include "../HAL/STM32F1/HAL.h"
#include "../sd/cardreader.h"
#include "../module/printcounter.h"
#include "../libs/duration_t.h"
#include "../module/planner.h"
#include "../module/temperature.h"
#include "../module/settings.h"
#include "../MarlinCore.h"
#include "../feature/host_actions.h"
#include "../wtgl/WTGL_Manager.h"
#include "../wtgl/WTGL_Serial.h"
#include "../module/probe.h"
#include "../module/stepper.h"
#include "WTUtilty.h"
#include "../../gcode/gcode.h"
#include "../libs/nozzle.h"
#include "../feature/pause.h"

#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../../feature/powerloss.h"
#endif

#define SD_CONFIG_FILE		"config.sav"

WT_STATUS wt_machineStatus = WS_IDLE;

WT_MAIN_ACTION wt_mainloop_action = WT_MAIN_ACTION::WMA_IDLE;

char parsedString[30];

static void wt_pause_print();
static void wt_resume_print();

void GetMachineStatus()
{
	/* it seems this is not needed...
	SERIAL_ECHOPGM("MStatus:");
	SERIAL_ECHO(wt_machineStatus);

	SERIAL_ECHOPGM(" MFile:");
	#ifdef SDSUPPORT
	card.printSelectedFilename();
	#endif

	char buffer[21];
	duration_t elapsed = print_job_timer.duration();
	elapsed.toString(buffer);

	SERIAL_ECHOPGM(" MTime:");
	SERIAL_ECHO(buffer);

	SERIAL_ECHOPGM(" MPercent:");
	#ifdef SDSUPPORT
	card.report_status();
	#endif

	SERIAL_EOL();
	//*/

	wtgl.jobinfo.filepos = card.getIndex();
}

void wt_sdcard_stop()
{
	card.endFilePrintNow();
	print_job_timer.stop();

	#if ENABLED(POWER_LOSS_RECOVERY)
      recovery.purge();
    #endif

	wait_for_heatup = false;

	wtgl.wtvar_gohome = 1;
	(void)settings.save();

	wt_restart();
}

void wt_sdcard_pause()
{
	card.pauseSDPrint();
	print_job_timer.pause();

	#if ENABLED(POWER_LOSS_RECOVERY)
      if (recovery.enabled) recovery.save(true);
    #endif

	#if ENABLED(PARK_HEAD_ON_PAUSE)
	 queue.enqueue_now_P(PSTR("M125"));
	#endif

	wt_machineStatus = WS_PAUSE;
	wt_mainloop_action = WMA_PAUSE;	
}

void wt_sdcard_resume()
{
	wt_resume_print();
	
	wtgl.GotoPrintingMenu();

	wt_machineStatus = WS_PRINTING;

  	#ifdef ACTION_ON_RESUMED
    hostui.resumed();
  	#endif
}

void wt_spark_begin()
{
	wtgl.wt_onlineprinting = OCTOPRINT_PRINTING;
	gserial.SendByte(REG_OCTOPRINT_STATE,  (uint8_t)wtgl.wt_onlineprinting);
	print_job_timer.start();

	#ifdef WTGL_LCD
	wtgl.GotoPrintingMenu();
	#endif

	wt_machineStatus = WS_PRINTING;
}

void wt_spark_end()
{
	wt_machineStatus = WS_FINISH;
	wtgl.wt_onlineprinting = OCTOPRINT_IDLE;
	print_job_timer.stop();
}

void wt_spark_cancel()
{
	wt_sdcard_stop();
}

void wt_spark_pause()
{
	print_job_timer.pause();

	wt_machineStatus = WS_PAUSE;
	wtgl.wt_onlineprinting = OCTOPRINT_PAUSED;

	#ifdef WTGL_LCD
	wtgl.GotoPrintingMenu();
	#endif

	wt_mainloop_action = WMA_PAUSE;	
}

void wt_spark_resume()
{
	wt_resume_print();

	wt_machineStatus = WS_PRINTING;
	wtgl.wt_onlineprinting = OCTOPRINT_PRINTING;

	#ifdef WTGL_LCD
	wtgl.GotoPrintingMenu();
	#endif
}

static bool write_line(char * const buf)
{
  char* begin = buf;
  char* end = buf + strlen(buf) - 1;

  end[1] = '\r';
  end[2] = '\n';
  end[3] = '\0';

  return card.write(begin, strlen(buf) + 2) >= 0;
}

static bool openNewFileWrite(char * const path) 
{
  if (!card.isMounted()) return false;

  if (card.fileExists(path))
	card.removeFile(path);

  card.openFileWrite(path);
  return true;
}

static bool openSavFileRead(char * const path) 
{
  if (!card.isMounted()) return false;

  card.openFileRead(path);
  return true;
}

void wt_save_config()
{
	if (!openNewFileWrite((char*)SD_CONFIG_FILE))
	{
		SERIAL_ECHOLNPGM("config file open fail!");

		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 1);
		#endif
		return;
	}

	char buffer[100];

	ZERO(buffer);
	sprintf(buffer, ";%s config data begin", MACHINE_NAME);
	if (!write_line(buffer))
	{
		SERIAL_ECHOLNPGM("Write to config save file fail.");
		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 2);
		#endif
		goto END;
	}

	ZERO(buffer);
	sprintf(buffer, "M665 L%.2f R%.2f H%.2f X%.2f Y%.2f Z%.2f",
					 LINEAR_UNIT(delta_diagonal_rod),
					 LINEAR_UNIT(delta_radius),
					 LINEAR_UNIT(delta_height),
					 LINEAR_UNIT(delta_tower_angle_trim.a),
					 LINEAR_UNIT(delta_tower_angle_trim.b),
					 LINEAR_UNIT(delta_tower_angle_trim.c)); // TODO: add others
	if (!write_line(buffer))
	{
		SERIAL_ECHOLNPGM("Write to config save file fail.");
		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 2);
		#endif
		goto END;
	}

	ZERO(buffer);
	sprintf(buffer, "M666 X%.2f Y%.2f Z%.2f",
					 LINEAR_UNIT(delta_endstop_adj.a),
					 LINEAR_UNIT(delta_endstop_adj.b),
					 LINEAR_UNIT(delta_endstop_adj.c));
	if (!write_line(buffer))
	{
		SERIAL_ECHOLNPGM("Write to config save file fail.");
		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 2);
		#endif
		goto END;
	}

	ZERO(buffer);
	sprintf(buffer, "M851 X%.2f Y%.2f Z%.2f",
					 LINEAR_UNIT(probe.offset_xy.x),
					 LINEAR_UNIT(probe.offset_xy.y),
					 probe.offset.z);
	if (!write_line(buffer))
	{
		SERIAL_ECHOLNPGM("Write to config save file fail.");
		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 2);
		#endif
		goto END;
	}

	ZERO(buffer);
	sprintf(buffer, "M907 X%ld Y%ld Z%ld E%ld",
					 stepper.motor_current_setting[0],
					 stepper.motor_current_setting[0], // same as X
					 stepper.motor_current_setting[1],
					 stepper.motor_current_setting[2]);
	if (!write_line(buffer))
	{
		SERIAL_ECHOLNPGM("Write to config save file fail.");
		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 2);
		#endif
		goto END;
	}

	ZERO(buffer);
	sprintf(buffer, ";%s config data end", MACHINE_NAME);
	if (!write_line(buffer))
	{
		SERIAL_ECHOLNPGM("Write to config save file fail.");
		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 2);
		#endif
		goto END;
	}

	SERIAL_ECHOLNPGM("Parameters saved successfully.");
	#ifdef WTGL_LCD
	gserial.SendByte(REG_CONFIG_SAVE_MSG, 3);
	#endif


END:
	card.closefile();
}

void wt_load_config()
{
	if (!openSavFileRead((char*)SD_CONFIG_FILE))
	{
		SERIAL_ECHOLNPGM("config file open fail!");
		#ifdef WTGL_LCD
		gserial.SendByte(REG_CONFIG_SAVE_MSG, 1);
		#endif
		return;
	}

	uint8_t sd_count = 0;
	bool card_eof = card.eof();
	bool sd_comment_mode = false;
	uint8_t linecount = 0;
	char buffer[100];

	ZERO(buffer);
	while (!card_eof && linecount < 10)
	{
		const int16_t n = card.get();
		char sd_char = (char)n;
		card_eof = card.eof();

		if (card_eof || n == -1 || sd_char == '\r')
		{	
			buffer[sd_count] = '\0';

			if (!sd_comment_mode)
			{
				queue.enqueue_one_now(buffer);
			}

			ZERO(buffer);

			sd_count = 0; // clear sd line buffer
			linecount++;
			sd_comment_mode = false;
		}
		else if (sd_count >= sizeof(buffer) - 1) 
		{	

		}
		else 
		{	
			if (sd_char == ';')
				sd_comment_mode = true;
			else if (sd_char != '\n')
				buffer[sd_count++] = sd_char;
		}
	}

	queue.enqueue_one_now("M500");

	SERIAL_ECHOLNPGM("Parameter recovery succeeded.");
	#ifdef WTGL_LCD
	gserial.SendByte(REG_CONFIG_SAVE_MSG, 4);
	#endif

	card.closefile();
}

void wt_reset_param(void)
{
	wtgl.wtvar_gohome = 0;
	wtgl.wtvar_showWelcome = 1;
	wtgl.wtvar_enablepowerloss = 0;
	wtgl.wtvar_enableselftest = 1;
	(void)settings.save();

	safe_delay(200);
	wt_restart();
}

// W Command Process
void WTCMD_Process(const uint16_t codenum)
{
	switch (codenum)
	{
	case 1:		
		wt_spark_begin();
		break;

	case 2:		
		wt_spark_end();
		break;

	case 3:		
		wt_spark_cancel();
		break;

	case 4:		
		wt_spark_pause();
		break;

	case 5:		
		wt_spark_resume();
		break;

	case 6:		
		wtgl.wt_onlineprinting = OCTOPRINT_IDLE;
		break;

	case 7:		
		wtgl.wt_onlineprinting = OCTOPRINT_LOST;
		break;

	case 201:	
		GetMachineStatus();
		break;

	case 203:	
		wt_restart();
		while (1);
		break;

	case 209:		
		wt_sdcard_stop();
		while (1);
		break;

	case 216:		
		wt_save_config();
		break;

	case 217:		
		wt_load_config();
		break;

	case 230:		
		wt_reset_param();
		break;

	case 300:		
		// not implemented
		// gserial.SendString(REG_WIFI_SSID, parser.string_arg);
		break;

	case 301:
		// not implemented	
		// gserial.SendString(REG_WIFI_PWD, parser.string_arg);
		break;

	case 302:		
		// not implemented
		gserial.SendCmd(REG_WIFI_JOIN);
		break;
	}

}

// restart
void wt_restart()
{
	nvic_sys_reset();
};


static void wt_pause_print()
{
	pause_print(PAUSE_PARK_RETRACT_LENGTH, NOZZLE_PARK_POINT);
}

static void wt_resume_print()
{
	resume_print();
}

// main loop action
void wt_loopaction(void)
{
	if (wt_mainloop_action == WMA_IDLE) return;

	if (wt_mainloop_action == WMA_PAUSE)
	{
		if (queue.has_commands_queued()) return;
		
		wt_pause_print();

		#ifdef ACTION_ON_PAUSED
		hostui.paused();
		#endif

		wt_mainloop_action = WMA_IDLE;
	}
	else if (wt_mainloop_action == WMA_RESUME)
	{

	}
}

// load sd card
void wt_load_sd(void)
{
    if ((uint8_t)IS_SD_INSERTED())
    {
        digitalWrite(STM_SD_BUSY, HIGH);
        card.mount();
        SERIAL_ECHOLNPGM("mount sd");
    }
}

// unload sd card
void wt_unload_sd(void)
{
    if ((uint8_t)IS_SD_INSERTED())
    {
        card.release();
        safe_delay(100);
        digitalWrite(STM_SD_BUSY, LOW);
        SERIAL_ECHOLNPGM("unmount sd");

    }
}

void wt_send_queue_length(void)
{
    gserial.SendByte(REG_QUEUE_LENGTH, queue.ring_buffer.length);
}

void wt_move_axis(const uint8_t axis, const float distance, const float fr_mm_s)
{
  const feedRate_t real_fr_mm_s = fr_mm_s ?: homing_feedrate((AxisEnum)axis);

    abce_pos_t target = planner.get_axis_positions_mm();
    target[axis] = 0;
    planner.set_machine_position_mm(target);
    target[axis] = distance;

    // Set delta/cartesian axes directly
    planner.buffer_line(target, real_fr_mm_s, 0);


  planner.synchronize();
}