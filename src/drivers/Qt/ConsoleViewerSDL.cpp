/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// GameViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <unistd.h>

#include "Qt/nes_shm.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleViewerSDL.h"

extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

ConsoleViewSDL_t::ConsoleViewSDL_t(QWidget *parent)
	: QWidget( parent )
{
	QPalette pal = palette();

	pal.setColor(QPalette::Window, Qt::black);
	setAutoFillBackground(true);
	setPalette(pal);

	setMinimumWidth( 256 );
	setMinimumHeight( 224 );
	setFocusPolicy(Qt::StrongFocus);

	view_width  = GL_NES_WIDTH;
	view_height = GL_NES_HEIGHT;

	sx = sy = 0;
	rw = view_width;
	rh = view_height;
	sdlRendW = 0;
	sdlRendH = 0;
	xscale = 2.0;
	yscale = 2.0;

	devPixRatio = 1.0f;
	aspectRatio = 1.0f;
	aspectX     = 1.0f;
	aspectY     = 1.0f;

	sdlWindow   = NULL;
	sdlRenderer = NULL;
	sdlTexture  = NULL;

	vsyncEnabled = false;
	mouseButtonMask = 0;

	localBufSize = (4 * GL_NES_WIDTH) * (4 * GL_NES_HEIGHT) * sizeof(uint32_t);

	localBuf = (uint32_t*)malloc( localBufSize );

	if ( localBuf )
	{
		memset( localBuf, 0, localBufSize );
	}

	forceAspect  = true;
	autoScaleEna = true;
	linearFilter = false;

	if ( g_config )
	{
		int opt;
		g_config->getOption("SDL.OpenGLip", &opt );
		
		linearFilter = (opt) ? true : false;

		g_config->getOption ("SDL.AutoScale", &opt);

		autoScaleEna = (opt) ? true : false;

		g_config->getOption("SDL.XScale", &xscale);
		g_config->getOption("SDL.YScale", &yscale);
	}
}

ConsoleViewSDL_t::~ConsoleViewSDL_t(void)
{
	if ( localBuf )
	{
		free( localBuf ); localBuf = NULL;
	}
}

void ConsoleViewSDL_t::setLinearFilterEnable( bool ena )
{
   if ( ena != linearFilter )
   {
      linearFilter = ena;

	   reset();
   }
}

void ConsoleViewSDL_t::setScaleXY( double xs, double ys )
{
	float xyRatio   = (float)nes_shm->video.xyRatio;

	xscale = xs;
	yscale = ys;

	if ( forceAspect )
	{
		if ( (xscale*xyRatio) < yscale )
		{
			yscale = (xscale*xyRatio);
		}
		else 
		{
			xscale = (yscale/xyRatio);
		}
	}
}

void ConsoleViewSDL_t::setAspectXY( double x, double y )
{
	aspectX = x;
	aspectY = y;

	aspectRatio = aspectY / aspectX;
}

void ConsoleViewSDL_t::getAspectXY( double &x, double &y )
{
	x = aspectX;
	y = aspectY;
}

double ConsoleViewSDL_t::getAspectRatio(void)
{
	return aspectRatio;
}

void ConsoleViewSDL_t::transfer2LocalBuffer(void)
{
	int i=0, hq = 0;
	int numPixels = nes_shm->video.ncol * nes_shm->video.nrow;
	unsigned int cpSize = numPixels * 4;
 	uint8_t *src, *dest;

	if ( cpSize > localBufSize )
	{
		cpSize = localBufSize;
	}
	src  = (uint8_t*)nes_shm->pixbuf;
	dest = (uint8_t*)localBuf;

	hq = (nes_shm->video.preScaler == 1) || (nes_shm->video.preScaler == 4); // hq2x and hq3x

	if ( hq )
	{
		for (i=0; i<numPixels; i++)
		{
			dest[3] = 0xFF;
			dest[1] = src[1];
			dest[2] = src[2];
			dest[0] = src[0];

			src += 4; dest += 4;
		}
	}
	else
	{
		memcpy( localBuf, nes_shm->pixbuf, cpSize );
	}
}

int ConsoleViewSDL_t::init(void)
{
	WId windowHandle;

   if ( linearFilter )
   {
       SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
   }
   else
   {
       SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "0" );
   }

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) 
	{
		printf("[SDL] Failed to initialize video subsystem.\n");
		return -1;
	}
	else
	{
		printf("Initialized SDL Video Subsystem\n");
	}

	for (int i=0; i<SDL_GetNumVideoDrivers(); i++)
	{
		printf("SDL Video Driver %i: %s\n", i, SDL_GetVideoDriver(i) );
	}
	printf("Using Video Driver: %s \n", SDL_GetCurrentVideoDriver() );

	windowHandle = this->winId();

	//printf("Window Handle: %llu \n", windowHandle );

	//sleep(1);

	sdlWindow = SDL_CreateWindowFrom( (void*)windowHandle);
	if (sdlWindow == NULL) 
	{
		printf("[SDL] Failed to create window from handle.\n");
		return -1;
	}

	uint32_t baseFlags = vsyncEnabled ? SDL_RENDERER_PRESENTVSYNC : 0;

	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, baseFlags | SDL_RENDERER_ACCELERATED);

	if (sdlRenderer == NULL) 
	{
		printf("[SDL] Failed to create accelerated renderer.\n");

		printf("[SDL] Attempting to create software renderer...\n");

		sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, baseFlags | SDL_RENDERER_SOFTWARE);

		if (sdlRenderer == NULL)
	  	{
			printf("[SDL] Failed to create software renderer.\n");
			return -1;
		}		
	}

	SDL_GetRendererOutputSize( sdlRenderer, &sdlRendW, &sdlRendH );

	printf("[SDL] Renderer Output Size: %i x %i \n", sdlRendW, sdlRendH );

	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, nes_shm->video.ncol, nes_shm->video.nrow);

	if (sdlTexture == NULL) 
	{
		printf("[SDL] Failed to create texture: %i x %i", nes_shm->video.ncol, nes_shm->video.nrow );
		return -1;
	}

	return 0;
}

void ConsoleViewSDL_t::cleanup(void)
{
	if (sdlTexture) 
	{
		SDL_DestroyTexture(sdlTexture);
		sdlTexture = NULL;		
	}
	if (sdlRenderer) 
	{
		SDL_DestroyRenderer(sdlRenderer);
		sdlRenderer = NULL;
	}
}

void ConsoleViewSDL_t::reset(void)
{
	cleanup();
	if ( init() == 0 )
  	{
	
	}
  	else 
	{
		cleanup();
	}
}

void ConsoleViewSDL_t::resizeEvent(QResizeEvent *event)
{
	QSize s;

	s = event->size();
	view_width  = s.width();
	view_height = s.height();
	//printf("SDL Resize: %i x %i \n", view_width, view_height);

	gui_draw_area_width = view_width;
	gui_draw_area_height = view_height;

	reset();
}

void ConsoleViewSDL_t::mousePressEvent(QMouseEvent * event)
{
	//printf("Mouse Button Press: (%i,%i) %x  %x\n", 
	//		event->pos().x(), event->pos().y(), event->button(), event->buttons() );

	mouseButtonMask = event->buttons();
}

void ConsoleViewSDL_t::mouseReleaseEvent(QMouseEvent * event)
{
	//printf("Mouse Button Release: (%i,%i) %x  %x\n", 
	//		event->pos().x(), event->pos().y(), event->button(), event->buttons() );

	mouseButtonMask = event->buttons();
}

bool ConsoleViewSDL_t::getMouseButtonState( unsigned int btn )
{
	return (mouseButtonMask & btn) ? true : false;
}

void  ConsoleViewSDL_t::getNormalizedCursorPos( double &x, double &y )
{
	QPoint cursor;

	cursor = QCursor::pos();

	//printf("Global Cursor (%i,%i) \n", cursor.x(), cursor.y() );

	cursor = mapFromGlobal( cursor );

	//printf("Window Cursor (%i,%i) \n", cursor.x(), cursor.y() );

	x = (double)(cursor.x() - sx) / (double)rw;
	y = (double)(cursor.y() - sy) / (double)rh;

	if ( x < 0.0 )
	{
		x = 0.0;
	}
	else if ( x > 1.0 )
	{
		x = 1.0;
	}
	if ( y < 0.0 )
	{
		y = 0.0;
	}
	else if ( y > 1.0 )
	{
		y = 1.0;
	}
	//printf("Normalized Cursor (%f,%f) \n", x, y );
}

void ConsoleViewSDL_t::render(void)
{
	int nesWidth  = GL_NES_WIDTH;
	int nesHeight = GL_NES_HEIGHT;
	float xyRatio = 1.0;

	if ( nes_shm != NULL )
	{
		nesWidth  = nes_shm->video.ncol;
		nesHeight = nes_shm->video.nrow;
		xyRatio   = (float)nes_shm->video.xyRatio;
	}
	//printf(" %i x %i \n", nesWidth, nesHeight );
	float xscaleTmp = (float)view_width  / (float)nesWidth;
	float yscaleTmp = (float)view_height / (float)nesHeight;

	if ( forceAspect )
	{
		if ( (xscaleTmp*xyRatio) < yscaleTmp )
		{
			yscaleTmp = (xscaleTmp*xyRatio);
		}
		else 
		{
			xscaleTmp = (yscaleTmp/xyRatio);
		}
	}

	if ( autoScaleEna )
	{
		xscale = xscaleTmp;
		yscale = yscaleTmp;
	}
	else
	{
		if ( xscaleTmp > xscale )
		{
			xscaleTmp = xscale;
		}
		if ( yscaleTmp > yscale )
		{
			yscaleTmp = yscale;
		}
	}

	rw=(int)(nesWidth*xscaleTmp);
	rh=(int)(nesHeight*yscaleTmp);

	if ( forceAspect )
	{
		int iw, ih, ax, ay;

		ax = (int)(aspectX+0.50);
		ay = (int)(aspectY+0.50);

		iw = rw * ay;
		ih = rh * ax;
		
		if ( iw > ih )
		{
			rh = (rw * ay) / ax;
		}
		else
		{
			rw = (rh * ax) / ay;
		}

		if ( rw > view_width )
		{
			rw = view_width;
			rh = (rw * ay) / ax;
		}

		if ( rh > view_height )
		{
			rh = view_height;
			rw = (rh * ax) / ay;
		}
	}

	if ( rw > view_width ) rw = view_width;
	if ( rh > view_height) rh = view_height;

	sx=(view_width-rw)/2;   
	sy=(view_height-rh)/2;

	if ( (sdlRenderer == NULL) || (sdlTexture == NULL) )
  	{
		return;
	}

	SDL_SetRenderDrawColor( sdlRenderer, 0, 0, 0, 0 );

	SDL_RenderClear(sdlRenderer);

	uint8_t *textureBuffer;
	int rowPitch;
	SDL_LockTexture( sdlTexture, nullptr, (void**)&textureBuffer, &rowPitch);
	{
		memcpy( textureBuffer, localBuf, nesWidth*nesHeight*sizeof(uint32_t) );
	}
	SDL_UnlockTexture(sdlTexture);

	SDL_Rect source = {0, 0, nesWidth, nesHeight };
	SDL_Rect dest = { sx, sy, rw, rh };
	SDL_RenderCopy(sdlRenderer, sdlTexture, &source, &dest);

	SDL_RenderPresent(sdlRenderer);

}
