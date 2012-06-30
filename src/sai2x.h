#ifndef MOM_SCREEN
#define MOM_SCREEN

#include <SDL.h>

//#include "global.h"

extern SDL_Surface *screen;

typedef enum 
{
	NoZoom = 0x01,
	SAI = 0x02,
	EAGLE = 0x03,
	DOUBLE = 0x04
} RenderEngine;

class Screen
{
	public:
		SDL_Surface *screen;
	private:
		SDL_Surface		*render;		// the physical screen
		SDL_Surface		*tempo;			// used to render org_screen before
										// update which is only a SDL_Update between
										// render and tempo
		RenderEngine	Engine;			// how to render the physical screen

	protected:
	public:
		Screen( RenderEngine engine, int fullscreen );

		void Quit();

		~Screen();

		void SaveBMP( char *filename );

		/*void Blit( SDL_Surface *src, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y );
	
		void Blit( SDL_Surface *src, int dst_x, int dst_y);*/

		void Update();

		void Update( int x, int y, int w, int h );

		void Update( SDL_Rect &r_upd );

		SDL_Surface *RenderAndReturn( int x, int y, int w, int h );

		void Swap(int x, int y, int w, int h);

		SDL_PixelFormat* GetPixelFormat()
		{	return tempo->format;	}

		SDL_Surface * GetOrgScreen()
		{	return screen;			}

		inline short GetZoom()
		{	return (Engine==NoZoom?1:2); }

/*		operator SDL_Surface*()
		{	return screen;	}*/

};

extern Screen *E_Screen;

inline void UpdateRect( SDL_Surface *src, int x, int y, int w, int h )
{
	if ( src==screen)
		E_Screen->Update( x,y,w,h);
	else {
		SDL_UpdateRect(src, x, y, w, h );
		printf("normal update rect\n");
	}
}


#endif
