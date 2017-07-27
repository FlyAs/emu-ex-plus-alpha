/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "VController"
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include "private.hh"

void VControllerDPad::init() {}

void VControllerDPad::setImg(Gfx::Renderer &r, Gfx::PixmapTexture &dpadR, Gfx::GTexC texHeight)
{
	using namespace Gfx;
	spr.init({-.5, -.5, .5, .5,});
	spr.setImg(&dpadR, {0., 0., 1., 64._gtexc/texHeight});
	spr.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void VControllerDPad::updateBoundingAreaGfx(Gfx::Renderer &r)
{
	if(visualizeBounds && padArea.xSize())
	{
		IG::MemPixmap mapPix{{padArea.size(), IG::PIXEL_FMT_RGB565}};
		iterateTimes(mapPix.h(), y)
			iterateTimes(mapPix.w(), x)
			{
				int input = getInput(padArea.xPos(LT2DO) + x, padArea.yPos(LT2DO) + y);
				//logMsg("got input %d", input);
				*((uint16*)mapPix.pixel({(int)x, (int)y})) = input == -1 ? IG::PIXEL_DESC_RGB565.build(1., 0., 0., 1.)
										: IG::isOdd(input) ? IG::PIXEL_DESC_RGB565.build(1., 1., 1., 1.)
										: IG::PIXEL_DESC_RGB565.build(0., 1., 0., 1.);
			}
		mapImg.init(r, {mapPix});
		mapImg.write(0, mapPix, {});
		mapSpr.init({}, mapImg);
		mapSpr.setPos(padArea, mainWin.projectionPlane);
	}
}

void VControllerDPad::setDeadzone(Gfx::Renderer &r, int newDeadzone)
{
	if(deadzone != newDeadzone)
	{
		deadzone = newDeadzone;
		updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::setDiagonalSensitivity(Gfx::Renderer &r, float newDiagonalSensitivity)
{
	if(diagonalSensitivity != newDiagonalSensitivity)
	{
		logMsg("set diagonal sensitivity: %f", (double)newDiagonalSensitivity);
		diagonalSensitivity = newDiagonalSensitivity;
		updateBoundingAreaGfx(r);
	}
}

IG::WindowRect VControllerDPad::bounds() const
{
	return padBaseArea;
}

void VControllerDPad::setSize(Gfx::Renderer &r, uint sizeInPixels)
{
	//logMsg("set dpad pixel size: %d", sizeInPixels);
	btnSizePixels = sizeInPixels;
	auto rect = IG::makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
	bool changedSize = rect.xSize() != padBaseArea.xSize();
	padBaseArea = rect;
	padArea = {0, 0, int(padBaseArea.xSize()*1.5), int(padBaseArea.xSize()*1.5)};
	if(visualizeBounds)
	{
		if(changedSize)
			updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::setPos(IG::Point2D<int> pos)
{
	padBaseArea.setPos(pos, C2DO);
	padBaseArea.fitIn(mainWin.viewport().bounds());
	padBase = mainWin.projectionPlane.unProjectRect(padBaseArea);
	spr.setPos(padBase);
	//logMsg("set dpad pos %d:%d:%d:%d, %f:%f:%f:%f", padBaseArea.x, padBaseArea.y, padBaseArea.x2, padBaseArea.y2,
	//	(double)padBase.x, (double)padBase.y, (double)padBase.x2, (double)padBase.y2);
	padArea.setPos(padBaseArea.pos(C2DO), C2DO);
	if(visualizeBounds)
	{
		mapSpr.setPos(padArea, mainWin.projectionPlane);
	}
}

void VControllerDPad::setBoundingAreaVisible(Gfx::Renderer &r, bool on)
{
	if(visualizeBounds == on)
		return;
	visualizeBounds = on;
	if(!on)
	{
		if(mapSpr.image())
		{
			logMsg("deallocating bounding box display resources");
			mapSpr.deinit();
			mapImg.deinit();
		}
	}
	else
	{
		updateBoundingAreaGfx(r);
	}
}

void VControllerDPad::draw(Gfx::Renderer &r) const
{
	Gfx::TextureSampler::bindDefaultNearestMipClampSampler(r);
	spr.useDefaultProgram(Gfx::IMG_MODE_MODULATE);
	spr.draw(r);

	if(visualizeBounds)
	{
		mapSpr.useDefaultProgram(Gfx::IMG_MODE_MODULATE);
		mapSpr.draw(r);
	}
}

int VControllerDPad::getInput(int cx, int cy) const
{
	if(padArea.overlaps({cx, cy}))
	{
		int x = cx - padArea.xCenter(), y = cy - padArea.yCenter();
		int xDeadzone = deadzone, yDeadzone = deadzone;
		if(std::abs(x) > deadzone)
			yDeadzone += (std::abs(x) - deadzone)/diagonalSensitivity;
		if(std::abs(y) > deadzone)
			xDeadzone += (std::abs(y) - deadzone)/diagonalSensitivity;
		//logMsg("dpad offset %d,%d, deadzone %d,%d", x, y, xDeadzone, yDeadzone);
		int pad = 4; // init to center
		if(std::abs(x) > xDeadzone)
		{
			if(x > 0)
				pad = 5; // right
			else
				pad = 3; // left
		}
		if(std::abs(y) > yDeadzone)
		{
			if(y > 0)
				pad += 3; // shift to top row
			else
				pad -= 3; // shift to bottom row
		}
		return pad == 4 ? -1 : pad; // don't send center dpad push
	}
	return -1;
}

void VControllerKeyboard::init()
{
	mode = 0;
}

void VControllerKeyboard::updateImg(Gfx::Renderer &r)
{
	if(mode)
		spr.setUVBounds({0., .5, texXEnd, 1.});
	else
		spr.setUVBounds({0., 0., texXEnd, .5});
	spr.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
}

void VControllerKeyboard::setImg(Gfx::Renderer &r, Gfx::PixmapTexture *img)
{
	spr.init({-.5, -.5, .5, .5}, *img);
	texXEnd = img->uvBounds().x2;
	updateImg(r);
}

void VControllerKeyboard::place(Gfx::GC btnSize, Gfx::GC yOffset)
{
	Gfx::GC xSize, ySize;
	IG::setSizesWithRatioX(xSize, ySize, 3./2., std::min(btnSize*10, mainWin.projectionPlane.w));
	Gfx::GC vArea = mainWin.projectionPlane.h - yOffset*2;
	if(ySize > vArea)
	{
		IG::setSizesWithRatioY(xSize, ySize, 3./2., vArea);
	}
	Gfx::GCRect boundGC {0., 0., xSize, ySize};
	boundGC.setPos({0., mainWin.projectionPlane.bounds().y + yOffset}, CB2DO);
	spr.setPos(boundGC);
	bound = mainWin.projectionPlane.projectRect(boundGC);
	keyXSize = (bound.xSize() / cols) + (bound.xSize() * (1./256.));
	keyYSize = bound.ySize() / 4;
	logMsg("key size %dx%d", keyXSize, keyYSize);
}

void VControllerKeyboard::draw(Gfx::Renderer &r) const
{
	if(spr.image()->levels() > 1)
		Gfx::TextureSampler::bindDefaultNearestMipClampSampler(r);
	else
		Gfx::TextureSampler::bindDefaultNoMipClampSampler(r);
	spr.useDefaultProgram(Gfx::IMG_MODE_MODULATE);
	spr.draw(r);
}

int VControllerKeyboard::getInput(int cx, int cy) const
{
	if(bound.overlaps({cx, cy}))
	{
		int relX = cx - bound.x, relY = cy - bound.y;
		uint row = relY/keyYSize;
		uint col;
		if((mode == 0 && row == 1) || row == 2)
		{
			relX -= keyXSize/2;
			col = relX/keyXSize;
			if(relX < 0)
				col = 0;
			else if(col > 8)
				col = 8;
		}
		else
		{
			if(row > 3)
				row = 3;
			col = relX/keyXSize;
			if(col > 9)
				col = 9;
		}
		uint idx = col + (row*cols);
		logMsg("pointer %d,%d key @ %d,%d, idx %d", relX, relY, row, col, idx);
		return idx;
	}
	return -1;
}

void VControllerGamepad::init(float alpha)
{
	if(!activeFaceBtns)
		activeFaceBtns = EmuSystem::inputFaceBtns;
	dp.init();
}

void VControllerGamepad::setBoundingAreaVisible(Gfx::Renderer &r, bool on)
{
	showBoundingArea = on;
	dp.setBoundingAreaVisible(r, on);
}

bool VControllerGamepad::boundingAreaVisible() const
{
	return showBoundingArea;
}

void VControllerGamepad::setImg(Gfx::Renderer &r, Gfx::PixmapTexture &pics)
{
	using namespace Gfx;
	pics.compileDefaultProgramOneShot(Gfx::IMG_MODE_MODULATE);
	Gfx::GTexC h = EmuSystem::inputFaceBtns == 2 ? 128. : 256.;
	dp.setImg(r, pics, h);
	iterateTimes(EmuSystem::inputCenterBtns, i)
	{
		centerBtnSpr[i].init({});
	}
	centerBtnSpr[0].setImg(&pics, {0., 65._gtexc/h, 32./64., 81._gtexc/h});
	if(EmuSystem::inputCenterBtns == 2)
	{
		centerBtnSpr[1].setImg(&pics, {33./64., 65._gtexc/h, 1., 81._gtexc/h});
	}

	iterateTimes(EmuSystem::inputFaceBtns, i)
	{
		circleBtnSpr[i].init({});
	}
	if(EmuSystem::inputFaceBtns == 2)
	{
		circleBtnSpr[0].setImg(&pics, {0., 82._gtexc/h, 32./64., 114._gtexc/h});
		circleBtnSpr[1].setImg(&pics, {33./64., 83._gtexc/h, 1., 114._gtexc/h});
	}
	else // for tall overlay image
	{
		circleBtnSpr[0].setImg(&pics, {0., 82._gtexc/h, 32./64., 114._gtexc/h});
		circleBtnSpr[1].setImg(&pics, {33./64., 83._gtexc/h, 1., 114._gtexc/h});
		circleBtnSpr[2].setImg(&pics, {0., 115._gtexc/h, 32./64., 147._gtexc/h});
		circleBtnSpr[3].setImg(&pics, {33./64., 116._gtexc/h, 1., 147._gtexc/h});
		if(EmuSystem::inputFaceBtns >= 6)
		{
			circleBtnSpr[4].setImg(&pics, {0., 148._gtexc/h, 32./64., 180._gtexc/h});
			circleBtnSpr[5].setImg(&pics, {33./64., 149._gtexc/h, 1., 180._gtexc/h});
		}
		if(EmuSystem::inputFaceBtns == 8)
		{
			circleBtnSpr[6].setImg(&pics, {0., 181._gtexc/h, 32./64., 213._gtexc/h});
			circleBtnSpr[7].setImg(&pics, {33./64., 182._gtexc/h, 1., 213._gtexc/h});
		}
	}
}

IG::WindowRect VControllerGamepad::centerBtnBounds() const
{
	return centerBtnsBound;
}

void VControllerGamepad::setCenterBtnPos(IG::Point2D<int> pos)
{
	centerBtnsBound.setPos(pos, C2DO);
	centerBtnsBound.fitIn(mainWin.viewport().bounds());
	int buttonXSpace = btnSpacePixels;//btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
	int extraXSize = buttonXSpace + btnSizePixels * btnExtraXSize;
	int spriteYPos = centerBtnsBound.yCenter() - centerBtnsBound.ySize()/6;
	if(EmuSystem::inputCenterBtns == 2)
	{
		centerBtnBound[0] = IG::makeWindowRectRel({centerBtnsBound.x - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnBound[1] = IG::makeWindowRectRel({(centerBtnsBound.x2 - btnSizePixels) - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnSpr[0].setPos(mainWin.projectionPlane.unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x, spriteYPos}, {btnSizePixels, btnSizePixels/2})));
		centerBtnSpr[1].setPos(mainWin.projectionPlane.unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x2 - btnSizePixels, spriteYPos}, {btnSizePixels, btnSizePixels/2})));
	}
	else
	{
		centerBtnBound[0] = IG::makeWindowRectRel({centerBtnsBound.x - extraXSize/2, centerBtnsBound.y}, {btnSizePixels + extraXSize, centerBtnsBound.ySize()});
		centerBtnSpr[0].setPos(mainWin.projectionPlane.unProjectRect(IG::makeWindowRectRel({centerBtnsBound.x, spriteYPos}, {centerBtnsBound.xSize(), btnSizePixels/2})));
	}
}

static uint lTriggerIdx()
{
	return EmuSystem::inputFaceBtns-2;
}

static uint rTriggerIdx()
{
	return EmuSystem::inputFaceBtns-1;
}

IG::WindowRect VControllerGamepad::lTriggerBounds() const
{
	return lTriggerBound;
}

void VControllerGamepad::setLTriggerPos(IG::Point2D<int> pos)
{
	uint idx = lTriggerIdx();
	lTriggerBound.setPos(pos, C2DO);
	lTriggerBound.fitIn(mainWin.viewport().bounds());
	auto lTriggerAreaGC = mainWin.projectionPlane.unProjectRect(lTriggerBound);
	faceBtnBound[idx] = lTriggerBound;
	circleBtnSpr[idx].setPos(lTriggerBound, mainWin.projectionPlane);
}

IG::WindowRect VControllerGamepad::rTriggerBounds() const
{
	return rTriggerBound;
}

void VControllerGamepad::setRTriggerPos(IG::Point2D<int> pos)
{
	uint idx = rTriggerIdx();
	rTriggerBound.setPos(pos, C2DO);
	rTriggerBound.fitIn(mainWin.viewport().bounds());
	auto rTriggerAreaGC = mainWin.projectionPlane.unProjectRect(rTriggerBound);
	faceBtnBound[idx] = rTriggerBound;
	circleBtnSpr[idx].setPos(rTriggerBound, mainWin.projectionPlane);
}

void VControllerGamepad::layoutBtnRows(uint a[], uint btns, uint rows, IG::Point2D<int> pos)
{
	int btnsPerRow = btns/rows;
	//logMsg("laying out buttons with size %d, space %d, row shift %d, stagger %d", btnSizePixels, btnSpacePixels, btnRowShiftPixels, btnStaggerPixels);
	faceBtnsBound.setPos(pos, C2DO);
	faceBtnsBound.fitIn(mainWin.viewport().bounds());
	auto btnArea = mainWin.projectionPlane.unProjectRect(faceBtnsBound);

	int row = 0, btnPos = 0;
	Gfx::GC yOffset = (btnStagger < 0) ? -btnStagger*(btnsPerRow-1) : 0,
		xOffset = -btnRowShift*(rows-1),
		staggerOffset = 0;
	iterateTimes(btns, i)
	{
		auto faceBtn = Gfx::makeGCRectRel(
			btnArea.pos(LB2DO) + Gfx::GP{xOffset, yOffset + staggerOffset}, {btnSize, btnSize});
		xOffset += btnSize + btnSpace;
		staggerOffset += btnStagger;
		if(++btnPos == btnsPerRow)
		{
			row++;
			yOffset += btnSize + btnSpace;
			staggerOffset = 0;
			xOffset = -btnRowShift*((rows-1)-row);
			btnPos = 0;
		}

		circleBtnSpr[a[i]].setPos(faceBtn);

		Gfx::GC btnExtraYSizeVal = (rows == 1) ? btnExtraYSize : btnExtraYSizeMultiRow;
		uint buttonXSpace = btnExtraXSize ? btnSpacePixels * 2 : btnSpacePixels;
		uint buttonYSpace = btnExtraYSizeVal ? btnSpacePixels * 2 : btnSpacePixels;
		int extraXSize = buttonXSpace + (Gfx::GC)btnSizePixels * btnExtraXSize;
		int extraYSize = buttonYSpace + (Gfx::GC)btnSizePixels * btnExtraYSizeVal;
		auto &bound = faceBtnBound[a[i]];
		bound = mainWin.projectionPlane.projectRect(faceBtn);
		bound = IG::makeWindowRectRel(bound.pos(LT2DO) - IG::WP{extraXSize/2, extraYSize/2},
			bound.size() + IG::WP{extraXSize, extraYSize});
	}
}

IG::WindowRect VControllerGamepad::faceBtnBounds() const
{
	return faceBtnsBound;
}

uint VControllerGamepad::rowsForButtons(uint activeButtons)
{
	if(!EmuSystem::inputHasTriggerBtns)
	{
		return (activeButtons > 3) ? 2 : 1;
	}
	else
	{
		return triggersInline ? 2 : (EmuSystem::inputFaceBtns < 6) ? 1 : 2;
	}
}

void VControllerGamepad::setFaceBtnPos(IG::Point2D<int> pos)
{
	using namespace IG;

	if(!EmuSystem::inputHasTriggerBtns)
	{
		uint btnMap[] 		{1, 0};
		uint btnMap2Rev[] {0, 1};
		uint btnMap3[] 		{0, 1, 2};
		uint btnMap4[] 		{0, 1, 2, 3};
		uint btnMap6Rev[] {0, 1, 2, 3, 4, 5};
		uint btnMap6[] 		{2, 1, 0, 3, 4, 5};
		uint rows = rowsForButtons(activeFaceBtns);
		if(activeFaceBtns == 6)
			layoutBtnRows(EmuSystem::inputHasRevBtnLayout ? btnMap6Rev : btnMap6, IG::size(btnMap6), rows, pos);
		else if(activeFaceBtns == 4)
			layoutBtnRows(btnMap4, IG::size(btnMap4), rows, pos);
		else if(activeFaceBtns == 3)
			layoutBtnRows(btnMap3, IG::size(btnMap3), rows, pos);
		else
			layoutBtnRows(EmuSystem::inputHasRevBtnLayout ? btnMap2Rev : btnMap, IG::size(btnMap), rows, pos);
	}
	else
	{
		if(triggersInline)
		{
			uint btnMap8[] {0, 1, 2, 6, 3, 4, 5, 7};
			uint btnMap6[] {1, 0, 5, 3, 2, 4};
			uint btnMap4[] {1, 0, 2, 3};
			if(EmuSystem::inputFaceBtns == 8)
				layoutBtnRows(btnMap8, IG::size(btnMap8), 2, pos);
			else if(EmuSystem::inputFaceBtns == 6)
				layoutBtnRows(btnMap6, IG::size(btnMap6), 2, pos);
			else
				layoutBtnRows(btnMap4, IG::size(btnMap4), 2, pos);
		}
		else
		{
			uint btnMap8[] {0, 1, 2, 3, 4, 5};
			uint btnMap6[] {1, 0, 3, 2};
			uint btnMap4[] {1, 0};
			if(EmuSystem::inputFaceBtns == 8)
				layoutBtnRows(btnMap8, IG::size(btnMap8), 2, pos);
			else if(EmuSystem::inputFaceBtns == 6)
				layoutBtnRows(btnMap6, IG::size(btnMap6), 2, pos);
			else
				layoutBtnRows(btnMap4, IG::size(btnMap4), 1, pos);
		}
	}
}

void VControllerGamepad::setBaseBtnSize(Gfx::Renderer &r, uint sizeInPixels)
{
	btnSizePixels = sizeInPixels;
	btnSize = mainWin.projectionPlane.unprojectXSize(sizeInPixels);
	dp.setSize(r, IG::makeEvenRoundedUp(int(sizeInPixels*(double)2.5)));

	// face buttons
	uint btns = (EmuSystem::inputHasTriggerBtns && !triggersInline) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns;
	uint rows = rowsForButtons(activeFaceBtns);
	int btnsPerRow = btns/rows;
	int xSizePixel = btnSizePixels*btnsPerRow + btnSpacePixels*(btnsPerRow-1) + std::abs(btnRowShiftPixels*((int)rows-1));
	int ySizePixel = btnSizePixels*rows + btnSpacePixels*(rows-1) + std::abs(btnStaggerPixels*((int)btnsPerRow-1));
	faceBtnsBound = IG::makeWindowRectRel({0, 0}, {xSizePixel, ySizePixel});

	// center buttons
	int cenBtnFullXSize = (EmuSystem::inputCenterBtns == 2) ? (btnSizePixels*2) + btnSpacePixels : btnSizePixels;
	int cenBtnFullYSize = IG::makeEvenRoundedUp(int(btnSizePixels*(double)1.25));
	centerBtnsBound = IG::makeWindowRectRel({0, 0}, {cenBtnFullXSize, cenBtnFullYSize});

	// triggers
	lTriggerBound = IG::makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
	rTriggerBound = IG::makeWindowRectRel({0, 0}, {btnSizePixels, btnSizePixels});
}

std::array<int, 2> VControllerGamepad::getCenterBtnInput(int x, int y) const
{
	std::array<int, 2> btnOut{-1, -1};
	uint count = 0;
	iterateTimes(EmuSystem::inputCenterBtns, i)
	{
		if(centerBtnBound[i].overlaps({x, y}))
		{
			//logMsg("overlaps %d", (int)i);
			btnOut[count] = i;
			count++;
			if(count == 2)
				return btnOut;
		}
	}
	return btnOut;
}

std::array<int, 2> VControllerGamepad::getBtnInput(int x, int y) const
{
	std::array<int, 2> btnOut{-1, -1};
	uint count = 0;
	bool doSeparateTriggers = EmuSystem::inputHasTriggerBtns && !triggersInline;
	if(faceBtnsState)
	{
		iterateTimes(doSeparateTriggers ? EmuSystem::inputFaceBtns-2 : activeFaceBtns, i)
		{
			if(faceBtnBound[i].overlaps({x, y}))
			{
				//logMsg("overlaps %d", (int)i);
				btnOut[count] = i;
				count++;
				if(count == 2)
					return btnOut;
			}
		}
	}

	if(doSeparateTriggers)
	{
		if(lTriggerState)
		{
			if(faceBtnBound[lTriggerIdx()].overlaps({x, y}))
			{
				btnOut[count] = lTriggerIdx();
				count++;
				if(count == 2)
					return btnOut;
			}
		}
		if(rTriggerState)
		{
			if(faceBtnBound[rTriggerIdx()].overlaps({x, y}))
			{
				btnOut[count] = rTriggerIdx();
				count++;
				if(count == 2)
					return btnOut;
			}
		}
	}

	return btnOut;
}

void VControllerGamepad::draw(Gfx::Renderer &r, bool showHidden) const
{
	using namespace Gfx;
	if(dp.state == 1 || (showHidden && dp.state))
	{
		dp.draw(r);
	}

	if(faceBtnsState == 1 || (showHidden && faceBtnsState))
	{
		TextureSampler::bindDefaultNearestMipClampSampler(r);
		if(showBoundingArea)
		{
			r.noTexProgram.use(r);
			iterateTimes((EmuSystem::inputHasTriggerBtns && !triggersInline) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns, i)
			{
				GeomRect::draw(r, faceBtnBound[i], mainWin.projectionPlane);
			}
		}
		dp.spr.useDefaultProgram(Gfx::IMG_MODE_MODULATE);
		//GeomRect::draw(faceBtnsBound);
		iterateTimes((EmuSystem::inputHasTriggerBtns && !triggersInline) ? EmuSystem::inputFaceBtns-2 : activeFaceBtns, i)
		{
			circleBtnSpr[i].draw(r);
		}
	}

	if(EmuSystem::inputHasTriggerBtns && !triggersInline)
	{
		TextureSampler::bindDefaultNearestMipClampSampler(r);
		if(showBoundingArea)
		{
			if(lTriggerState == 1 || (showHidden && lTriggerState))
			{
				r.noTexProgram.use(r);
				GeomRect::draw(r, faceBtnBound[lTriggerIdx()], mainWin.projectionPlane);
			}
			if(rTriggerState == 1 || (showHidden && rTriggerState))
			{
				r.noTexProgram.use(r);
				GeomRect::draw(r, faceBtnBound[rTriggerIdx()], mainWin.projectionPlane);
			}
		}
		if(lTriggerState == 1 || (showHidden && lTriggerState))
		{
			dp.spr.useDefaultProgram(Gfx::IMG_MODE_MODULATE);
			circleBtnSpr[lTriggerIdx()].draw(r);
		}
		if(rTriggerState == 1 || (showHidden && rTriggerState))
		{
			dp.spr.useDefaultProgram(Gfx::IMG_MODE_MODULATE);
			circleBtnSpr[rTriggerIdx()].draw(r);
		}
	}

	if(centerBtnsState == 1 || (showHidden && centerBtnsState))
	{
		TextureSampler::bindDefaultNearestMipClampSampler(r);
		if(showBoundingArea)
		{
			r.noTexProgram.use(r);
			iterateTimes(EmuSystem::inputCenterBtns, i)
			{
				GeomRect::draw(r, centerBtnBound[i], mainWin.projectionPlane);
			}
		}
		dp.spr.useDefaultProgram(Gfx::IMG_MODE_MODULATE);
		iterateTimes(EmuSystem::inputCenterBtns, i)
		{
			centerBtnSpr[i].draw(r);
		}
	}
}

Gfx::GC VController::xMMSize(Gfx::GC mm) const
{
	return useScaledCoordinates ? mainWin.projectionPlane.xSMMSize(mm) : mainWin.projectionPlane.xMMSize(mm);
}

Gfx::GC VController::yMMSize(Gfx::GC mm) const
{
	return useScaledCoordinates ? mainWin.projectionPlane.ySMMSize(mm) : mainWin.projectionPlane.yMMSize(mm);
}

int VController::xMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const
{
	return useScaledCoordinates ? win.widthSMMInPixels(mm) : win.widthMMInPixels(mm);
}

int VController::yMMSizeToPixel(const Base::Window &win, Gfx::GC mm) const
{
	return useScaledCoordinates ? win.heightSMMInPixels(mm) : win.heightMMInPixels(mm);
}

bool VController::hasTriggers() const
{
	return EmuSystem::inputHasTriggerBtns;
}

void VController::setImg(Gfx::PixmapTexture &pics)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setImg(renderer, pics);
	#endif
}

void VController::setBoundingAreaVisible(bool on)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBoundingAreaVisible(renderer, on);
	#endif
}

bool VController::boundingAreaVisible() const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	return gp.boundingAreaVisible();
	#else
	return false;
	#endif
}

void VController::setMenuBtnPos(IG::Point2D<int> pos)
{
	menuBound.setPos(pos, C2DO);
	menuBound.fitIn(mainWin.viewport().bounds());
	menuBtnSpr.setPos(menuBound, mainWin.projectionPlane);
}

void VController::setFFBtnPos(IG::Point2D<int> pos)
{
	ffBound.setPos(pos, C2DO);
	ffBound.fitIn(mainWin.viewport().bounds());
	ffBtnSpr.setPos(ffBound, mainWin.projectionPlane);
}

void VController::setBaseBtnSize(uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP)
{
	if(EmuSystem::inputHasKeyboard)
		kb.place(projP.unprojectYSize(gamepadBtnSizeInPixels), projP.unprojectYSize(gamepadBtnSizeInPixels * .75));
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.setBaseBtnSize(renderer, gamepadBtnSizeInPixels);
	#endif
	int size = uiBtnSizeInPixels;
	if(menuBound.xSize() != size)
		logMsg("set UI button size: %d", size);
	menuBound = IG::makeWindowRectRel({0, 0}, {size, size});
	ffBound = IG::makeWindowRectRel({0, 0}, {size, size});
}

void VController::inputAction(uint action, uint vBtn)
{
	if(isInKeyboardMode())
	{
		assert(vBtn < IG::size(kbMap));
		EmuSystem::handleInputAction(action, kbMap[vBtn]);
	}
	else
	{
		assert(vBtn < IG::size(map));
		auto turbo = map[vBtn] & TURBO_BIT;
		auto keyCode = map[vBtn] & ACTION_MASK;
		if(turbo)
		{
			if(action == Input::PUSHED)
			{
				turboActions.addEvent(keyCode);
			}
			else
			{
				turboActions.removeEvent(keyCode);
			}
		}
		EmuSystem::handleInputAction(action, keyCode);
	}
}

void VController::resetInput(bool init)
{
	for(auto &e : ptrElem)
	{
		for(auto &vBtn : e)
		{
			if(!init && vBtn != -1) // release old key, if any
				inputAction(Input::RELEASED, vBtn);
			vBtn = -1;
		}
	}
}

void VController::init(float alpha, uint gamepadBtnSizeInPixels, uint uiBtnSizeInPixels, const Gfx::ProjectionPlane &projP)
{
	this->alpha = alpha;
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	gp.init(alpha);
	#endif
	setBaseBtnSize(gamepadBtnSizeInPixels, uiBtnSizeInPixels, projP);
	if(EmuSystem::inputHasKeyboard)
	{
		kb.init();
		kbMode = 0;
	}
	resetInput(1);
}

void VController::place()
{
	resetInput();
}

void VController::toggleKeyboard()
{
	logMsg("toggling keyboard");
	resetInput();
	kbMode ^= true;
}

std::array<int, 2> VController::findElementUnderPos(Input::Event e)
{
	if(isInKeyboardMode())
	{
		int kbChar = kb.getInput(e.x, e.y);
		if(kbChar == -1)
			return {-1, -1};
		if(kbChar == 30 && e.pushed())
		{
			logMsg("dismiss kb");
			toggleKeyboard();
		}
		else if((kbChar == 31 || kbChar == 32) && e.pushed())
		{
			logMsg("switch kb mode");
			kb.mode ^= true;
			kb.updateImg(renderer);
			resetInput();
			updateKeyboardMapping();
		}
		else
			 return {kbChar, -1};
		return {-1, -1};
	}

	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if(gp.centerBtnsState != 0)
	{
		auto elem = gp.getCenterBtnInput(e.x, e.y);
		if(elem[0] != -1)
		{
			return {C_ELEM + elem[0], elem[1] != -1 ? C_ELEM + elem[1] : -1};
		}
	}

	{
		auto elem = gp.getBtnInput(e.x, e.y);
		if(elem[0] != -1)
		{
			return {F_ELEM + elem[0], elem[1] != -1 ? F_ELEM + elem[1] : -1};
		}
	}

	if(gp.dp.state != 0)
	{
		int elem = gp.dp.getInput(e.x, e.y);
		if(elem != -1)
		{
			return {D_ELEM + elem, -1};
		}
	}
	#endif
	return {-1, -1};
}

void VController::applyInput(Input::Event e)
{
	using namespace IG;
	assert(e.isPointer());
	auto &currElem = ptrElem[e.devId];
	std::array<int, 2> elem{-1, -1};
	if(e.isPointerPushed(Input::Pointer::LBUTTON)) // make sure the cursor isn't hovering
		elem = findElementUnderPos(e);

	//logMsg("under %d %d", elem[0], elem[1]);

	// release old buttons
	for(auto vBtn : currElem)
	{
		if(vBtn != -1 && !IG::contains(elem, vBtn))
		{
			//logMsg("releasing %d", vBtn);
			inputAction(Input::RELEASED, vBtn);
		}
	}

	// push new buttons
	for(auto vBtn : elem)
	{
		if(vBtn != -1 && !IG::contains(currElem, vBtn))
		{
			//logMsg("pushing %d", vBtn);
			inputAction(Input::PUSHED, vBtn);
			if(optionVibrateOnPush)
			{
				Base::vibrate(32);
			}
		}
	}

	currElem = elem;
}

void VController::draw(bool emuSystemControls, bool activeFF, bool showHidden)
{
	draw(emuSystemControls, activeFF, showHidden, alpha);
}

void VController::draw(bool emuSystemControls, bool activeFF, bool showHidden, float alpha)
{
	using namespace Gfx;
	if(unlikely(alpha == 0.))
		return;
	auto &r = renderer;
	r.setBlendMode(BLEND_MODE_ALPHA);
	r.setColor(1., 1., 1., alpha);
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	if(isInKeyboardMode())
		kb.draw(r);
	else if(emuSystemControls)
		gp.draw(r, showHidden);
	#else
	if(isInKeyboardMode())
		kb.draw();
	#endif
	//GeomRect::draw(menuBound);
	//GeomRect::draw(ffBound);
	if(menuBtnState == 1 || (showHidden && menuBtnState))
	{
		TextureSampler::bindDefaultNearestMipClampSampler(r);
		menuBtnSpr.useDefaultProgram(IMG_MODE_MODULATE);
		menuBtnSpr.draw(r);
	}
	if(ffBtnState == 1 || (showHidden && ffBtnState))
	{
		TextureSampler::bindDefaultNearestMipClampSampler(r);
		ffBtnSpr.useDefaultProgram(IMG_MODE_MODULATE);
		if(activeFF)
			r.setColor(1., 0., 0., alpha);
		ffBtnSpr.draw(r);
	}
}

int VController::numElements() const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	return (EmuSystem::inputHasTriggerBtns && !gp.triggersInline) ? 7 : 5;
	#else
	return 5;
	#endif
}

IG::WindowRect VController::bounds(int elemIdx) const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		case 0: return gp.dp.bounds();
		case 1: return gp.centerBtnBounds();
		case 2: return gp.faceBtnBounds();
		case 3: return menuBound;
		case 4: return ffBound;
		case 5: return gp.lTriggerBounds();
		case 6: return gp.rTriggerBounds();
		default: bug_branch("%d", elemIdx); return {0,0,0,0};
	}
	#else
	switch(elemIdx)
	{
		case 0: return {0,0,0,0};
		case 1: return {0,0,0,0};
		case 2: return {0,0,0,0};
		case 3: return menuBound;
		case 4: return ffBound;
		default: bug_branch("%d", elemIdx); return {0,0,0,0};
	}
	#endif
}

void VController::setPos(int elemIdx, IG::Point2D<int> pos)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		bcase 0: gp.dp.setPos(pos);
		bcase 1: gp.setCenterBtnPos(pos);
		bcase 2: gp.setFaceBtnPos(pos);
		bcase 3: setMenuBtnPos(pos);
		bcase 4: setFFBtnPos(pos);
		bcase 5: gp.setLTriggerPos(pos);
		bcase 6: gp.setRTriggerPos(pos);
		bdefault: bug_branch("%d", elemIdx);
	}
	#else
	switch(elemIdx)
	{
		bcase 0:
		bcase 1:
		bcase 2:
		bcase 3: setMenuBtnPos(pos);
		bcase 4: setFFBtnPos(pos);
		bdefault: bug_branch("%d", elemIdx);
	}
	#endif
}

void VController::setState(int elemIdx, uint state)
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		bcase 0: gp.dp.state = state;
		bcase 1: gp.centerBtnsState = state;
		bcase 2: gp.faceBtnsState = state;
		bcase 3: menuBtnState = state;
		bcase 4: ffBtnState = state;
		bcase 5: gp.lTriggerState = state;
		bcase 6: gp.rTriggerState = state;
		bdefault: bug_branch("%d", elemIdx);
	}
	#else
	switch(elemIdx)
	{
		bcase 0:
		bcase 1:
		bcase 2:
		bcase 3: menuBtnState = state;
		bcase 4: ffBtnState = state;
		bdefault: bug_branch("%d", elemIdx);
	}
	#endif
}

uint VController::state(int elemIdx) const
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	switch(elemIdx)
	{
		case 0: return gp.dp.state;
		case 1: return gp.centerBtnsState;
		case 2: return gp.faceBtnsState;
		case 3: return menuBtnState;
		case 4: return ffBtnState;
		case 5: return gp.lTriggerState;
		case 6: return gp.rTriggerState;
		default: bug_branch("%d", elemIdx); return 0;
	}
	#else
	switch(elemIdx)
	{
		case 0: return 0;
		case 1: return 0;
		case 2: return 0;
		case 3: return menuBtnState;
		case 4: return ffBtnState;
		default: bug_branch("%d", elemIdx); return 0;
	}
	#endif
}

bool VController::isInKeyboardMode() const
{
	return EmuSystem::inputHasKeyboard && vController.kbMode;
}

void VController::updateMapping(uint player)
{
	updateVControllerMapping(player, map);
}

void VController::updateKeyboardMapping()
{
	updateVControllerKeyboardMapping(kb.mode, kbMap);
}

[[gnu::weak]] void updateVControllerKeyboardMapping(uint mode, SysVController::KbMap &map) {}
