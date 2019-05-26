#pragma once

#include "resource.h"

bool InitDesktopDrawing();
HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent);
void timer();