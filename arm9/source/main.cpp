/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>

#include <vector>

#include "nds_loader_arm9.h"
#include "file_browse.h"
#include "inifile.h"
#include "menu.h"

char tmpdatapath[]="sd:/kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk";
std::string datapath;
std::string ndsPath;
std::string savePath;
std::string gamebootstrap;
std::string bootstrappath;
std::string bootstrapversion;

PrintConsole upperScreen;
PrintConsole lowerScreen;

void displayInit() {
	lowerScreen = *consoleDemoInit();
	videoSetMode(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	consoleInit(&upperScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
}

std::string GetRomName(const std::string& file) {
	auto pos1 = file.find_last_of("/") + 1;
	auto pos2 = file.find_last_of(".");
	return file.substr(pos1, pos2-pos1);
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

void SetTargetBootstrap() {
	std::string filename;
	std::vector<std::string> extensionList={".nds"};
	
	consoleSelect(&upperScreen);
	consoleClear();
	iprintf("Select the target bootstrap file");
	iprintf("to be used");
	consoleSelect(&lowerScreen);
	filename = browseForFile(extensionList);
	
	size_t found  = filename.find_last_of("/");
	
	bootstrappath=filename.substr(0,found+1);
	bootstrapversion=filename.substr(found+1);
	CIniFile configini;
	configini.LoadIniFile((datapath + "config.ini").c_str());
	configini.SetString( "NDS-FORWARDER", "BOOTSTRAP_PATH", bootstrappath.c_str());
	configini.SetString( "NDS-FORWARDER", "BOOTSTRAP_VERSION", bootstrapversion.c_str());
	configini.SaveIniFile((datapath + "config.ini").c_str());
}

void SetTargetRom() {
	std::vector<std::string> extensionList={".nds"};
	consoleSelect(&upperScreen);
	consoleClear();
	iprintf("Select the target rom");
	consoleSelect(&lowerScreen);
	chdir("/");
	ndsPath = browseForFile(extensionList);
	CIniFile bootstrap_template;
	bootstrap_template.LoadIniFile((bootstrappath+"nds-bootstrap.ini").c_str());
	bootstrap_template.SetString( "NDS-BOOTSTRAP", "NDS_PATH", ndsPath.c_str());
	savePath = ReplaceAll(ndsPath, ".nds", ".sav");
	bootstrap_template.SetString( "NDS-BOOTSTRAP", "SAV_PATH", savePath.c_str());
	bootstrap_template.SaveIniFile(gamebootstrap.c_str());
	bootstrap_template.SaveIniFile((bootstrappath+"nds-bootstrap.ini").c_str());
}

void SetSavePath() {
	consoleSelect(&upperScreen);
	consoleClear();
	iprintf("Select the folder where the save will be stored\n");
	iprintf("Press X once you are in the desired folder");
	consoleSelect(&lowerScreen);
	chdir("/");
	savePath = browseForFolder() + "/" + GetRomName(ndsPath) + ".sav";
	CIniFile bootstrap_template;
	bootstrap_template.LoadIniFile(gamebootstrap.c_str());
	bootstrap_template.SetString( "NDS-BOOTSTRAP", "SAV_PATH", savePath.c_str());
	bootstrap_template.SaveIniFile(gamebootstrap.c_str());
	bootstrap_template.SaveIniFile((bootstrappath+"nds-bootstrap.ini").c_str());
}

void LoadSettings(void) {
	CIniFile configini;
	if(configini.LoadIniFile((datapath + "config.ini").c_str())) {
		bootstrappath = configini.GetString( "NDS-FORWARDER", "BOOTSTRAP_PATH", "");
		bootstrapversion = configini.GetString( "NDS-FORWARDER", "BOOTSTRAP_VERSION", "");
	} else {
		displayInit();
		SetTargetBootstrap();
	}
	
	CIniFile settingsini;
	if(settingsini.LoadIniFile(gamebootstrap.c_str())) {
		ndsPath = settingsini.GetString( "NDS-BOOTSTRAP", "NDS_PATH", "");
		savePath = settingsini.GetString( "NDS-BOOTSTRAP", "SAV_PATH", "");
		settingsini.SaveIniFile((bootstrappath+"nds-bootstrap.ini").c_str());
	} else {
		displayInit();
		SetTargetRom();
		//SetSavePath();
	}
}

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

int lastRanROM() {
	char game_TID[5];
	FILE *f_nds_file = fopen(ndsPath.c_str(), "rb");
	typedef struct {
		char gameTitle[12];			//!< 12 characters for the game title.
		char gameCode[4];			//!< 4 characters for the game code.
	} sNDSHeadertitlecodeonly;
	fseek(f_nds_file, offsetof(sNDSHeadertitlecodeonly, gameCode), SEEK_SET);
	fread(game_TID, 1, 4, f_nds_file);
	game_TID[4] = 0;
	game_TID[3] = 0;
	fclose(f_nds_file);
	std::vector<char*> argarray;
	argarray.push_back(strdup((bootstrappath+bootstrapversion).c_str()));
	if (access(savePath.c_str(), F_OK) && strcmp(game_TID, "###") != 0) {
		displayInit();
		consoleSelect(&lowerScreen);
		printf("Creating save file...\n");
		static const int BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		memset(buffer, 0, sizeof(buffer));
		int savesize = 524288;	// 512KB (default size for most games)
		// Set save size to 8KB for the following games
		if (strcmp(game_TID, "ASC") == 0 )	// Sonic Rush
		{
			savesize = 8192;
		}
		// Set save size to 256KB for the following games
		if (strcmp(game_TID, "AMH") == 0 )	// Metroid Prime Hunters
		{
			savesize = 262144;
		}
		// Set save size to 1MB for the following games
		if ( strcmp(game_TID, "AZL") == 0		// Wagamama Fashion: Girls Mode/Style Savvy/Nintendo presents: Style Boutique/Namanui Collection: Girls Style
			|| strcmp(game_TID, "BKI") == 0 )	// The Legend of Zelda: Spirit Tracks
		{
			savesize = 1048576;
		}
		// Set save size to 32MB for the following games
		if (strcmp(game_TID, "UOR") == 0 )	// WarioWare - D.I.Y. (Do It Yourself)
		{
			savesize = 1048576*32;
		}
		FILE *pFile = fopen(savePath.c_str(), "wb");
		if (pFile) {
			for (int i = savesize; i > 0; i -= BUFFER_SIZE) {
				fwrite(buffer, 1, sizeof(buffer), pFile);
			}
			fclose(pFile);
		}
		printf("Save file created!\n");
		
		for (int i = 0; i < 30; i++) {
			swiWaitForVBlank();
		}
	}
	return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true);
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// overwrite reboot stub identifier
	extern u64 *fake_heap_end;
	*fake_heap_end = 0;

	defaultExceptionHandler();
	if (!fatInitDefault()) {
		displayInit();
		consoleSelect(&lowerScreen);
		printf("fatInitDefault failed!");
		stop();
	}
	
	datapath=tmpdatapath;
	gamebootstrap = (datapath + "bootstrap.ini");
	LoadSettings();
	scanKeys();
	int keys = keysDown();
	if (keys&KEY_A) {
		for (int i = 0; i < 30; i++) {
			swiWaitForVBlank();
		}
		displayInit();
		menu mainmenu;
		mainmenu.AddOption("Set target bootstrap");
		mainmenu.AddOption("Set target rom");
		mainmenu.AddOption("Set save folder");
		mainmenu.AddOption("Start");
		int ret = -1;
		while((ret = mainmenu.DoMenu(&lowerScreen)) != 3) {
			if (ret==0)
				SetTargetBootstrap();
			else if (ret==1)
				SetTargetRom();
			else if (ret==2)
				SetSavePath();
		}
	}
	
	swiWaitForVBlank();

	fifoWaitValue32(FIFO_USER_06);

	int err = lastRanROM();
	displayInit();
	consoleSelect(&lowerScreen);
	iprintf ("Start failed. Error %i", err);
	stop();

	return 0;
}
