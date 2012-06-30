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
#include "graph.h"
#include "smooth.h"
#include "util.h"

short next_scenario = 1;
FILE * open_misc_file(const char *, const char *);
FILE * open_misc_file(const char *, const char *, const char *);

short load_saved_game(const char *filename, screen  *myscreen)
{
	char          scenfile[20];
	guy           *temp_guy;
	walker        *temp_walker,  *replace_walker;
	oblink        * here;
	short         myord, myfam;
	int           multi_team = 0;
	int           i;

	// First load the team list ..
	if (!load_team_list(filename, myscreen))
	{
		printf("Error loading saved game %s.\n", filename);
		release_keyboard();
		exit(0);
	}

	// Determine the scenario name to load
	sprintf(scenfile, "scen%d", next_scenario);
	// And our default par value ..
	myscreen->par_value = next_scenario;
	// And load the scenario ..
	if (!load_scenario(scenfile, myscreen))
	{
		myscreen->par_value = 1;
		load_scenario("scen0", myscreen);
		myscreen->scen_num = 0;
		mysmoother->set_target(myscreen);
	}
	mysmoother->set_target(myscreen);

	here = myscreen->oblist;
	while (here)
	{
		if (here->ob)
			here->ob->set_difficulty((Uint32)here->ob->stats->level);
		here = here->next;
	}

	// Cycle through the team list ..
	temp_guy = myscreen->first_guy;
	while (temp_guy)
	{
		temp_walker = myscreen->add_ob(ORDER_LIVING, temp_guy->family);
		temp_walker->myguy = temp_guy;
		temp_walker->stats->level = temp_guy->level;

		// Set hitpoints based on stats:
		temp_walker->stats->max_hitpoints = (short)
		                                    (10 + (temp_guy->constitution*3 +
		                                           ((temp_guy->strength)/2) + (short) (25*temp_guy->level))  );
		temp_walker->stats->hitpoints = temp_walker->stats->max_hitpoints;

		// Set damage based on strength and level
		temp_walker->damage += (temp_guy->strength/4) + temp_guy->level
		                       + (temp_guy->dexterity/11);
		// Set magicpoints based on stats:
		temp_walker->stats->max_magicpoints = (short)
		                                      (10 + (temp_guy->intelligence*3) + ( 25 * temp_guy->level) +(short) (temp_guy->dexterity) );
		temp_walker->stats->magicpoints = temp_walker->stats->max_magicpoints;

		// Set our armor level ..
		temp_walker->stats->armor =(short)  ( temp_guy->armor + (temp_guy->dexterity / 14) + temp_guy->level );

		// Set the heal delay ..

		temp_walker->stats->max_heal_delay = REGEN;
		temp_walker->stats->current_heal_delay =
		    (temp_guy->constitution) + (temp_guy->strength/6) +
		    (temp_guy->level * 2) + 20; //for purposes of calculation only

		while (temp_walker->stats->current_heal_delay > REGEN)
		{
			temp_walker->stats->current_heal_delay -= REGEN;
			temp_walker->stats->heal_per_round++;
		} // this takes care of the integer part, now calculate the fraction

		if (temp_walker->stats->current_heal_delay > 1)
		{
			temp_walker->stats->max_heal_delay /=
			    (Sint32) (temp_walker->stats->current_heal_delay + 1);
		}
		temp_walker->stats->current_heal_delay = 0; //start off without healing

		//make sure we have at least a 2 wait, otherwise we should have
		//calculated our heal_per_round as one higher, and the math must
		//have been screwed up some how
		if (temp_walker->stats->max_heal_delay < 2)
			temp_walker->stats->max_heal_delay = 2;

		// Set the magic delay ..
		temp_walker->stats->max_magic_delay = REGEN;
		temp_walker->stats->current_magic_delay = (Sint32)
		        (temp_guy->intelligence * 45) + (temp_guy->level*60) +
		        (temp_guy->dexterity * 15) + 200;

		while (temp_walker->stats->current_magic_delay > REGEN)
		{
			temp_walker->stats->current_magic_delay -= REGEN;
			temp_walker->stats->magic_per_round++;
		} // this takes care of the integer part, now calculate the fraction

		if (temp_walker->stats->current_magic_delay > 1)
		{
			temp_walker->stats->max_magic_delay /=
			    (Sint32) (temp_walker->stats->current_magic_delay + 1);
		}
		temp_walker->stats->current_magic_delay = 0; //start off without magic regen

		//make sure we have at least a 2 wait, otherwise we should have
		//calculated our magic_per_round as one higher, and the math must
		//have been screwed up some how
		if (temp_walker->stats->max_magic_delay < 2)
			temp_walker->stats->max_magic_delay = 2;

		//stepsize makes us run faster, max for a non-weapon is 12
		temp_walker->stepsize += (temp_guy->dexterity/54);
		if (temp_walker->stepsize > 12)
			temp_walker->stepsize = 12;
		temp_walker->normal_stepsize = temp_walker->stepsize;

		//fire_frequency makes us fire faster, min is 1
		temp_walker->fire_frequency -= (temp_guy->dexterity / 47);
		if (temp_walker->fire_frequency < 1)
			temp_walker->fire_frequency = 1;

		// Fighters: limited weapons
		if (temp_walker->query_family() == FAMILY_SOLDIER)
			temp_walker->weapons_left = (short) ((temp_walker->stats->level+1) / 2);

		// Set our team number ..
		temp_walker->team_num = temp_guy->teamnum;
		temp_walker->real_team_num = 255;

		// Do we have guys on multiple teams? If so, we need
		// to record it so that we can set the controls of
		// the viewscreens correctly
		if (temp_guy->teamnum != 0)
			multi_team = 1;

		// First, try to find a marker that's the correct team number ..
		replace_walker = myscreen->first_of(ORDER_SPECIAL,
		                                    FAMILY_RESERVED_TEAM,
		                                    (int)temp_guy->teamnum);
		// If that doesn't work, though, grab any marker we can ..
		if (!replace_walker)
			replace_walker = myscreen->first_of(ORDER_SPECIAL, FAMILY_RESERVED_TEAM);
		if (replace_walker)
		{
			temp_walker->setxy(replace_walker->xpos, replace_walker->ypos);
			replace_walker->dead = 1;
			temp_guy = temp_guy->next;
		}
		else
		{
			// Scatter the overflowing characters..
			temp_walker->teleport();
			temp_guy = temp_guy->next;
		}

	}

	// Now remove any extra guys .. (set to dead)
	replace_walker = myscreen->first_of(ORDER_SPECIAL, FAMILY_RESERVED_TEAM);
	while (replace_walker)
	{
		replace_walker->dead = 1;
		replace_walker = myscreen->first_of(ORDER_SPECIAL, FAMILY_RESERVED_TEAM);
	}
	// Remove the links between 'guys'
	here = myscreen->oblist;
	while (here)
	{
		if (here->ob && here->ob->myguy)
			here->ob->myguy->next = NULL;
		here = here->next;
	}

	// Have we already done this scenario?
	if (myscreen->levelstatus[myscreen->scen_num])
	{
		//                printf("already done level\n");
		here = myscreen->oblist;
		while (here)
		{
			if (here->ob)
			{
				myfam = here->ob->query_family();
				myord = here->ob->query_order();
				if ( ( (here->ob->team_num==0 || here->ob->myguy) && myord==ORDER_LIVING) || //living team member
				        (myord==ORDER_TREASURE && myfam==FAMILY_EXIT) || // exit
				        (myord==ORDER_TREASURE && myfam==FAMILY_TELEPORTER)  // teleporters
				   )
				{
					// do nothing; legal guy
				}
				else
					here->ob->dead = 1;
			}
			here = here->next;
		}
		here = myscreen->weaplist;
		while (here)
		{
			if (here->ob)
			{
				myfam = here->ob->query_family();
				myord = here->ob->query_order();
				if ( (here->ob->team_num==0 && myord==ORDER_LIVING) || //living team member
				        (myord==ORDER_TREASURE && myfam==FAMILY_EXIT) || // exit
				        (myord==ORDER_TREASURE && myfam==FAMILY_TELEPORTER)  // teleporters

				   )
				{
					// do nothing; legal guy
				}
				else
					here->ob->dead = 1;
			}
			here = here->next;
		}

		here = myscreen->fxlist;
		while (here)
		{
			if (here->ob)
			{
				myfam = here->ob->query_family();
				myord = here->ob->query_order();
				if ( (here->ob->team_num==0 && myord==ORDER_LIVING) || //living team member
				        (myord==ORDER_TREASURE && myfam==FAMILY_EXIT) || // exit
				        (myord==ORDER_TREASURE && myfam==FAMILY_TELEPORTER)  // teleporters

				   )
				{
					// do nothing; legal guy
				}
				else
					here->ob->dead = 1;
			}
			here = here->next;
		}
	}

	// Here we decide if all players are controlling
	// team 0, or if they're playing competing teams ..
	if (multi_team)
	{
		for (i=0; i < myscreen->numviews; i++)
			myscreen->viewob[i]->my_team = i;
	}
	else
	{
		for (i=0; i < myscreen->numviews; i++)
			myscreen->viewob[i]->my_team = 0;
	}

	return 1;
}

short load_team_list(const char * filename, screen  *myscreen)
{
	char filler[50] = "GTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTL"; // for RESERVED
	FILE  *infile;
	char temp_filename[80];
	guy  *temp_guy;

	char temptext[10] = "GTL";
	char savedgame[40];
	char temp_version = 7;
	Uint32 newcash;
	Uint32 newscore = 0;
	//  short numguys;
	short listsize;
	short i;

	char tempname[12] = "FRED";
	char guyname[12] = "JOE";
	char temp_order, temp_family;
	short temp_str, temp_dex, temp_con;
	short temp_short, temp_arm, temp_lev;
	char numplayers;
	Uint32 temp_exp;
	short temp_kills;
	Sint32 temp_level_kills;
	Sint32 temp_td, temp_th, temp_ts;
	short temp_teamnum; // version 5+
	short temp_allied;            // v.7+
	short temp_registered;        // v.7+

	// Format of a team list file is:
	// 3-byte header: 'GTL'
	// 1-byte version number
	// 2-bytes registered mark            // Versions 7+
	// 40-bytes saved game name, version 2 and up
	// 2-bytes (short) = scenario number
	// 4-bytes (Sint32)= cash (unsigned)
	// 4-bytes (Sint32)= score (unsigned)
	// 4-bytes (Sint32)= cash-B (unsigned)   // All alternate scores
	// 4-bytes (Sint32)= score-B (unsigned)  // version 6+
	// 4-bytes (Sint32)= cash-C (unsigned)
	// 4-bytes (Sint32)= score-C (unsigned)
	// 4-bytes (Sint32)= cash-D (unsigned)
	// 4-bytes (Sint32)= score-D (unsigned)
	// 2-bytes Allied mode                // Versions 7+
	// 2-bytes (short) = # of team members in list
	// 1-byte number of players
	// 31-bytes RESERVED
	// List of n objects, each of 58-bytes of form:
	// 1-byte ORDER
	// 1-byte FAMILY
	// 12-byte name
	// 2-bytes strength
	// 2-bytes dexterity
	// 2-bytes constitution
	// 2-bytes intelligence
	// 2-bytes armor
	// 2-bytes level
	// 4-bytes unsigned experience
	// 2-bytes # kills, v.3
	// 4-bytes # total levels killed, v.3
	// 4-bytes total damage delt, v.4+
	// 4-bytes total hits inflicted, v.4+
	// 4-bytes total shots made, v.4+
	// 2-bytes team number
	// 2*4 = 8 bytes RESERVED
	// List of 200 or 500 (max levels) 1-byte scenario-level status

	//strcpy(temp_filename, scen_directory);
	strcpy(temp_filename, filename);
	strcat(temp_filename, ".gtl"); // gladiator team list

	if ( (infile = open_misc_file(temp_filename, "save/")) == NULL )
	{
		//gotoxy(1, 22);
		//printf("Error in opening team file: %s\n", filename);
		return 0;
	}

	// Read id header
	fread(temptext, 3, 1, infile);
	if ( strcmp(temptext,"GTL"))
	{
		fclose(infile);
		printf("Error, selected file is not a GTL file: %s\n",filename);
		return 0; //not a gtl file
	}

	// Read version number
	fread(&temp_version, 1, 1, infile);

	// Versions 7+ have a registered mark ..
	if (temp_version >= 7)
	{
		fread(&temp_registered, 2, 1, infile);
	}

	// Do other stuff based on version ..
	if (temp_version != 1)
	{
		if (temp_version >= 2)
			fread(savedgame, 40, 1, infile); // read and ignore the name
		else
		{
			fclose(infile);
			printf("Error, selected files is not version one: %s\n",filename);
			return 0;
		}
	}

	// Read scenario number
	fread(&next_scenario, 2, 1, infile);
	myscreen->scen_num = next_scenario;

	// Read cash
	fread(&newcash, 4, 1, infile);
	myscreen->totalcash = newcash;
	// Read score
	fread(&newscore, 4, 1, infile);
	myscreen->totalscore = newscore;

	// Versions 6+ have a score for each possible team, 0-3
	if (temp_version >= 6)
	{
		for (i=0; i < 4; i++)
		{
			fread(&newcash, 4, 1, infile);
			myscreen->m_totalcash[i] = newcash;
			fread(&newscore, 4, 1, infile);
			myscreen->m_totalscore[i] = newscore;
		}
	}

	// Versions 7+ have the allied information ..
	if (temp_version >= 7)
	{
		fread(&temp_allied, 2, 1, infile);
		myscreen->allied_mode = temp_allied;
	}

	// Get # of guys to read
	fread(&listsize, 2, 1, infile);

	// Read (and ignore) the # of players
	fread(&numplayers, 1, 1, infile);

	// Read the reserved area, 31 bytes
	fread(filler, 31, 1, infile);

	// Okay, we've read header .. now read the team list data ..
	if (myscreen->first_guy)
	{
		delete myscreen->first_guy;  // delete the old list of guys
		myscreen->first_guy = NULL;
	}

	// Make a new 'head' guy ..
	myscreen->first_guy = new guy();
	temp_guy = myscreen->first_guy;
	while (listsize--)
	{
		// Get temp values to be read
		temp_order = ORDER_LIVING; // may be changed later
		// Read name of current guy...
		strcpy(guyname, tempname);
		// Set any chars under 12 not used to 0 ..
		for (i=(short) strlen(guyname); i < 12; i++)
			guyname[i] = 0;
		// Now write all those values
		fread(&temp_order, 1, 1, infile);
		fread(&temp_family,1, 1, infile);
		fread(guyname, 12, 1, infile);
		fread(&temp_str, 2, 1, infile);
		fread(&temp_dex, 2, 1, infile);
		fread(&temp_con, 2, 1, infile);
		fread(&temp_short, 2, 1, infile);
		fread(&temp_arm, 2, 1, infile);
		fread(&temp_lev, 2, 1, infile);
		fread(&temp_exp, 4, 1, infile);
		// Below here is version 3 and up..
		fread(&temp_kills, 2, 1, infile); // how many kills we have
		fread(&temp_level_kills, 4, 1, infile); // levels of kills
		// Below here is version 4 and up ..
		fread(&temp_td, 4, 1, infile); // total damage
		fread(&temp_th, 4, 1, infile); // total hits
		fread(&temp_ts, 4, 1, infile); // total shots
		fread(&temp_teamnum, 2, 1, infile); // team number

		// And the filler
		fread(filler, 8, 1, infile);
		// Now set the values ..
		temp_guy->family       = temp_family;
		strcpy(temp_guy->name,guyname);
		temp_guy->strength     = temp_str;
		temp_guy->dexterity    = temp_dex;
		temp_guy->constitution = temp_con;
		temp_guy->intelligence = temp_short;
		temp_guy->armor        = temp_arm;
		temp_guy->level        = temp_lev;
		temp_guy->exp          = temp_exp;
		if (temp_version >=3)
		{
			temp_guy->kills      = temp_kills;
			temp_guy->level_kills= temp_level_kills;
		}
		else // version 2 or earlier
		{
			temp_guy->kills      = 0;
			temp_guy->level_kills= 0;
		}
		if (temp_version >= 4)
		{
			temp_guy->total_damage = temp_td;
			temp_guy->total_hits   = temp_th;
			temp_guy->total_shots  = temp_ts;
		}
		else
		{
			temp_guy->total_damage = 0;
			temp_guy->total_hits   = 0;
			temp_guy->total_shots  = 0;
		}
		if (temp_version >= 5)
		{
			temp_guy->teamnum = temp_teamnum;
		}
		else
		{
			temp_guy->teamnum = 0;
		}

		// Advance to the next guy ..
		if (listsize)
		{
			temp_guy->next = new guy();
			temp_guy = temp_guy->next;
		}
	}

	if (temp_version >= 5)
		fread( (myscreen->levelstatus), 500, 1, infile);
	else
	{
		memset( (myscreen->levelstatus), 0, 500);
		fread((myscreen->levelstatus), 200, 1, infile);
	}

	fclose(infile);

	return 1;
}

short save_game(const char * filename, screen  *myscreen)
{
	char filler[50] = "GTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTLGTL"; // for RESERVED
	FILE  *outfile;
	char temp_filename[80];
	oblink  *here = myscreen->oblist;
	walker  * temp_walker;
	char savedgame[40];

	char temptext[10] = "GTL";
	char temp_version = 7;
	short next_scenario = (short) ( myscreen->scen_num + 1 );
	Uint32 newcash = myscreen->totalcash;
	Uint32 newscore = myscreen->totalscore;
	//  short numguys;
	short listsize;
	short i;

	char guyname[12] = "JOE";
	char temp_order, temp_family;
	short temp_str, temp_dex, temp_con;
	short temp_short, temp_arm, temp_lev;
	char numplayers = (char) myscreen->numviews;
	Uint32 temp_exp;
	short temp_kills;
	Sint32 temp_level_kills;
	Sint32 temp_td, temp_th, temp_ts;
	short temp_teamnum;
	short temp_allied;
	short temp_registered;


	// Format of a team list file is:
	// 3-byte header: 'GTL'
	// 1-byte version number
	// 2-bytes Registered or not          // Version 7+
	// 40-bytes saved-game name, dummy here
	// 2-bytes (short) = scenario number
	// 4-bytes (Sint32)= cash (unsigned)
	// 4-bytes (Sint32)= score (unsigned)
	// 4-bytes (Sint32)= cash-B (unsigned)   // All alternate scores
	// 4-bytes (Sint32)= score-B (unsigned)  // version 6+
	// 4-bytes (Sint32)= cash-C (unsigned)
	// 4-bytes (Sint32)= score-C (unsigned)
	// 4-bytes (Sint32)= cash-D (unsigned)
	// 4-bytes (Sint32)= score-D (unsigned)
	// 2-bytes allied setting              // Version 7+
	// 2-bytes (short) = # of team members in list
	// 1-byte number of players
	// 31-bytes RESERVED
	// List of n objects, each of 58-bytes of form:
	// 1-byte ORDER
	// 1-byte FAMILY
	// 12-byte name
	// 2-bytes strength
	// 2-bytes dexterity
	// 2-bytes constitution
	// 2-bytes intelligence
	// 2-bytes armor
	// 2-bytes level
	// 4-bytes Uint32 experience
	// 2-bytes # kills, v.3+
	// 4-bytes # total levels killed, v.3+
	// 4-bytes total damage delt, v.4+
	// 4-bytes total hits inflicted, v.4+
	// 4-bytes total shots made, v.4+
	// 2-bytes team number, v.5+
	// 2*4 = 8 bytes RESERVED
	// List of 500 (max scenarios) 1-byte scenario-level status

	//strcpy(temp_filename, scen_directory);
	strcpy(temp_filename, filename);
	strcat(temp_filename, ".gtl"); // gladiator team list

	if ( (outfile = open_misc_file(temp_filename, "save/", "wb")) == NULL ) // open for write
	{
		//gotoxy(1, 22);
		printf("Error in writing team file %s\n", filename);
		return 0;
	}

	// Write id header
	fwrite(temptext, 3, 1, outfile);

	// Write version number
	fwrite(&temp_version, 1, 1, outfile);

	// Versions 7+ include a mark for registered or not
	temp_registered = 1;
	fwrite(&temp_registered, 2, 1, outfile);

	// Write the name
	fwrite(savedgame, 40, 1, outfile);

	// Write scenario number
	fwrite(&next_scenario, 2, 1, outfile);

	// Write cash
	fwrite(&newcash, 4, 1, outfile);
	// Write score
	fwrite(&newscore, 4, 1, outfile);

	// Versions 6+ have a score for each possible team
	for (i=0; i < 4; i++)
	{
		newcash = myscreen->m_totalcash[i];
		fwrite(&newcash, 4, 1, outfile);
		newscore = myscreen->m_totalscore[i];
		fwrite(&newscore, 4, 1, outfile);
	}

	// Versions 7+ include the allied mode information
	temp_allied = myscreen->allied_mode;
	fwrite(&temp_allied, 2, 1, outfile);

	// Determine size of team list ...
	listsize = 0;
	while (here)
	{
		if (here->ob && !here->ob->dead && here->ob->myguy)
			//if (here->ob && !here->ob->dead && here->ob->myguy &&
			//    (here->ob->real_team_num==0 || (here->ob->real_team_num==255
			//                                    && here->ob->team_num==0)
			//    )
			//   )
			listsize++;
		here = here->next;
	}

	//gotoxy(1, 22);
	//printf("Team size: %d  ", listsize);
	fwrite(&listsize, 2, 1, outfile);

	fwrite(&numplayers, 1, 1, outfile);

	// Write the reserved area, 31 bytes
	fwrite(filler, 31, 1, outfile);

	// Okay, we've written header .. now dump the data ..
	here = myscreen->oblist;  // back to head of list
	while (here)
	{
		if (here->ob && !here->ob->dead && here->ob->myguy)
		{
			temp_walker = here->ob;

			// Get temp values to be saved
			temp_order = temp_walker->query_order(); // may be changed later
			temp_family= temp_walker->query_family();
			// Write name of current guy...
			strcpy(guyname, temp_walker->myguy->name);
			// Set any chars under 12 not used to 0 ..
			for (i=(short) strlen(guyname); i < 12; i++)
				guyname[i] = 0;
			temp_str = temp_walker->myguy->strength;
			temp_dex = temp_walker->myguy->dexterity;
			temp_con = temp_walker->myguy->constitution;
			temp_short = temp_walker->myguy->intelligence;
			temp_arm = temp_walker->myguy->armor;
			temp_lev = temp_walker->myguy->level;
			temp_exp = temp_walker->myguy->exp;
			// Version 3+ below here
			temp_kills = temp_walker->myguy->kills;
			temp_level_kills = temp_walker->myguy->level_kills;
			// Version 4+ below here
			temp_td = temp_walker->myguy->total_damage;
			temp_th = temp_walker->myguy->total_hits;
			temp_ts = temp_walker->myguy->total_shots;

			// Version 5+ below here
			temp_teamnum = temp_walker->myguy->teamnum;

			// Now write all those values
			fwrite(&temp_order, 1, 1, outfile);
			fwrite(&temp_family,1, 1, outfile);
			fwrite(guyname, 12, 1, outfile);
			fwrite(&temp_str, 2, 1, outfile);
			fwrite(&temp_dex, 2, 1, outfile);
			fwrite(&temp_con, 2, 1, outfile);
			fwrite(&temp_short, 2, 1, outfile);
			fwrite(&temp_arm, 2, 1, outfile);
			fwrite(&temp_lev, 2, 1, outfile);
			fwrite(&temp_exp, 4, 1, outfile);
			fwrite(&temp_kills, 2, 1, outfile);
			fwrite(&temp_level_kills, 4, 1, outfile);
			fwrite(&temp_td, 4, 1, outfile);
			fwrite(&temp_th, 4, 1, outfile);
			fwrite(&temp_ts, 4, 1, outfile);
			fwrite(&temp_teamnum, 2, 1, outfile);
			// And the filler
			fwrite(filler, 8, 1, outfile);
		}
		// Advance to the next guy ..
		here = here->next;
	}

	// Write the level status ..
	fwrite((myscreen->levelstatus), 500, 1, outfile);

	fclose(outfile);

	return 1;
}

