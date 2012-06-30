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
//
// util.cpp
//
// random helper functions
//
#include "util.h"

// Notice: If config.h does not exist, copy src/config.h-<platform> to config.h or run autoconf.
#include "config.h"

#include <stdio.h>
#include <time.h>
#include <string.h> //buffers: for strlen
#include <string>
#include <sys/stat.h>
#include "base.h"

using namespace std;

Uint32 start_time=0;
Uint32 reset_value=0;

void change_time(Uint32 new_count)
{}

void grab_timer()
{}

void release_timer()
{}

void reset_timer()
{
	reset_value = SDL_GetTicks();
}

Sint32 query_timer()
{
	// Zardus: why 13.6? With DOS timing, you had to divide 1,193,180 by the desired frequency and
	// that would return ticks / second. Gladiator used to use a frequency of 65536/4 ticks per hour,
	// or 1193180/16383 = 72.3 ticks per second. This translates into 13.6 milliseconds / tick
	return (Sint32) ((SDL_GetTicks() - reset_value) / 13.6);
}

Sint32 query_timer_control()
{
	return (Sint32) (SDL_GetTicks() / 13.6);
}

void time_delay(Sint32 delay)
{
	if (delay < 0) return;
	SDL_Delay((Uint32) (delay * 13.6));
}

void lowercase(char * str)
{
	unsigned int i;
	for (i = 0; i < strlen(str);i++)
		str[i] = tolower(str[i]);
}

//buffers: add: another extra routine.
void uppercase(char *str)
{
	unsigned int i;
	for(i=0;i<strlen(str);i++)
		str[i] = toupper(str[i]);
}

// kari: yet two extra
void lowercase(std::string &str)
{
	for(std::string::iterator iter = str.begin(); iter!=str.end(); ++iter)
		*iter = tolower(*iter);
}

void uppercase(std::string &str)
{
	for(std::string::iterator iter = str.begin(); iter!=str.end(); ++iter)
		*iter = toupper(*iter);
}

FILE * open_misc_file(const char * file, const char * pos_dir, const char * attr)
{
	FILE * infile;
	char * filename = get_file_path(file, pos_dir, attr);

	if (filename && (infile = fopen(filename, attr)))
	{
		free(filename);
                return infile;
	}

	// if it got here, it didn't find the file
	return NULL;
}

FILE * open_misc_file(const char * file, const char * pos_dir)
{
	return open_misc_file(file, pos_dir, "rb");
}

FILE * open_misc_file(const char * file)
{
	return open_misc_file(file, "", "rb");
}

void create_dataopenglad()
{
#ifndef WINDOWS
	string path(getenv("HOME"));
	path += "/.openglad/";
	mkdir(path.c_str(), 0755);
	path.reserve(path.size()+10);
	string::iterator subdirpos = path.end();
	path += "pix/";
	mkdir(path.c_str(), 0755);
	path.replace(subdirpos, path.end(), "scen/", 4);
	mkdir(path.c_str(), 0755);
	path.replace(subdirpos, path.end(), "save/", 5);
	mkdir(path.c_str(), 0755);
	path.replace(subdirpos, path.end(), "sound/", 5);
	mkdir(path.c_str(), 0755);
#endif
}

char * get_file_path(const char * file, const char * pos_dir, const char * attr)
{
	FILE * infile;
	string filepath(file);

#ifndef	WINDOWS
	filepath = getenv("HOME");
	filepath += "/.openglad/";
	filepath += pos_dir;
	filepath += file;

	if ((infile = fopen(filepath.c_str(), attr)))
	{
		fclose(infile);
		return strdup(filepath.c_str());
	}
#endif

	// Lets try the datadir option now.
	if (cfg.query("dirs", "data"))
	{
		filepath = cfg.query("dirs", "data");
		filepath += pos_dir;
		filepath += file;

		if ((infile = fopen(filepath.c_str(), attr)))
		{
			fclose(infile);
			return strdup(filepath.c_str());
		}
	}
    
	filepath = DATADIR;
	filepath += pos_dir;
	filepath += file;

	if ((infile = fopen(filepath.c_str(), attr)))
	{
		fclose(infile);
		return strdup(filepath.c_str());
	}

	// as a last resort, look in ./posdir/file
	filepath = pos_dir;
	filepath += file;
	if ((infile = fopen(filepath.c_str(), attr)))
	{
		fclose (infile);
		return strdup(filepath.c_str());
	}

	// if it got here, it didn't find the file
	return NULL;
}

char * get_file_path(const char * file, const char * pos_dir)
{
	return get_file_path(file, pos_dir, "rb");
}

char * get_file_path(const char * file)
{
	return get_file_path(file, "", "rb");
}
