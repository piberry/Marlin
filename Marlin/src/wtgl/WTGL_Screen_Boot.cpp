/**
* Copyright (C) 2021 WEEDO3D Perron
*/
#include "../MarlinCore.h"
#include "../module/temperature.h"
#include "../../sd/cardreader.h"
#include "../module/planner.h"
#include "../feature/powerloss.h"
#include "../module/settings.h"
#include "WTGL_Screen_Boot.h"
#include "WTGL_Serial.h"
#include "WTGL_Manager.h"

void WTGL_Screen_Boot::Init()
{
	holdontime = wtgl.getcurrenttime();

	checkCurrentScreen();
}

void WTGL_Screen_Boot::Update()
{
	if (holdontime > lastchecktime)
		checkCurrentScreen();
}

void WTGL_Screen_Boot::checkCurrentScreen(void)
{
    wtgl.currentID = 0xFF;
    gserial.SendCmd(REG_CURRENT_ID);
}

void WTGL_Screen_Boot::KeyProcess(uint16_t addr, uint8_t *data, uint8_t data_length)
{
	if (addr == ADDR_GLOBAL_CURRENT)
	{
		if (data[0] != 0x0 && wtgl.wtvar_gohome == 0)
		{ 
			wtgl.ResetScreen();
			TERN_(WTGL_WAIT_FOR_ANIMATION, safe_delay(4000));
		}
        else
        {   
            TERN_(WTGL_WAIT_FOR_ANIMATION, safe_delay(3000));
        }
        safe_delay(1000);

		if (wtgl.wtvar_gohome == 1)
        {  
            wtgl.wtvar_gohome = 0;
            (void)settings.save();
        }
        else if (wtgl.wtvar_showWelcome)
        {  
            wtgl.GotoWizardMenu();
            return;
        }
        else if (wtgl.wtvar_enableselftest)
        {  
            wtgl.GotoSelfTest();
            return;
        }
        
        uint16_t temp_nozzle0 = thermalManager.degHotend(0);
        uint16_t temp_bed = thermalManager.degBed();

        if (temp_nozzle0 >= 0)
            gserial.SendInt16(REG_NOZZLE0_CURRENT, temp_nozzle0);
        else
            gserial.SendInt16(REG_NOZZLE0_CURRENT, 0);

        if (temp_bed >= 0)
            gserial.SendInt16(REG_BED_CURRENT, temp_bed);
        else
            gserial.SendInt16(REG_BED_CURRENT, 0);

		gserial.SendString(REG_DEVICE_VERSION, SHORT_BUILD_VERSION);
        wtgl.GotoMain();
	}
}