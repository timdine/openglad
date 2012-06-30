/* Copyright (C) 1995-2002  FSGames. Ported by Sean Ford and Yan Shosh
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "scenbrowse.h"
#include "input.h"
#include "util.h"

/* Changelog
 * 	8/8/02: Zardus: added scrolling-by-minimap
 * 		Zardus: added scrolling-by-keyboard
 */

#include <string>
using namespace std;
#include <stdlib.h>
#define MINIMUM_TIME 0

// From picker // just emulate these so other files are happy
// Difficulty settings .. in percent, so 100 == normal
Sint32 current_difficulty = 1; // setting 'normal'
Sint32 difficulty_level[DIFFICULTY_SETTINGS] =
    {
        50,
        100,
        200,
    };  // end of difficulty settings

FILE * open_misc_file(const char *, const char *, const char *);

Sint32 do_load(screen *ascreen);  // load a scenario or grid
Sint32 score_panel(screen *myscreen);
void info_box(walker  *target, screen * myscreen);
void set_facing(walker *target, screen *myscreen);
void set_name(walker  *target, screen * myscreen);
void scenario_options(screen * myscreen);
Sint32 quit(Sint32);



Sint32 browse(screen *screenp);


screen *myscreen;  // global for scen?

// Zardus: our prefs object from view.cpp
extern options * theprefs;

// To appease the linker, we'll fake these from picker.cpp
Sint32 statcosts[NUM_FAMILIES][6];
Sint32 costlist[NUM_FAMILIES];

Sint32 calculate_level(Uint32 howmuch)
{
	return (Sint32) (howmuch/10);
}

Sint32 *mymouse;
Uint8 *mykeyboard;
//scenario *myscen = new scenario;
Sint32 currentmode = OBJECT_MODE;
Uint32 currentlevel = 1;
char scen_name[10] = "test";
char grid_name[10] = "test";

unsigned char  *mypixdata[PIX_MAX+1];
unsigned char scenpalette[768];
Sint32 backcount=0, forecount = 0;
Sint32 myorder = ORDER_LIVING;
char currentteam = 0;
Sint32 event = 1;  // need to redraw?
Sint32 levelchanged = 0;  // has level changed?
Sint32 cyclemode = 0;      // for color cycling
Sint32 grid_aligned = 1;  // aligned by grid, default is on
//buffers: PORT: changed start_time to start_time_s to avoid conflict with
//input.cpp
Sint32 start_time_s; // for timer ops

smoother  *mysmoother = NULL;

Sint32 backgrounds[] = {
                         PIX_GRASS1, PIX_GRASS2, PIX_GRASS_DARK_1, PIX_GRASS_DARK_2,
                         //PIX_GRASS_DARK_B1, PIX_GRASS_DARK_BR, PIX_GRASS_DARK_R1, PIX_GRASS_DARK_R2,
                         PIX_BOULDER_1, PIX_GRASS_DARK_LL, PIX_GRASS_DARK_UR, PIX_GRASS_RUBBLE,

                         PIX_GRASS_LIGHT_LEFT_TOP, PIX_GRASS_LIGHT_1,
                         PIX_GRASS_LIGHT_RIGHT_TOP, PIX_WATER1,

                         PIX_WATERGRASS_U, PIX_WATERGRASS_D,
                         PIX_WATERGRASS_L, PIX_WATERGRASS_R,

                         PIX_DIRTGRASS_UR1, PIX_DIRT_1, PIX_DIRT_1, PIX_DIRTGRASS_LL1,
                         PIX_DIRTGRASS_LR1, PIX_DIRT_DARK_1, PIX_DIRT_DARK_1, PIX_DIRTGRASS_UL1,

                         PIX_DIRTGRASS_DARK_UR1, PIX_DIRTGRASS_DARK_LL1,
                         PIX_DIRTGRASS_DARK_LR1, PIX_DIRTGRASS_DARK_UL1,

                         PIX_JAGGED_GROUND_1, PIX_JAGGED_GROUND_2,
                         PIX_JAGGED_GROUND_3, PIX_JAGGED_GROUND_4,

                         PIX_PATH_1, PIX_PATH_2, PIX_PATH_3, PIX_PATH_4,
                         PIX_COBBLE_1, PIX_COBBLE_2, PIX_COBBLE_3, PIX_COBBLE_4,

                         //PIX_WALL2, PIX_WALL3, PIX_WALL4, PIX_WALL5,

                         PIX_WALL4, PIX_WALL_ARROW_GRASS,
                         PIX_WALL_ARROW_FLOOR, PIX_WALL_ARROW_GRASS_DARK,

                         PIX_WALL2, PIX_WALL3, PIX_H_WALL1, PIX_WALL_LL,

                         PIX_WALLSIDE_L, PIX_WALLSIDE_C, PIX_WALLSIDE_R, PIX_WALLSIDE1,

                         PIX_WALLSIDE_CRACK_C1, PIX_WALLSIDE_CRACK_C1,
                         PIX_TORCH1, PIX_VOID1,

                         //PIX_VOID1, PIX_FLOOR1, PIX_VOID1, PIX_VOID1,

                         PIX_CARPET_SMALL_TINY, PIX_CARPET_M2, PIX_PAVEMENT1, PIX_FLOOR1,

                         //PIX_PAVEMENT1, PIX_PAVEMENT2, PIX_PAVEMENT3, PIX_PAVEMENT3,
                         PIX_FLOOR_PAVEL, PIX_FLOOR_PAVEU, PIX_FLOOR_PAVED, PIX_FLOOR_PAVED,

                         PIX_WALL_LL,
                         PIX_WALLTOP_H,
                         PIX_PAVESTEPS1,
                         PIX_BRAZIER1,

                         PIX_PAVESTEPS2L, PIX_PAVESTEPS2, PIX_PAVESTEPS2R, PIX_PAVESTEPS1,
                         //PIX_TORCH1, PIX_TORCH2, PIX_TORCH3, PIX_TORCH3,

                         PIX_COLUMN1, PIX_COLUMN2, PIX_COLUMN2, PIX_COLUMN2,

                         PIX_TREE_T1, PIX_TREE_T1, PIX_TREE_T1, PIX_TREE_T1,
                         PIX_TREE_ML, PIX_TREE_M1, PIX_TREE_MT, PIX_TREE_MR,
                         PIX_TREE_B1, PIX_TREE_B1, PIX_TREE_B1, PIX_TREE_B1,

                         PIX_CLIFF_BACK_L, PIX_CLIFF_BACK_1, PIX_CLIFF_BACK_2, PIX_CLIFF_BACK_R,
                         PIX_CLIFF_LEFT, PIX_CLIFF_BOTTOM, PIX_CLIFF_TOP, PIX_CLIFF_RIGHT,
                         PIX_CLIFF_LEFT, PIX_CLIFF_TOP_L, PIX_CLIFF_TOP_R, PIX_CLIFF_RIGHT,
                     };

Sint32 rowsdown = 0;
Sint32 maxrows = ((sizeof(backgrounds)/4) / 4);
text *scentext;

int main(int argc, char **argv)
{
	cfg.commandline(argc, argv);
	Sint32 i,j;
	Sint32 extra;
	//  unsigned char input;
	//  char soundpath[80];
	Sint32 windowx, windowy;
	walker  *newob;
	Sint32 mx, my;
	char mystring[80]; //, someletter;
	//  Sint32 pos;
	short count;
	// char buffer[80];
	char * filepath;

	//Zardus: add: init the input
	init_input();

	// Zardus: create dirs
	create_dataopenglad();

	filepath = get_file_path("openglad.cfg");
	cfg.parse(filepath);
	delete filepath;

	// Zardus: load prefs
	theprefs = new options;

	// For informational purposes..
	if (argc > 1 && !strcmp(argv[1], "/?") )
	{
		printf("\nScenario Editor version %s\n", GLAD_VER);

		// Free memory ..
		meminfo Memory;
		Memory.FreeLinAddrSpace = 0;
		//buffers: PORT: union REGS regs;
		//buffers: PORT: struct SREGS sregs;
		Sint32 bytes;

		//buffers: PORT: regs.x.eax = 0x00000500;
		//buffers: PORT: memset( &sregs, 0, sizeof(sregs) );
		//buffers: PORT: sregs.es = FP_SEG( &Memory );
		//buffers: regs.x.edi = FP_OFF( &Memory );

		//buffers: int386x( DPMI_INT, &regs, &regs, &sregs );
		bytes = Memory.FreeLinAddrSpace * 4096;
		printf("\nMemory available: %d bytes.\n", bytes);

		exit (0);
	}

	myscreen = new screen(1);

	scentext = new text(myscreen);
	// Set the un-set text to empty ..
	for (i=0; i < 60; i ++)
		myscreen->scentext[i][0] = 0;

	// Install our masking timer interrupt
	grab_timer();

	// Now install the keyboard interrupt
	grab_keyboard();
	// Clear the keyboard state
	clear_keyboard();

	// Now do the mouse ..
	grab_mouse();

	// Get our pixie data ..
	load_map_data(mypixdata);
	load_and_set_palette("our.pal", scenpalette);

	// Set our default par value ..
	myscreen->par_value = 1;
	load_scenario("test", myscreen);

	myscreen->clearfontbuffer();
	myscreen->redraw();
	myscreen->refresh();

	//******************************
	// Keyboard loop
	//******************************

	grab_mouse();
	mykeyboard = query_keyboard();

	//
	// This is the main program loop
	//
	while(1)
	{
		// Reset the timer count to zero ...
		reset_timer();

		if (myscreen->end)
			break;

		//buffers: get keys and stuff
		get_input_events(POLL);

		// Zardus: COMMENT: I went through and replaced dumbcounts with get_input_events.

		// Delete all with ^D
		if (mykeyboard[SDLK_d] && mykeyboard[SDLK_LCTRL])
		{
			remove_all_objects(myscreen);
			event = 1;
		}

		// Change teams ..
		if (mykeyboard[SDLK_0])
		{
			currentteam = 0;
			event = 1;
		}
		if (mykeyboard[SDLK_1])
		{
			currentteam = 1;
			event = 1;
		}
		if (mykeyboard[SDLK_2])
		{
			currentteam = 2;
			event = 1;
		}
		if (mykeyboard[SDLK_3])
		{
			currentteam = 3;
			event = 1;
		}
		if (mykeyboard[SDLK_4])
		{
			currentteam = 4;
			event = 1;
		}
		if (mykeyboard[SDLK_5])
		{
			currentteam = 5;
			event = 1;
		}
		if (mykeyboard[SDLK_6])
		{
			currentteam = 6;
			event = 1;
		}
		if (mykeyboard[SDLK_7])
		{
			currentteam = 7;
			event = 1;
		}

		// Toggle grid alignment
		if (mykeyboard[SDLK_g])
		{
			grid_aligned = (grid_aligned+1)%3;
			event = 1;
			while (mykeyboard[SDLK_g])
				//buffers: dumbcount++;
				get_input_events(WAIT);
		}

		// Show help
		if (mykeyboard[SDLK_h])
		{
			release_mouse();
			do_help(myscreen);
			myscreen->clearfontbuffer();
			grab_mouse();
			event = 1;
		}

		if (mykeyboard[SDLK_KP_MULTIPLY]) // options menu
		{
			release_mouse();
			scenario_options(myscreen);
			grab_mouse();
			event = 1; // redraw screen
		}

		// Load scenario, etc. ..
		if (mykeyboard[SDLK_l])
		{
			myscreen->draw_button(30, 15, 220, 25, 1, 1);
			scentext->write_xy(32, 17, "Loading Level...", DARK_BLUE, 1);
			do_load(myscreen);
			myscreen->clearfontbuffer();
		}
		
		// Browse
		if (mykeyboard[SDLK_b])
		{
			browse(myscreen);
			myscreen->clearfontbuffer();
		}


		// Switch modes ..
		if (mykeyboard[SDLK_m])        // switch to map or guys ..
		{
			event = 1;
			currentmode = (currentmode+1) %2;
			while (mykeyboard[SDLK_m])
				get_input_events(WAIT);
		}

		// New names
		if (mykeyboard[SDLK_n])
		{
			event = 1;
			//gotoxy(1, 23);
			myscreen->draw_button(50, 30, 200, 40, 1, 1);
			scentext->write_xy(52, 32, "New name [G/S] : ", DARK_BLUE, 1);
			myscreen->buffer_to_screen(0, 0, 320, 200);
			while ( !mykeyboard[SDLK_g] && !mykeyboard[SDLK_s] )
				get_input_events(WAIT);
			if (mykeyboard[SDLK_s])
			{
				myscreen->draw_button(50, 30, 200, 40, 1, 1);
				myscreen->buffer_to_screen(0, 0, 320, 200);
				new_scenario_name();
				while (mykeyboard[SDLK_s])
					get_input_events(WAIT);
			} // end new scenario name
			else if (mykeyboard[SDLK_g])
			{
				myscreen->draw_button(50, 30, 200, 40, 1, 1);
				myscreen->buffer_to_screen(0, 0, 320, 200);
				new_grid_name();
				while (mykeyboard[SDLK_g])
					get_input_events(WAIT);
			} // end new grid name
			myscreen->clearfontbuffer(50,30,150,10);
		}

		// Enter scenario text ..
		if (mykeyboard[SDLK_t])
		{
#define TEXT_DOWN(x)  (14+((x)*7))
   #define TL 4
			//gotoxy(1, 1);
			myscreen->draw_button(0, 10, 200, 200, 2, 1);
			myscreen->buffer_to_screen(0, 0, 320, 200);
			scentext->write_xy(TL, TEXT_DOWN(0), "Enter new scenario text;", DARK_BLUE, 1);
			scentext->write_xy(TL, TEXT_DOWN(1), " PERIOD (.) alone to end.", DARK_BLUE, 1);
			scentext->write_xy(TL, TEXT_DOWN(2), "*--------*---------*---------*", DARK_BLUE, 1);
			myscreen->buffer_to_screen(0, 0, 320, 200);
			myscreen->scentextlines = 0;
			count = 2;
			for (i=0; i < 23; i++)
				if (strlen(myscreen->scentext[i]))
					scentext->write_xy(TL, TEXT_DOWN(i+3), myscreen->scentext[i], DARK_BLUE, 1);
			myscreen->buffer_to_screen(0, 0, 320, 200);
			extra = 1;
			for (i=0; i < 60; i++)
			{
				count++;
				mystring[0] = 0;
				if (! (count%26) )
				{
					myscreen->draw_box(TL, TEXT_DOWN(3), 196, 196, 27, 1, 1);
					myscreen->buffer_to_screen(0, 0, 320, 200);
					for (j=0; j < 23; j++)
					{
						count = j+(23*extra);
						if (count < 60)
							if (strlen(myscreen->scentext[count]))
								scentext->write_xy(TL, TEXT_DOWN(j+3), myscreen->scentext[count], DARK_BLUE, 1);
					}
					count = 3;
					extra++;
					myscreen->buffer_to_screen(0, 0, 320, 200);
				}
				strcpy(mystring, scentext->input_string(TL, TEXT_DOWN(count), 30, myscreen->scentext[i]) );
				strcpy(myscreen->scentext[i], mystring);
				if (!strcmp(".", mystring)) // says end ..
				{
					i = 70;
					myscreen->draw_box(0, 10, 200, 200, 0, 1, 1);
					myscreen->buffer_to_screen(0, 0, 320, 200);
					myscreen->scentext[i][0] = 0;
					event = 1;
				}
				else
					myscreen->scentextlines++;
			}
			myscreen->draw_box(0, 10, 200, 200, 0, 1, 1);
			myscreen->buffer_to_screen(0, 0, 320, 200);
			event = 1;
		}

		// Display the scenario help..
		if (mykeyboard[SDLK_SLASH] && (mykeyboard[SDLK_RSHIFT] || mykeyboard[SDLK_LSHIFT]))
		{
			read_scenario(myscreen);
			myscreen->clearfontbuffer();
			event = 1;
		}

		// Change level of current guy being placed ..
		if (mykeyboard[SDLK_RIGHTBRACKET])
		{
			currentlevel++;
			//while (mykeyboard[SDLK_RIGHTBRACKET])
			//  dumbcount++;
			event = 1;
		}
		if (mykeyboard[SDLK_LEFTBRACKET] && currentlevel > 1)
		{
			currentlevel--;
			//while (mykeyboard[SDLK_LEFTBRACKET])
			//  dumbcount++;
			event = 1;
		}

		// Change between generator and living orders
		if (mykeyboard[SDLK_o])        // this is letter o
		{
			if (myorder == ORDER_LIVING)
			{
				myorder = ORDER_GENERATOR;
				forecount = FAMILY_TENT;
			}
			else if (myorder == ORDER_GENERATOR)
				myorder = ORDER_SPECIAL;   // for placing team guys ..
			else if (myorder == ORDER_SPECIAL)
			{
				myorder = ORDER_TREASURE;
				forecount = FAMILY_DRUMSTICK;
			}
			else if (myorder == ORDER_TREASURE)
				myorder = ORDER_WEAPON;
			else if (myorder == ORDER_WEAPON)
				myorder = ORDER_LIVING;
			currentmode = OBJECT_MODE;
			event = 1; // change score panel
			while (mykeyboard[SDLK_o])
				get_input_events(WAIT);
		}

		// Slide tile selector down ..
		if (mykeyboard[SDLK_DOWN])
		{
			rowsdown++;
			event = 1;
			if (rowsdown >= maxrows)
				rowsdown -= maxrows;
			score_panel(myscreen);
			while (mykeyboard[SDLK_DOWN])
				get_input_events(WAIT);
		}

		// Slide tile selector up ..
		if (mykeyboard[SDLK_UP])
		{
			rowsdown--;
			event = 1;
			if (rowsdown < 0)
				rowsdown += maxrows;
			if (rowsdown <0 || rowsdown >= maxrows) // bad case
				rowsdown = 0;
			score_panel(myscreen);
			while (mykeyboard[SDLK_UP])
				get_input_events(WAIT);
		}

		// Smooth current map, F5
		if (mykeyboard[SDLK_F5])
		{
			if (mysmoother)
				delete mysmoother;
			mysmoother = new smoother();
			mysmoother->set_target(myscreen);
			mysmoother->smooth();
			while (mykeyboard[SDLK_F5])
				get_input_events(WAIT);
			event = 1;
			levelchanged = 1;
		}

		// Change to new palette ..
		if (mykeyboard[SDLK_F9])
		{
			load_and_set_palette("our.pal", scenpalette);
			while (mykeyboard[SDLK_F9])
				get_input_events(WAIT);
		}

		// Toggle color cycling
		if (mykeyboard[SDLK_F10])
		{
			cyclemode++;
			cyclemode %= 2;
			while (mykeyboard[SDLK_F10])
				get_input_events(WAIT);
		}
		// Now perform color cycling if selected
		if (cyclemode)
		{
			cycle_palette(scenpalette, WATER_START, WATER_END, 1);
			cycle_palette(scenpalette, ORANGE_START, ORANGE_END, 1);
		}

		// Mouse stuff ..
		mymouse = query_mouse();

		// Scroll the screen ..
		// Zardus: ADD: added scrolling by keyboard
		// Zardus: PORT: disabled mouse scrolling
		if ((mykeyboard[SDLK_KP8] || mykeyboard[SDLK_KP7] || mykeyboard[SDLK_KP9]) // || mymouse[MOUSE_Y]< 2)
		        && myscreen->topy >= 0) // top of the screen
			set_screen_pos(myscreen, myscreen->topx,
			               myscreen->topy-SCROLLSIZE);
		if ((mykeyboard[SDLK_KP2] || mykeyboard[SDLK_KP1] || mykeyboard[SDLK_KP3]) // || mymouse[MOUSE_Y]> 198)
		        && myscreen->topy <= (GRID_SIZE*myscreen->maxy)-18) // scroll down
			set_screen_pos(myscreen, myscreen->topx,
			               myscreen->topy+SCROLLSIZE);
		if ((mykeyboard[SDLK_KP4] || mykeyboard[SDLK_KP7] || mykeyboard[SDLK_KP1]) // || mymouse[MOUSE_X]< 2)
		        && myscreen->topx >= 0) // scroll left
			set_screen_pos(myscreen, myscreen->topx-SCROLLSIZE,
			               myscreen->topy);
		if ((mykeyboard[SDLK_KP6] || mykeyboard[SDLK_KP3] || mykeyboard[SDLK_KP9]) // || mymouse[MOUSE_X] > 318)
		        && myscreen->topx <= (GRID_SIZE*myscreen->maxx)-18) // scroll right
			set_screen_pos(myscreen, myscreen->topx+SCROLLSIZE,
			               myscreen->topy);

		if (mymouse[MOUSE_LEFT])       // put or remove the current guy
		{
			event = 1;
			mx = mymouse[MOUSE_X];
			my = mymouse[MOUSE_Y];

			// Zardus: ADD: can move map by clicking on minimap
			if (mx > myscreen->viewob[0]->endx - myscreen->viewob[0]->myradar->xview - 4
			        && my > myscreen->viewob[0]->endy - myscreen->viewob[0]->myradar->yview - 4
			        && mx < myscreen->viewob[0]->endx - 4 && my < myscreen->viewob[0]->endy - 4)
			{
				mx -= myscreen->viewob[0]->endx - myscreen->viewob[0]->myradar->xview - 4;
				my -= myscreen->viewob[0]->endy - myscreen->viewob[0]->myradar->yview - 4;

				// Zardus: above set_screen_pos doesn't take into account that minimap scrolls too. This one does.
				set_screen_pos (myscreen, myscreen->viewob[0]->myradar->radarx * GRID_SIZE + mx * GRID_SIZE - 160,
				                myscreen->viewob[0]->myradar->radary * GRID_SIZE + my * GRID_SIZE - 100);
			}
			else if ( (mx >= S_LEFT) && (mx <= S_RIGHT) &&
			          (my >= S_UP) && (my <= S_DOWN) )      // in the main window
			{
				windowx = mymouse[MOUSE_X] + myscreen->topx - myscreen->viewob[0]->xloc; // - S_LEFT
				if (grid_aligned==1)
					windowx -= (windowx%GRID_SIZE);
				windowy = mymouse[MOUSE_Y] + myscreen->topy - myscreen->viewob[0]->yloc; // - S_UP
				if (grid_aligned==1)
					windowy -= (windowy%GRID_SIZE);
				if (mykeyboard[SDLK_i]) // get info on current object
				{
					newob = myscreen->add_ob(ORDER_LIVING, FAMILY_ELF);
					newob->setxy(windowx, windowy);
					if (some_hit(windowx, windowy, newob, myscreen))
						info_box(newob->collide_ob,myscreen);
					myscreen->remove_ob(newob,0);
					continue;
				}  // end of info mode
				if (mykeyboard[SDLK_f]) // set facing of current object
				{
					newob = myscreen->add_ob(ORDER_LIVING, FAMILY_ELF);
					newob->setxy(windowx, windowy);
					if (some_hit(windowx, windowy, newob, myscreen))
						set_facing(newob->collide_ob,myscreen);
					myscreen->remove_ob(newob,0);
					continue;
				}  // end of set facing

				if (mykeyboard[SDLK_r]) // (re)name the current object
				{
					newob = myscreen->add_ob(ORDER_LIVING, FAMILY_ELF);
					newob->setxy(windowx, windowy);
					if (some_hit(windowx, windowy, newob, myscreen))
					{
						set_name(newob->collide_ob,myscreen);
						levelchanged = 1;
					}
					myscreen->remove_ob(newob,0);
				}  // end of info mode
				else if (currentmode == OBJECT_MODE)
				{
					newob = myscreen->add_ob(myorder, forecount);
					newob->setxy(windowx, windowy);
					newob->team_num = currentteam;
					newob->stats->level = currentlevel;
					newob->dead = 0; // just in case
					newob->collide_ob = 0;
					if ( (grid_aligned==1) && some_hit(windowx, windowy, newob, myscreen))
					{
						if (mykeyboard[SDLK_LCTRL] &&    // are we holding the erase?
						        newob->collide_ob )                    // and hit a guy?
						{
							myscreen->remove_ob(newob->collide_ob,0);
							while (mymouse[MOUSE_LEFT])
							{
								mymouse = query_mouse();
							}
							levelchanged = 1;
						} // end of deleting guy
						if (newob)
						{
							myscreen->remove_ob(newob,0);
							newob = NULL;
						}
					}  // end of failure to put guy
					else if (grid_aligned == 2)
					{
						newob->draw(myscreen->viewob[0]);
						myscreen->buffer_to_screen(0, 0, 320, 200);
						start_time_s = query_timer();
						while ( mymouse[MOUSE_LEFT] && (query_timer()-start_time_s) < 36 )
						{
							mymouse = query_mouse();
						}
						levelchanged = 1;
					}
					if (mykeyboard[SDLK_LCTRL] && newob)
					{
						myscreen->remove_ob(newob,0);
						newob = NULL;
					}
					//       while (mymouse[MOUSE_LEFT])
					//         mymouse = query_mouse();
				}  // end of putting a guy
				if (currentmode == MAP_MODE)
				{
					windowx /= GRID_SIZE;  // get the map position ..
					windowy /= GRID_SIZE;
					// Set to our current selection
					myscreen->grid[windowy*(myscreen->maxx)+windowx] = some_pix(backcount);
					levelchanged = 1;
					if (!mykeyboard[SDLK_LCTRL]) // smooth a few squares, if not control
					{
						if (mysmoother)
						{
							delete mysmoother;
							mysmoother = new smoother();
							mysmoother->set_target(myscreen);
						}
						for (i=windowx-1; i <= windowx+1; i++)
							for (j=windowy-1; j <=windowy+1; j++)
								if (i >= 0 && i < myscreen->maxx &&
								        j >= 0 && j < myscreen->maxy)
									mysmoother->smooth(i, j);
					}
					else if (mysmoother) // update smoother anyway
					{
						delete mysmoother;
						mysmoother = new smoother();
						mysmoother->set_target(myscreen);
					}
					myscreen->viewob[0]->myradar->update();
				}  // end of setting grid square
			} // end of main window
			//    if ( (mx >= PIX_LEFT) && (mx <= PIX_RIGHT) &&
			//        (my >= PIX_TOP) && (my <= PIX_BOTTOM) ) // grid menu
			if (mx >= S_RIGHT && my >= PIX_TOP && my <= PIX_BOTTOM)
			{
				//windowx = (mx - PIX_LEFT) / GRID_SIZE;
				windowx = (mx-S_RIGHT) / GRID_SIZE;
				windowy = (my - PIX_TOP) / GRID_SIZE;
				backcount = backgrounds[ (windowx + ((windowy+rowsdown) * PIX_OVER))
				                         % (sizeof(backgrounds)/4)];
				backcount %= NUM_BACKGROUNDS;
				currentmode = MAP_MODE;
			} // end of background grid window

		}      // end of left mouse button

		if (mymouse[MOUSE_RIGHT])      // cycle through things ...
		{
			event = 1;
			if (currentmode == OBJECT_MODE)
			{
				if (myorder == ORDER_LIVING)
					forecount = (forecount+1) % NUM_FAMILIES;
				else if (myorder == ORDER_TREASURE)
					forecount = (forecount+1) % (MAX_TREASURE+1);
				else if (myorder == ORDER_GENERATOR)
					forecount = (forecount+1) % 4;
				else if (myorder == ORDER_WEAPON)
					forecount = (forecount+1) % (FAMILY_DOOR+1); // use largest weapon
				else
					forecount = 0;
			} // end of if object mode
			if (currentmode == MAP_MODE)
			{
				windowx = mymouse[MOUSE_X] + myscreen->topx - myscreen->viewob[0]->xloc; // - S_LEFT
				windowx -= (windowx%GRID_SIZE);
				windowy = mymouse[MOUSE_Y] + myscreen->topy - myscreen->viewob[0]->yloc; // - S_UP
				windowy -= (windowy%GRID_SIZE);
				windowx /= GRID_SIZE;
				windowy /= GRID_SIZE;
				backcount = myscreen->grid[windowy*(myscreen->maxx)+windowx];
			}
			while (mymouse[MOUSE_RIGHT])
			{
				mymouse = query_mouse();
			}
		}

		if (event)
		{
			release_mouse();
			myscreen->redraw();
			score_panel(myscreen);
			myscreen->refresh();
			//    score_panel(myscreen);
			grab_mouse();
		}
		event = 0;

		if (mykeyboard[SDLK_ESCAPE])
			quit(0);

	}

	return 0;
}

Sint32 quit(Sint32 num)
{
	int i;

	for (i = 0; i < PIX_MAX+1; i++)
		if (mypixdata[i]) delete mypixdata[i];

	// Release the mouse ..
	release_mouse();

	// Release the keyboard interrupt
	release_keyboard();

	// And release the timer ..
	release_timer();

	// Get rid of the prefs
	delete theprefs;

	// And delete myscreen
	delete myscreen;

	// Delete scentext
	delete scentext;

	SDL_Quit();

	exit(num);
	return num;
}

Sint32 score_panel(screen *myscreen)
{
	char message[50];
	Sint32 i, j; // for loops
	//   static Sint32 family=-1, hitpoints=-1, score=-1, act=-1;
	static Sint32 numobs = myscreen->numobs;
	static Sint32 lm = 245;
	Sint32 curline = 0;
	Sint32 whichback;
	static char treasures[20][NUM_FAMILIES] =
	    { "BLOOD", "DRUMSTICK", "GOLD", "SILVER",
	      "MAGIC", "INVIS", "INVULN", "FLIGHT",
	      "EXIT", "TELEPORTER", "LIFE GEM", "KEY", "SPEED", "CC",
	    };
	static char weapons[20][NUM_FAMILIES] =
	    { "KNIFE", "ROCK", "ARROW", "FIREBALL",
	      "TREE", "METEOR", "SPRINKLE", "BONE",
	      "BLOOD", "BLOB", "FIRE ARROW", "LIGHTNING",
	      "GLOW", "WAVE 1", "WAVE 2", "WAVE 3",
	      "PROTECTION", "HAMMER", "DOOR",
	    };

	static char livings[NUM_FAMILIES][20] =
	    {  "SOLDIER", "ELF", "ARCHER", "MAGE",
	       "SKELETON", "CLERIC", "ELEMENTAL",
	       "FAERIE", "L SLIME", "S SLIME", "M SLIME",
	       "THIEF", "GHOST", "DRUID", "ORC",
	       "ORC CAPTAIN", "BARBARIAN", "ARCHMAGE",
	       "GOLEM", "G SKELETON", "TOWER1",
	    };

	// Hide the mouse ..
	release_mouse();

	// Draw the bounding box
	//myscreen->draw_dialog(lm-4, L_D(-1), 310, L_D(8), "Info");
	myscreen->draw_button(lm-4, L_D(-1)+4, 315, L_D(7)-2, 1, 1);

	// Show scenario and grid info
	strcpy(message, scen_name);
	uppercase(message);

	//myscreen->fastbox(lm, S_UP, 70, 8*5, 27, 1);
	scentext->write_xy(lm,L_D(curline++),message, DARK_BLUE, 1);

	strcpy(message, grid_name);
	uppercase(message);
	scentext->write_xy(lm,L_D(curline++),message, DARK_BLUE, 1);

	if (currentmode==MAP_MODE)
		scentext->write_xy(lm,L_D(curline++), "MODE: MAP", DARK_BLUE, 1);
	else if (currentmode==OBJECT_MODE)
		scentext->write_xy(lm,L_D(curline++), "MODE: OBS", DARK_BLUE, 1);

	// Get team number ..
	sprintf(message, "%d:", currentteam);
	if (myorder == ORDER_LIVING)
		strcat(message, livings[forecount]);
	else if (myorder == ORDER_GENERATOR)
		switch (forecount)      // who are we?
		{
			case FAMILY_TENT:
				strcat(message, "TENT");
				break;
			case FAMILY_TOWER:
				strcat(message, "TOWER");
				break;
			case FAMILY_BONES:
				strcat(message, "BONEPILE");
				break;
			case FAMILY_TREEHOUSE:
				strcat(message, "TREEHOUSE");
				break;
			default:
				strcat(message, "GENERATOR");
				break;
		}
	else if (myorder == ORDER_SPECIAL)
		strcat(message, "PLAYER");
	else if (myorder == ORDER_TREASURE)
		strcat(message, treasures[forecount]);
	else if (myorder == ORDER_WEAPON)
		strcat(message, weapons[forecount]);
	else
		strcat(message, "UNKNOWN");
	scentext->write_xy(lm, L_D(curline++), message, DARK_BLUE, 1);

	// Level display
	sprintf(message, "LVL: %u", currentlevel);
	//myscreen->fastbox(lm,L_D(curline),55,7,27, 1);
	scentext->write_xy(lm, L_D(curline++), message, DARK_BLUE, 1);

	// Is grid alignment on?
	//myscreen->fastbox(lm, L_D(curline),65, 7, 27, 1);
	if (grid_aligned==1)
		scentext->write_xy(lm, L_D(curline++), "ALIGN: ON", DARK_BLUE, 1);
	else if (grid_aligned==2)
		scentext->write_xy(lm, L_D(curline++), "ALIGN: STACK", DARK_BLUE, 1);
	else
		scentext->write_xy(lm, L_D(curline++), "ALIGN: OFF", DARK_BLUE, 1);

	numobs = myscreen->numobs;
	//myscreen->fastbox(lm,L_D(curline),55,7,27, 1);
	sprintf(message, "OB: %d", numobs);
	scentext->write_xy(lm,L_D(curline++),message, DARK_BLUE, 1);

	// Show the background grid ..
	myscreen->putbuffer(lm+40, PIX_TOP-16, GRID_SIZE, GRID_SIZE,
	                    0, 0, 320, 200, mypixdata[backcount]+3);

	//   rowsdown = (NUM_BACKGROUNDS / 4) + 1;
	//   rowsdown = 0; // hack for now
	for (i=0; i < PIX_OVER; i++)
	{
		for (j=0; j < 4; j++)
		{
			//myscreen->back[i]->draw( S_RIGHT+(i*8), S_UP+100);
			//myscreen->back[0]->draw(64, 64);
			whichback = (i+(j+rowsdown)*4) % (sizeof(backgrounds)/4);
			myscreen->putbuffer(S_RIGHT+i*GRID_SIZE, PIX_TOP+j*GRID_SIZE,
			                    GRID_SIZE, GRID_SIZE,
			                    0, 0, 320, 200,
			                    mypixdata[ backgrounds[whichback] ]+3);
		}
	}
	myscreen->draw_box(S_RIGHT, PIX_TOP,
	                   S_RIGHT+4*GRID_SIZE, PIX_TOP+4*GRID_SIZE, 0, 0, 1);
	myscreen->buffer_to_screen(0, 0, 320, 200);
	// Restore the mouse
	grab_mouse();

	return 1;
}


void set_screen_pos(screen *myscreen, Sint32 x, Sint32 y)
{
	myscreen->topx = x;
	myscreen->topy = y;
	event = 1;
}

void remove_all_objects(screen *master)
{
	oblink *fx = master->fxlist;

	while (fx)
	{
		if (fx->ob)
		{
			delete fx->ob;
			fx->ob = NULL;
		}
		else
			fx = fx->next;
	}
	if (fx && fx->ob)
		delete fx->ob;

	fx = master->oblist;
	while (fx)
	{
		if (fx->ob)
		{
			delete fx->ob;
			fx->ob = NULL;
		}
		else
			fx = fx->next;
	}
	if (fx && fx->ob)
		delete fx->ob;

	fx = master->weaplist;
	while (fx)
	{
		if (fx->ob)
		{
			delete fx->ob;
			fx->ob = NULL;
		}
		else
			fx = fx->next;
	}
	if (fx && fx->ob)
		delete fx->ob;

	master->numobs = 0;
} // end remove_all_objects

void remove_first_ob(screen *master)
{
	oblink  *here;

	here = master->oblist;

	while (here)
	{
		if (here->ob)
		{
			delete here->ob;
			return;
		}
		else
			here = here->next;
	}
}

Sint32 load_new_grid(screen *master)
{
	string tempstring;
	//char tempstring[80];

	scentext->write_xy(52, 32, "Grid name: ", DARK_BLUE, 1);
	tempstring = scentext->input_string(115, 32, 8, grid_name);
	//NORMAL_KEYBOARD(SDLKf("%s", tempstring);)
	if (tempstring.empty())
	{
		//buffers: our grid files are all lowercase...
		lowercase(tempstring);

		tempstring += grid_name;
	}

	//printf("DB: loading %s\n", grid_name);

	//buffers: PORT: changed .PIX to .pix
	tempstring += ".pix";
	master->grid = read_pixie_file(tempstring.c_str());
	master->maxx = master->grid[1];
	master->maxy = master->grid[2];
	master->grid = master->grid + 3;
	//printf("DB: loaded data to grid\n");
	//master->viewob[0]->myradar = new radar(master->viewob[0],
	//  master, 0);

	master->viewob[0]->myradar->start();
	master->viewob[0]->myradar->update();

	//printf("DB: made new radar\n");
	return 1;
}

Sint32 new_scenario_name()
{
	char tempstring[80];

	scentext->write_xy(52, 32, "Scenario name: ", DARK_BLUE, 1);
	strcpy(tempstring, scentext->input_string(135, 32, 8, scen_name));
	//NORMAL_KEYBOARD(SDLKf("%s", tempstring);)
	if (strlen(tempstring))
	{
		strcpy(scen_name, tempstring);
		//buffers: all our files are lowercase....
		lowercase(scen_name);
	}

	return 1;
}

Sint32 new_grid_name()
{
	char tempstring[80];

	scentext->write_xy(52, 32, "Grid name: ", DARK_BLUE, 1);
	strcpy(tempstring, scentext->input_string(117, 32, 8, grid_name));
	//NORMAL_KEYBOARD(SDLKf("%s", tempstring);)
	if (strlen(tempstring))
		strcpy(grid_name, tempstring);

	return 1;
}

void do_help(screen * myscreen)
{
	text *helptext = new text(myscreen);
	Sint32 lm = S_LEFT+4+43, tm=S_UP+15;  // left and top margins
	Sint32 lines = 0;

	// Zardus: new margins
	//myscreen->draw_button(S_LEFT+32,S_UP,S_RIGHT-1+16,S_DOWN-1,2, 1);
	myscreen->draw_button(S_LEFT+43,S_UP + 11,S_RIGHT-1+16,S_DOWN-1,2, 1);

	helptext->write_xy(lm + L_W(10), tm, "**HELP**", DARK_BLUE, 1);

	helptext->write_xy(lm, tm+L_H(++lines), "G : TOGGLE GRID ALIGNMENT", DARK_BLUE, 1);
	helptext->write_xy(lm, tm+L_H(++lines), "H : HELP", DARK_BLUE, 1);
	helptext->write_xy(lm, tm+L_H(++lines), "I : INFO ON CLICKED OBJECT", DARK_BLUE, 1);
	helptext->write_xy(lm, tm+L_H(++lines), "L : LOAD NEW SCEN OR GRID", DARK_BLUE, 1);
	helptext->write_xy(lm, tm+L_H(++lines), "M : TOGGLE OBJECT OR MAP MODE", DARK_BLUE, 1);
	helptext->write_xy(lm, tm+L_H(++lines), "N : NEW SCEN OR GRID NAME", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "O : TOGGLE LIVING/TENT/ETC", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "R : RENAME OBJECT", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "T : ENTER SCEN TEXT", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "* : TOGGLE SCENARIO OPTIONS", DARK_BLUE, 1);

	//lines +=1;
	helptext->write_xy(lm, tm+L_H(++lines), "ESC         : QUIT", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "LEFT CLICK  : PUT OB OR BACKGD", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "CTRL + LEFT : Remove Object", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "CTRL + D    : Remove all Obs", DARK_BLUE,1);

	helptext->write_xy(lm, tm+L_H(++lines), "RIGHT CLICK : CYCLE THRU OBS", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "KEYS 0-7    : CYCLE TEAM NUMBER", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "F5          : SMOOTH MAP TILES", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "[,]         : LOWER/RAISE OB LEVEL", DARK_BLUE,1);
	helptext->write_xy(lm, tm+L_H(++lines), "?           : DISPLAY SCEN TEXT", DARK_BLUE,1);

	myscreen->buffer_to_screen(0, 0, 320, 200);

	wait_for_key(SDLK_SPACE);

	delete helptext;
}

char some_pix(Sint32 whatback)
{
	Sint32 i;

	i = random(4);  // max # of types of any particular ..

	switch (whatback)
	{
		case PIX_GRASS1:
			switch (i)
			{
				case 0:
					return PIX_GRASS1;
				case 1:
					return PIX_GRASS2;
				case 2:
					return PIX_GRASS3;
				case 3:
					return PIX_GRASS4;
				default:
					return PIX_GRASS1;
			}
			//break;
		case PIX_GRASS_DARK_1:
			switch (i)
			{
				case 0:
					return PIX_GRASS_DARK_1;
				case 1:
					return PIX_GRASS_DARK_2;
				case 2:
					return PIX_GRASS_DARK_3;
				case 3:
					return PIX_GRASS_DARK_4;
				default:
					return PIX_GRASS_DARK_1;
			}
			//break;
		case PIX_GRASS_DARK_B1:
		case PIX_GRASS_DARK_B2:
			switch (i)
			{
				case 0:
				case 1:
					return PIX_GRASS_DARK_B1;
				case 2:
				case 3:
				default:
					return PIX_GRASS_DARK_B2;
			}
			//break;
		case PIX_GRASS_DARK_R1:
		case PIX_GRASS_DARK_R2:
			switch (i)
			{
				case 0:
				case 1:
					return PIX_GRASS_DARK_R1;
				case 2:
				case 3:
				default:
					return PIX_GRASS_DARK_R2;
			}
			//break;
		case PIX_WATER1:
			switch (i)
			{
				case 0:
					return PIX_WATER1;
				case 1:
					return PIX_WATER2;
				case 2:
					return PIX_WATER3;
				default:
					return PIX_WATER1;
			}
			//break;
		case PIX_PAVEMENT1:
			switch (random(12))
			{
				case 0:
					return PIX_PAVEMENT1;
				case 1:
					return PIX_PAVEMENT2;
				case 2:
					return PIX_PAVEMENT3;
				default:
					return PIX_PAVEMENT1;
			}
			//break;
		case PIX_COBBLE_1:
			switch (random(i))
			{
				case 0:
					return PIX_COBBLE_1;
				case 1:
					return PIX_COBBLE_2;
				case 2:
					return PIX_COBBLE_3;
				case 3:
					return PIX_COBBLE_4;
				default:
					return PIX_COBBLE_1;
			}
			//break;
		case PIX_BOULDER_1:
			switch (random(i))
			{
				case 0:
					return PIX_BOULDER_1;
				case 1:
					return PIX_BOULDER_2;
				case 2:
					return PIX_BOULDER_3;
				case 3:
					return PIX_BOULDER_4;
				default:
					return PIX_BOULDER_1;
			}
			//break;
		case PIX_JAGGED_GROUND_1:
			switch (i)
			{
				case 0:
					return PIX_JAGGED_GROUND_1;
				case 1:
					return PIX_JAGGED_GROUND_2;
				case 2:
					return PIX_JAGGED_GROUND_3;
				case 3:
					return PIX_JAGGED_GROUND_4;
				default:
					return PIX_JAGGED_GROUND_1;
			}
		default:
			return whatback;
	}
}

// Copy of collide from obmap; used manually .. :(
Sint32 check_collide(Sint32 x,  Sint32 y,  Sint32 xsize,  Sint32 ysize,
                   Sint32 x2, Sint32 y2, Sint32 xsize2, Sint32 ysize2)
{
	if (x < x2)
	{
		if (y < y2)
		{
			if (x2 - x < xsize &&
			        y2 - y < ysize)
				return 1;
		}
		else // y >= y2
		{
			if (x2 - x < xsize &&
			        y - y2 < ysize2)
				return 1;
		}
	}
	else // x >= x2
	{
		if (y < y2)
		{
			if (x - x2 < xsize2 &&
			        y2 - y < ysize)
				return 1;
		}
		else // y >= y2
		{
			if (x - x2 < xsize2 &&
			        y - y2 < ysize2)
				return 1;
		}
	}
	return 0;
}

// The old-fashioned hit check ..
walker * some_hit(Sint32 x, Sint32 y, walker  *ob, screen *screenp)
{
	oblink  *here;

	here = screenp->oblist;

	while (here)
	{
		if (here->ob && here->ob != ob)
			if (check_collide(x, y, ob->sizex, ob->sizey,
			                  here->ob->xpos, here->ob->ypos,
			                  here->ob->sizex, here->ob->sizey) )
			{
				ob->collide_ob = here->ob;
				return here->ob;
			}
		here = here->next;
	}

	// Also check the fx list ..
	here = screenp->fxlist;
	while (here)
	{
		if (here->ob && here->ob != ob)
			if (check_collide(x, y, ob->sizex, ob->sizey,
			                  here->ob->xpos, here->ob->ypos,
			                  here->ob->sizex, here->ob->sizey) )
			{
				ob->collide_ob = here->ob;
				return here->ob;
			}
		here = here->next;
	}

	// Also check the weapons list ..
	here = screenp->weaplist;
	while (here)
	{
		if (here->ob && !here->ob->dead && here->ob != ob)
			if (check_collide(x, y, ob->sizex, ob->sizey,
			                  here->ob->xpos, here->ob->ypos,
			                  here->ob->sizex, here->ob->sizey) )
			{
				ob->collide_ob = here->ob;
				return here->ob;
			}
		here = here->next;
	}

	ob->collide_ob = NULL;
	return NULL;
}

// Display info about the target object ..
#define INFO_DOWN(x) (25+7*x)
void info_box(walker  *target,screen * myscreen)
{
	text *infotext = new text(myscreen);
	Sint32 linesdown = 0;
	Sint32 lm = 25+32;
	char message[80];
	treasure  *teleporter, *temp;

	static const char *orders[] =
	    { "LIVING", "WEAPON", "TREASURE", "GENERATOR", "FX", "SPECIAL", };
	static const char *livings[] =
	    { "SOLDIER", "ELF", "ARCHER", "MAGE",
	      "SKELETON", "CLERIC", "ELEMENTAL",
	      "FAERIE", "L-SLIME", "S-SLIME",
	      "M-SLIME", "THIEF", "GHOST",
	      "DRUID",
	    };
	static const char *treasures[] =
	    { "BLOODSTAIN", "DRUMSTICK: FOOD",
	      "GOLD BAR", "SILVER BAR",
	      "MAGIC POTION", "INVISIBILITY POTION",
	      "INVULNERABILITY POTION",
	      "FLIGHT POTION", "EXIT", "TELEPORTER",
	      "LIFE GEM", "KEY", "SPEED", "CC",
	    };

	release_mouse();
	myscreen->draw_button(20+32, 20, 220+32, 170, 1, 1);

	infotext->write_xy(lm, INFO_DOWN(linesdown++), "INFO TEXT", DARK_BLUE,1);
	linesdown++;

	if (strlen(target->stats->name)) // it has a name
	{
		sprintf(message, "Name    : %s", target->stats->name);
		infotext->write_xy(lm, INFO_DOWN(linesdown++), message, DARK_BLUE,1);
	}

	sprintf(message, "Order   : %s", orders[(int)target->query_order()] );
	infotext->write_xy(lm, INFO_DOWN(linesdown++), message, DARK_BLUE,1);

	if (target->query_order() == ORDER_LIVING)
		sprintf(message, "Family  : %s",
		        livings[(int)target->query_family()] );
	else if (target->query_order() == ORDER_TREASURE)
		sprintf(message, "Family  : %s",
		        treasures[(int)target->query_family()] );
	else
		sprintf(message, "Family  : %d", target->query_family());
	infotext->write_xy(lm, INFO_DOWN(linesdown++), message, DARK_BLUE,1);

	sprintf(message, "Team Num: %d", target->team_num);
	infotext->write_xy(lm, INFO_DOWN(linesdown++), message, DARK_BLUE, 1);

	sprintf(message, "Position: %dx%d (%dx%d)", target->xpos, target->ypos,
	        (target->xpos/GRID_SIZE), (target->ypos/GRID_SIZE) );
	infotext->write_xy(lm, INFO_DOWN(linesdown++), message, DARK_BLUE,1);

	if (target->query_order() == ORDER_TREASURE &&
	        target->query_family()== FAMILY_EXIT)
		sprintf(message, "Exits to: Level %d", target->stats->level);
	else if (target->query_order() == ORDER_TREASURE &&
	         target->query_family() == FAMILY_TELEPORTER)
	{
		sprintf(message, "Group # : %d", target->stats->level);
		infotext->write_xy(lm, INFO_DOWN(linesdown++), message, DARK_BLUE,1);
		temp = (treasure  *) target;
		teleporter = (treasure  *) temp->find_teleport_target();
		if (!teleporter || teleporter == target)
			infotext->write_xy(lm, INFO_DOWN(linesdown++), "Goes to : Itself!", DARK_BLUE,1);
		else
		{
			sprintf(message, "Goes to : %dx%d (%dx%d)", teleporter->xpos,
			        teleporter->ypos, teleporter->xpos/GRID_SIZE, teleporter->ypos/GRID_SIZE);
		}
	}
	else
		sprintf(message, "Level   : %d", target->stats->level);
	infotext->write_xy(lm, INFO_DOWN(linesdown++), message, DARK_BLUE,1);

	linesdown++;
	infotext->write_xy(lm, INFO_DOWN(linesdown++),
	                   "PRESS ESC TO EXIT", DARK_BLUE,1);

	myscreen->buffer_to_screen(0, 0, 320, 200);
	grab_mouse();

	// Wait for press and release of ESC
	while (!mykeyboard[SDLK_ESCAPE])
		get_input_events(WAIT);
	while (mykeyboard[SDLK_ESCAPE])
		get_input_events(WAIT);
}

// Set the stats->name value of a walker ..
void set_name(walker  *target, screen * master)
{
	char newname[11];
	char oldname[20];
	char buffer[200];

	//gotoxy(1,20);
	master->draw_button(30, 30, 220, 70, 1, 1);
	sprintf(buffer, "Renaming object");
	scentext->write_xy(32, 32, buffer, DARK_BLUE, 1);
	sprintf(buffer, "Enter '.' to not change.");
	scentext->write_xy(32, 42, buffer, DARK_BLUE, 1);

	if (strlen(target->stats->name))
	{
		sprintf(buffer, "Current name: %s", target->stats->name);
		strcpy(oldname, target->stats->name);
	}
	else
	{
		sprintf(buffer, "Current name: NOT SET");
		strcpy(oldname, "NOT SET");
	}
	scentext->write_xy(32, 52, buffer, DARK_BLUE, 1);
	scentext->write_xy(32, 62, "    New name:", DARK_BLUE, 1);

	master->buffer_to_screen(0, 0, 320, 200);

	// wait for key release
	while (mykeyboard[SDLK_r])
		get_input_events(WAIT);

	strcpy(newname, scentext->input_string(115, 62, 9, oldname) );
	newname[10] = 0;

	if (strcmp(newname, ".")) // didn't type '.'
		strcpy(target->stats->name, newname);

	info_box(target,master);

}

void scenario_options(screen *myscreen)
{
	static text opt_text(myscreen);
	Uint8 *opt_keys = query_keyboard();
	short lm, tm;
	char message[80];

	lm = 55;
	tm = 45;

#define OPT_LD(x) (short) (tm + (x*8) )
while (!opt_keys[SDLK_ESCAPE])
        {


	myscreen->draw_button(lm-5, tm-5, 260, 160, 2, 1);

	opt_text.write_xy(lm, OPT_LD(0), "SCENARIO OPTIONS", DARK_BLUE, 1);

	if (myscreen->scenario_type & SCEN_TYPE_CAN_EXIT)
		opt_text.write_xy(lm, OPT_LD(2), "Can Always Exit (E)         : Yes", DARK_BLUE, 1);
	else
		opt_text.write_xy(lm, OPT_LD(2), "Can Always Exit (E)         : No ", DARK_BLUE, 1);

	if (myscreen->scenario_type & SCEN_TYPE_GEN_EXIT)
		opt_text.write_xy(lm, OPT_LD(3), " Kill Generators to Exit (G): Yes", DARK_BLUE, 1);
	else
		opt_text.write_xy(lm, OPT_LD(3), " Kill Generators to Exit (G): No ", DARK_BLUE, 1);

	if (myscreen->scenario_type & SCEN_TYPE_SAVE_ALL)
		opt_text.write_xy(lm, OPT_LD(4), " Must Save Named NPC's (N)  : Yes", DARK_BLUE, 1);
	else
		opt_text.write_xy(lm, OPT_LD(4), " Must Save Named NPC's (N)  : No ", DARK_BLUE, 1);

	sprintf(message, " Level Par Value (+,-)      : %d ", myscreen->par_value);
	opt_text.write_xy(lm, OPT_LD(5), message, DARK_BLUE, 1);


	myscreen->buffer_to_screen(0, 0, 320, 200);

	get_input_events(WAIT);
	if (opt_keys[SDLK_e]) // toggle exit mode
	{
		if (myscreen->scenario_type & SCEN_TYPE_CAN_EXIT) // already set
			myscreen->scenario_type -= SCEN_TYPE_CAN_EXIT;
		else
			myscreen->scenario_type += SCEN_TYPE_CAN_EXIT;
	}
	if (opt_keys[SDLK_g]) // toggle exit mode -- generators
	{
		if (myscreen->scenario_type & SCEN_TYPE_GEN_EXIT) // already set
			myscreen->scenario_type -= SCEN_TYPE_GEN_EXIT;
		else
			myscreen->scenario_type += SCEN_TYPE_GEN_EXIT;
	}
	if (opt_keys[SDLK_n]) // toggle fail mode -- named guys
	{
		if (myscreen->scenario_type & SCEN_TYPE_SAVE_ALL) // already set
			myscreen->scenario_type -= SCEN_TYPE_SAVE_ALL;
		else
			myscreen->scenario_type += SCEN_TYPE_SAVE_ALL;
	}
	if (opt_keys[SDLK_KP_MINUS]) // lower the par value
	{
		if (myscreen->par_value > 1)
			myscreen->par_value--;
	}
	if (opt_keys[SDLK_KP_PLUS]) // raise the par value
	{
		myscreen->par_value++;
	}
}

while (opt_keys[SDLK_ESCAPE])
	get_input_events(WAIT); // wait for key release

	myscreen->clearfontbuffer(lm-5, tm-5, 260-(lm-5), 160-(tm-5));
}

// Set an object's facing ..
void set_facing(walker *target, screen *myscreen)
{
	Uint8 *setkeys = query_keyboard();

	if (target)
		target = target;  // dummy code

	myscreen->draw_dialog(100, 50, 220, 170, "Set Facing");
	myscreen->buffer_to_screen(0, 0, 320, 200);

	while (setkeys[SDLK_f])
		get_input_events(WAIT);

}


// Load a grid or scenario ..
Sint32 do_load(screen *ascreen)
{
	Sint32 i;
	text *loadtext = new text(ascreen);
	char buffer[200],temp[200];

	event = 1;
	ascreen->draw_button(50, 30, 200, 40, 1, 1);
	loadtext->write_xy(52, 32, "Load [G/S] : ", DARK_BLUE, 1);
	ascreen->buffer_to_screen(0, 0, 320, 200);
	while ( !mykeyboard[SDLK_g] && !mykeyboard[SDLK_s] )
		get_input_events(WAIT);
	if (mykeyboard[SDLK_s])
	{
		ascreen->draw_button(50, 30, 200, 40, 1, 1);
		ascreen->buffer_to_screen(0, 0, 320, 200);
		new_scenario_name();
		ascreen->clearfontbuffer(50, 30, 150, 10);
		loadtext->write_xy(52, 32, "Loading scenario..", DARK_BLUE, 1);
		ascreen->buffer_to_screen(0, 0, 320, 200);
		remove_all_objects(ascreen);  // kill   current obs
		for (i=0; i < 60; i ++)
			ascreen->scentext[i][0] = 0;
		load_scenario(scen_name, ascreen);
		ascreen->viewob[0]->myradar->start();
		ascreen->viewob[0]->myradar->update();
		strcpy(grid_name, query_my_map_name());
		while (mykeyboard[SDLK_s])
			//buffers: dumbcount++;
			get_input_events(WAIT);
		//buffers: PORT: stricmp isn't compiling... need to find replacement func
		//buffers: workaround: copy scenario_title to new buffer and make it all
		//buffers: lowercase and then compare it to lowercase 'none'
		strcpy(temp,ascreen->scenario_title);
		lowercase(temp);
		if (strlen(ascreen->scenario_title) &&
		        strcmp(temp, "none") )
		{
			ascreen->draw_button(10, 30, 238, 51, 1, 1);
			ascreen->clearfontbuffer(10, 30, 228, 21);
			sprintf(buffer, "Loaded: %s", ascreen->scenario_title);
			loadtext->write_xy(12, 33, buffer, DARK_BLUE, 1);
			loadtext->write_xy(12, 43, "Press space to continue", RED, 1);
			ascreen->buffer_to_screen(0, 0, 320, 200);
			while (!mykeyboard[SDLK_SPACE])
				get_input_events(WAIT);
		}
	} // end load scenario
	else if (mykeyboard[SDLK_g])
	{
		ascreen->draw_button(50, 30, 200, 40, 1, 1);
		ascreen->buffer_to_screen(0, 0, 320, 200);
		load_new_grid(ascreen);
		while (mykeyboard[SDLK_g])
			//dumbcount++;
			get_input_events(WAIT);
	} // end load new grid

	delete loadtext;
	levelchanged = 0;
	return 1;
}
















#include "base.h"

class browse_radar
{
	public:
		browse_radar(short x, short y, screen * myscreen);
		~browse_radar();
		short draw ();
		short sizex, sizey;
		short xpos,ypos;
		short xloc, yloc;        // where on the screen to display
		// Zardus: radarx and radary are now class members (instead of temp vars) so that scen can use them for scroll
		short radarx, radary;	  // what actual portion of the map is on the radar (top-left coord)
		short on_screen();
		short on_screen(short whatx, short whaty, short hor, short ver);
		short refresh();
		void update(); // slow function to update radar/map info
		void start();

		screen * screenp;
		
		unsigned char  *bmp,  *oldbmp;
		short xview;
		short yview;
	protected:
		//         char  *buffer;
		short size;
};



#include "graph.h"
#include "colors.h"

#define RADAR_X 60  // These are the dimensions of the radar
#define RADAR_Y 44  // viewport

// ************************************************************
//  RADAR -- It's nothing like pixie, it just looks like it
// ************************************************************
/*
  radar(char,short,short,screen)    - initializes the radar data (pix = char)
  short draw()
  short on_screen()
*/


// Radar -- this initializes the graphics data for the radar,
// as well as its graphics x and y size.  In addition, it informs
// the radar of the screen object it is linked to.
browse_radar::browse_radar(short x, short y, screen * myscreen)
{
	screenp = myscreen;
	bmp = NULL;

	xloc = x;
	yloc = y;
}

void browse_radar::start()
{

	sizex = (unsigned short) screenp->maxx;
	sizey = (unsigned short) screenp->maxy;
	size = (unsigned short) (((unsigned short) sizex)*((unsigned short) sizey));
	xview = RADAR_X;
	yview = RADAR_Y;
	radarx = 0;
	radary = 0;

	if (xview > sizex)
		xview = sizex;
	if (yview > sizey)
		yview = sizey;
		
	bmp = new unsigned char[size];
	update();

}


// Destruct the radar and its variables
browse_radar::~browse_radar()
{
	if (bmp)
	{
		delete[] bmp;
		bmp = NULL;
	}
}

short browse_radar::draw()
{
	oblink  * here;
	Sint32 tempx, tempy, tempz;
	unsigned char tempcolor;
	short oborder, obfamily, obteam;
	short can_see = 0, do_show = 0;
	Sint32 listtype = 0;

	radarx = 0;
	radary = 0;

	if (!screenp)
		return 0; //shouldn't be needed????
    
    // Set viewing position
	{
		radarx = (short) (screenp->topx/GRID_SIZE - xview/2);
		radary = (short) (screenp->topy/GRID_SIZE - yview/2);
		obteam = 0;
	}
	if (radarx > (sizex - xview))
		radarx = (short) (sizex - xview);
	if (radary > (sizey - yview))
		radary = (short) (sizey - yview);
	if (radarx < 0)
		radarx = 0;
	if (radary < 0)
		radary = 0;
	screenp->putbuffer(xloc, yloc,
	                   sizex,sizey,
	                   xloc,yloc,xloc + xview,yloc + yview,
	                   &bmp[radarx + (radary * sizex)]);


	// Now determine what objects are visible on the radar ..
	while (listtype <= 1)
	{
		if (listtype == 0) // do oblist, standard
		{
			here = screenp->oblist;
			listtype++;
		}
		else if (listtype == 1) // do weapons
		{
			here = screenp->weaplist;
			listtype++;
		}
		else
			continue;

		while (here)
		{
			if (here->ob)
				oborder = here->ob->query_order();
			do_show = 0; // don't show, by default
			if (here->ob
			        && (oborder == ORDER_LIVING || oborder == ORDER_WEAPON
			            || (oborder == ORDER_TREASURE && (here->ob->query_family() == FAMILY_LIFE_GEM))
			            || (oborder == ORDER_TREASURE && (here->ob->query_family() == FAMILY_EXIT))
			            || (oborder == ORDER_GENERATOR && can_see)
			           )
			        && (obteam==here->ob->team_num || here->ob->invisibility_left < 1 || can_see)
			        && on_screen( (short) ((here->ob->xpos+1)/GRID_SIZE), (short) ((here->ob->ypos+1)/GRID_SIZE), radarx, radary)
			   )
				do_show = 1;
			if (do_show)
			{
				tempx = xloc + ((here->ob->xpos+1)/GRID_SIZE - radarx);
				tempy = yloc + ((here->ob->ypos+1)/GRID_SIZE - radary);
				if ( (tempx < xloc) || (tempx > (xloc + xview)) )
				{} //do nothing
				else if ( (tempy < yloc) || (tempy > (yloc + yview)) )
				{} //also do nothing
				else
				{
					tempz = (tempx+(tempy*320)); //this may need fixing
					if (tempz > 64000 || tempz < 0)
					{
						printf("bad radar, bad\n");
						return 1;
					}
					tempcolor = (here->ob->query_team_color());
					if (oborder == ORDER_LIVING)
						//buffers: PORT: screenp->videobuffer[tempz] = tempcolor;
						screenp->pointb(tempx,tempy,tempcolor);
					else if (oborder == ORDER_GENERATOR)
						//buffers: PORT: screenp->videobuffer[tempz] = (char) (tempcolor+1);
						screenp->pointb(tempx,tempy,(char)(tempcolor+1));
					else if (oborder == ORDER_TREASURE) // currently life gems
						//buffers: PORT: screenp->videobuffer[tempz] = COLOR_FIRE;
						screenp->pointb(tempx,tempy,COLOR_FIRE);
					else
						//buffers: PORT: screenp->videobuffer[tempz] = COLOR_WHITE;
						screenp->pointb(tempx,tempy,COLOR_WHITE);
				}//draw the blob onto the radar
			}
			here = here->next;
		}
	} // go back to new screen lists (weapons, etc.)

	here = screenp->fxlist;
	while (here)
	{
		if (here->ob && !here->ob->dead)
		{
			oborder  = here->ob->query_order();
			obfamily = here->ob->query_family();

			do_show = 0; // don't show, by default
			if (oborder == ORDER_TREASURE)
			{
				if (can_see)
				{
					switch (obfamily)
					{
						case FAMILY_GOLD_BAR:
							do_show = (short) (YELLOW + random(5));
							break;
						case FAMILY_SILVER_BAR:
							do_show = (short) (GREY + random(5));
							break;
						case FAMILY_DRUMSTICK:
							do_show = (short) (COLOR_BROWN + random(2));
							break;
						case FAMILY_MAGIC_POTION:
						case FAMILY_INVIS_POTION:
						case FAMILY_INVULNERABLE_POTION:
						case FAMILY_FLIGHT_POTION:
							do_show = (short) (COLOR_BLUE + random(5));
							break;
						default:
							do_show = 0;
							break;
					}
				}
				if (obfamily == FAMILY_EXIT || obfamily == FAMILY_TELEPORTER)
					do_show = (short) LIGHT_BLUE + random(7);
			}
			if (!on_screen( (short) ((here->ob->xpos+1)/GRID_SIZE),
			                (short) ((here->ob->ypos+1)/GRID_SIZE),
			                radarx, radary) )
				do_show = 0;
			if (do_show)
			{
				tempx = xloc + ((here->ob->xpos+1)/GRID_SIZE - radarx);
				tempy = yloc + ((here->ob->ypos+1)/GRID_SIZE - radary);
				if ( (tempx < xloc) || (tempx > (xloc + xview)) )
				{} //do nothing
				else if ( (tempy < yloc) || (tempy > (yloc + yview)) )
				{} //also do nothing
				else
				{
					tempz = (tempx+(tempy*320)); //this may need fixing
					if (tempz > 64000 || tempz < 0)
					{
						printf("bad radar, bad\n");
						return 1;
					}
					//buffers: PORT: screenp->videobuffer[tempz] = (char) do_show;
					screenp->pointb(tempx,tempy,(char)do_show);
				}//draw the blob onto the radar
			} // end of valid do_show
		}  // end of if here->ob
		here = here->next;
	} // end of while (here)

	return 1;
}

//short radar::refresh()
//{
// The first two values are screwy... I don't know why
// screenp->buffer_to_screen(xloc, yloc, xview, yview);
// return 1;
//}
// In theory the above function will NOT be required

short browse_radar::on_screen(short whatx, short whaty,
                       short hor, short ver)
{
	// Return 0 if off radar.
	// These measurements are grid coords, not pixels.
	if (whatx < hor || whatx >= (hor+xview) ||
	        whaty < ver || whaty >= (ver+yview) )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

// This function re-initializes the radar map data.  Do not
// call it often, as it is very slow ..
void browse_radar::update()
{
	short temp, i, j;

	for (i = 0; i < sizex; i++)
		for (j = 0; j < sizey; j++)
		{
			// Check if item in background grid
			switch ((unsigned char)screenp->grid[i+sizex*j])
			{
				case PIX_GRASS1:  // grass is green
				case PIX_GRASS_DARK_1:
				case PIX_GRASS_DARK_B1:
				case PIX_GRASS_DARK_BR:
					temp = COLOR_GREEN+3;
					break;
				case PIX_GRASS2:
				case PIX_GRASS_DARK_2:
				case PIX_GRASS_DARK_B2:
				case PIX_WALL_ARROW_GRASS:
					temp = COLOR_GREEN+4;
					break;
				case PIX_GRASS3:
				case PIX_GRASS_DARK_3:
				case PIX_GRASS_DARK_R1:
				case PIX_WALL_ARROW_GRASS_DARK:
					temp = COLOR_GREEN+5;
					break;
				case PIX_GRASS4:
				case PIX_GRASS_DARK_4:
				case PIX_GRASS_DARK_R2:
					temp = COLOR_GREEN+5;
					break;
				case PIX_GRASS_DARK_LL:
				case PIX_GRASS_DARK_UR:
				case PIX_GRASS_RUBBLE:
				case PIX_GRASS_LIGHT_1: // lighter grass
				case PIX_GRASS_LIGHT_TOP:
				case PIX_GRASS_LIGHT_RIGHT_TOP:
				case PIX_GRASS_LIGHT_RIGHT:
				case PIX_GRASS_LIGHT_RIGHT_BOTTOM:
				case PIX_GRASS_LIGHT_BOTTOM:
				case PIX_GRASS_LIGHT_LEFT_BOTTOM:
				case PIX_GRASS_LIGHT_LEFT:
				case PIX_GRASS_LIGHT_LEFT_TOP:
					temp = (short) (COLOR_GREEN + random(3) + 3);
					break;
				case PIX_TREE_M1: // Trees are green
				case PIX_TREE_ML:
				case PIX_TREE_T1:
				case PIX_TREE_MR:
				case PIX_TREE_MT:
					temp = (short) (COLOR_TREES + random(3));
					break;
				case PIX_TREE_B1: // Trunks are brown
					temp = COLOR_BROWN + 6;
					break;
				case PIX_PAVEMENT1:   // pavement dark grey
				case PIX_PAVEMENT2:
				case PIX_PAVEMENT3:
				case PIX_PAVESTEPS1:
				case PIX_PAVESTEPS2:
				case PIX_PAVESTEPS2L:
				case PIX_PAVESTEPS2R:
				case PIX_COBBLE_1:
				case PIX_COBBLE_2:
				case PIX_COBBLE_3:
				case PIX_COBBLE_4:
					temp = 17;
					break;
				case PIX_FLOOR_PAVEL: // wood is brown
				case PIX_FLOOR_PAVER:
				case PIX_FLOOR_PAVEU:
				case PIX_FLOOR_PAVED:
				case PIX_FLOOR1:
				case PIX_WALL_ARROW_FLOOR:
					temp = COLOR_BROWN+4;
					break;
				case PIX_DIRT_1: // path is brown
				case PIX_DIRTGRASS_UL1:
				case PIX_DIRTGRASS_UR1:
				case PIX_DIRTGRASS_LL1:
				case PIX_DIRTGRASS_LR1:
				case PIX_DIRT_DARK_1:
				case PIX_DIRTGRASS_DARK_UL1:
				case PIX_DIRTGRASS_DARK_UR1:
				case PIX_DIRTGRASS_DARK_LL1:
				case PIX_DIRTGRASS_DARK_LR1:
					temp = COLOR_BROWN+5;
					break;
				case PIX_JAGGED_GROUND_1:
				case PIX_JAGGED_GROUND_2:
				case PIX_JAGGED_GROUND_3:
				case PIX_JAGGED_GROUND_4:
					temp = COLOR_BROWN+5;
					break;
				case PIX_CLIFF_BOTTOM:  // slightly darker
				case PIX_CLIFF_TOP:
				case PIX_CLIFF_LEFT:
				case PIX_CLIFF_RIGHT:
				case PIX_CLIFF_BACK_1:
				case PIX_CLIFF_BACK_2:
				case PIX_CLIFF_BACK_L:
				case PIX_CLIFF_BACK_R:
				case PIX_CLIFF_TOP_L:
				case PIX_CLIFF_TOP_R:
					temp = COLOR_BROWN+6;
					break;
				case PIX_CARPET_LL:   // carpet is purple
				case PIX_CARPET_B:
				case PIX_CARPET_LR:
				case PIX_CARPET_UR:
				case PIX_CARPET_U:
				case PIX_CARPET_UL:
				case PIX_CARPET_L:
				case PIX_CARPET_M:
				case PIX_CARPET_M2:
				case PIX_CARPET_R:
				case PIX_CARPET_SMALL_HOR:
                case PIX_CARPET_SMALL_VER:
				case PIX_CARPET_SMALL_CUP:
				case PIX_CARPET_SMALL_CAP:
				case PIX_CARPET_SMALL_LEFT:
				case PIX_CARPET_SMALL_RIGHT:
				case PIX_CARPET_SMALL_TINY:
					temp = COLOR_PURPLE+4;
					break;
				case PIX_H_WALL1: // walls are light grey
				case PIX_WALL2:
				case PIX_WALL3:
				case PIX_WALL_LL:
				case PIX_WALLTOP_H:
				case PIX_WALL4:
				case PIX_WALL5:
				case PIX_BOULDER_1:
				case PIX_BOULDER_2:
				case PIX_BOULDER_3:
				case PIX_BOULDER_4:
				case PIX_PATH_1:      // sparser cobblestone/grass
				case PIX_PATH_2:
				case PIX_PATH_3:
				case PIX_PATH_4:
					temp = 24;
					break;
				case PIX_WATER1:      // Water is dark blue
				case PIX_WATER2:
				case PIX_WATER3:
				case PIX_WATERGRASS_LL:
				case PIX_WATERGRASS_LR:
				case PIX_WATERGRASS_UL:
				case PIX_WATERGRASS_UR:
				case PIX_GRASSWATER_LL:
				case PIX_GRASSWATER_LR:
				case PIX_GRASSWATER_UL:
				case PIX_GRASSWATER_UR:
					temp = COLOR_BLUE+2;
					break;

				case PIX_WALLSIDE_L:  // White, maybe?
				case PIX_WALLSIDE1:
				case PIX_WALLSIDE_R:
				case PIX_WALLSIDE_C:
				case PIX_WALLSIDE_CRACK_C1:
					temp = COLOR_WHITE-1;
					break;

				case PIX_TORCH1:
				case PIX_TORCH2:
				case PIX_TORCH3:
				case PIX_BRAZIER1:
					temp = COLOR_FIRE;
					break;

				default:
					temp =  0;
			}
			bmp[i+sizex*j] = (unsigned char) temp;
		}

}














#define NUM_BROWSE_RADARS 6

// Load a grid or scenario ..
Sint32 browse(screen *screenp)
{
    remove_all_objects(screenp);  // kill current obs
    for (int j=0; j < 60; j++)
        screenp->scentext[j][0] = 0;
        
    SDL_Rect mapAreas[NUM_BROWSE_RADARS];
    browse_radar* radars[NUM_BROWSE_RADARS];
    oblink  *oblist[NUM_BROWSE_RADARS];
    oblink  *fxlist[NUM_BROWSE_RADARS];  // fx--explosions, etc.
    oblink  *weaplist[NUM_BROWSE_RADARS];  // weapons
    
    for(int i = 0; i < NUM_BROWSE_RADARS; i++)
    {
        mapAreas[i].x = 10 + (screenp->maxx + 10)*i;
        mapAreas[i].y = 20;
        mapAreas[i].w = screenp->maxx;
        mapAreas[i].h = screenp->maxy;
        
        char buf[20];
        snprintf(buf, 20, "scen%d", i);
        load_scenario(buf, screenp);
        
        browse_radar* r = new browse_radar(mapAreas[i].x, mapAreas[i].y, screenp);
        r->start();
        radars[i] = r;
        oblist[i] = screenp->oblist;
        screenp->oblist = NULL;
        fxlist[i] = screenp->fxlist;
        screenp->fxlist = NULL;
        weaplist[i] = screenp->weaplist;
        screenp->weaplist = NULL;
        
		remove_all_objects(screenp);  // kill current obs
		for (int j=0; j < 60; j++)
			screenp->scentext[j][0] = 0;
    }
    
    bool done = false;
	while (!done)
	{
		// Reset the timer count to zero ...
		reset_timer();

		if (screenp->end)
			break;

		//buffers: get keys and stuff
		get_input_events(POLL);
		
		// Wait for a key to be pressed ..
		if(mykeyboard[SDLK_q])
            done = true;
        
        screenp->clearscreen();
        
        for(int i = 0; i < NUM_BROWSE_RADARS; i++)
        {
            screenp->oblist = oblist[i];
            screenp->fxlist = fxlist[i];
            screenp->weaplist = weaplist[i];
            radars[i]->draw();
        }
        
        //screenp->draw_box(x, y, x+maxlength*(sizex+1), y+sizey+1, backcolor, 1, 1);
		//write_xy(x, y, editstring, forecolor, 1);
		screenp->buffer_to_screen(0, 0, 320, 200);
	}
	
    while (mykeyboard[SDLK_q])
        get_input_events(WAIT);
	
    for(int i = 0; i < NUM_BROWSE_RADARS; i++)
    {
        screenp->oblist = oblist[i];
        screenp->fxlist = fxlist[i];
        screenp->weaplist = weaplist[i];
		remove_all_objects(screenp);  // kill current obs
        delete radars[i];
    }
    
    load_scenario("test", screenp);
    
	return 1;
}





