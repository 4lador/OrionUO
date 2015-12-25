/****************************************************************************
**
** GameScreen.cpp
**
** Copyright (C) September 2015 Hotride
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*****************************************************************************
*/
//---------------------------------------------------------------------------
#include "stdafx.h"

TGameScreen *GameScreen = NULL;
//---------------------------------------------------------------------------
TGameScreen::TGameScreen()
: TBaseScreen(), m_GameWindowMoving(false), m_GameWindowResizing(false),
m_ListSize(0)
{
	memset(&m_List[0], 0, sizeof(m_List));
}
//---------------------------------------------------------------------------
TGameScreen::~TGameScreen()
{
}
//---------------------------------------------------------------------------
void TGameScreen::Init()
{
	SendMessage(g_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
	SendMessage(g_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);

	m_GameWindowMoving = false;
	m_GameWindowResizing = false;

	Tooltip.SeqIndex = 0;

	//Prepare textures on Main Screen:
	UO->ExecuteGump(0x0588); //Main Screen background
	UO->ExecuteGump(0x157C); //Main Screen
	UO->ExecuteGump(0x15A0); //Main Screen Notes
	UO->ExecuteResizepic(0x13BE); //ActPwd Container
	UO->ExecuteGump(0x058A); //Main Screen Castle?
	UO->ExecuteGumpPart(0x1589, 3); //X gump
	UO->ExecuteGumpPart(0x15A4, 3); //> gump
	UO->ExecuteResizepic(0x0BB8); //Account/Password text field
	UO->ExecuteGumpPart(0x00D2, 2); //Checkbox on / off
}
//---------------------------------------------------------------------------
void TGameScreen::InitTooltip()
{
	if (!ConfigManager.UseToolTips)
		return;

}
//---------------------------------------------------------------------------
int TGameScreen::GetMaxDrawZ(bool &noDrawRoof, int &maxGroundZ)
{
	int playerX = g_Player->X;
	int playerY = g_Player->Y;
	int playerZ = g_Player->Z;

	int maxDrawZ = 125; //playerZ + 15;

	int bx = playerX / 8;
	int by = playerY / 8;

	int blockIndex = (bx * 512) + by;
	TMapBlock *mb = MapManager->GetBlock(blockIndex);

	if (mb != NULL)
	{
		int x = playerX % 8;
		int y = playerY % 8;

		maxDrawZ = 125;
		int pz15 = playerZ + 15;
		int pz5 = playerZ + 5;

		for (TRenderWorldObject *ro = mb->GetRender(x, y); ro != NULL; ro = ro->m_NextXY)
		{
			if (ro->IsLandObject())
			{
				int testZ = ((TLandObject*)ro)->MinZ;

				if (pz15 <= testZ)
				{
					maxGroundZ = pz15 + 1;
					maxDrawZ = maxGroundZ;

					break;
				}
			}

			if (!ro->IsStaticObject() && !ro->IsGameObject() && !ro->IsMultiObject())
				continue;

			if (ro->IsGameObject() && ((TGameObject*)ro)->NPC)
				continue;
	
			if (ro->Z >= pz15 && maxDrawZ > ro->Z && !ro->IsRoof() && (ro->IsSurface() || ro->IsImpassable()))
				maxDrawZ = ro->Z;
			else  if (ro->Z > pz5 && (ro->IsRoof() || (ro->IsBackground() && ro->IsSurface())))
			{
				if (maxDrawZ > pz15)
					maxDrawZ = pz15;

				noDrawRoof = true;
			}
		}
	}

	return maxDrawZ;
}
//---------------------------------------------------------------------------
void TGameScreen::CalculateGameWindow()
{
	m_RenderBounds.GameWindowPosX = g_GameWindowPosX;
	m_RenderBounds.GameWindowPosY = g_GameWindowPosY;

	m_RenderBounds.GameWindowSizeX = g_GameWindowSizeX;
	m_RenderBounds.GameWindowSizeY = g_GameWindowSizeY;

	if (g_LastObjectType == SOT_GAME_GUMP_SCOPE && g_LeftMouseDown && !g_LastGumpLeftMouseDown && !g_LastGumpRightMouseDown)
	{
		if (g_LastObjectLeftMouseDown == 2)
		{
			m_RenderBounds.GameWindowSizeX += (g_MouseX - g_DroppedLeftMouseX);
			m_RenderBounds.GameWindowSizeY += (g_MouseY - g_DroppedLeftMouseY);

			if (m_RenderBounds.GameWindowSizeX < 640)
				m_RenderBounds.GameWindowSizeX = 640;

			if (m_RenderBounds.GameWindowSizeY < 480)
				m_RenderBounds.GameWindowSizeY = 480;

			TGumpOptions *opt = (TGumpOptions*)GumpManager->GetGump(g_PlayerSerial, 0, GT_OPTIONS);

			if (opt != NULL)
			{
				char txtBuf[20] = {0};

				sprintf(txtBuf, "%i", m_RenderBounds.GameWindowSizeX);
				opt->TextEntryGameSizeX->SetText(txtBuf);

				sprintf(txtBuf, "%i", m_RenderBounds.GameWindowSizeY);
				opt->TextEntryGameSizeY->SetText(txtBuf);

				opt->GenerateFrame(opt->X, opt->Y);
			}
		}

		if (g_LastObjectLeftMouseDown == 1)
		{
			m_RenderBounds.GameWindowPosX += (g_MouseX - g_DroppedLeftMouseX);
			m_RenderBounds.GameWindowPosY += (g_MouseY - g_DroppedLeftMouseY);

			if (m_RenderBounds.GameWindowPosX < 0)
				m_RenderBounds.GameWindowPosX = 0;

			if (m_RenderBounds.GameWindowPosY < 0)
				m_RenderBounds.GameWindowPosY = 0;

			if (m_RenderBounds.GameWindowPosX + m_RenderBounds.GameWindowSizeX > g_ClientWidth)
				m_RenderBounds.GameWindowPosX = g_ClientWidth - m_RenderBounds.GameWindowSizeX;

			if (m_RenderBounds.GameWindowPosY + m_RenderBounds.GameWindowSizeY > g_ClientHeight)
				m_RenderBounds.GameWindowPosY = g_ClientHeight - m_RenderBounds.GameWindowSizeY;
		}
	}

	int playerZOffset = (g_Player->Z * 4);

	m_RenderBounds.GameWindowCenterX = m_RenderBounds.GameWindowPosX + m_RenderBounds.GameWindowSizeX / 2;
	m_RenderBounds.GameWindowCenterY = (m_RenderBounds.GameWindowPosY + m_RenderBounds.GameWindowSizeY / 2) + playerZOffset;

	m_RenderBounds.GameWindowCenterX -= (int)g_Player->OffsetX;
	m_RenderBounds.GameWindowCenterY -= (int)g_Player->OffsetY;

	g_PlayerRect.X = m_RenderBounds.GameWindowCenterX - 40;
	g_PlayerRect.Y = m_RenderBounds.GameWindowCenterY - 60 - (g_Player->Z * 4);
	g_PlayerRect.Width = 80;
	g_PlayerRect.Height = 120;

	int rangeX = (m_RenderBounds.GameWindowSizeX / 44) + 1;
	int rangeY = (m_RenderBounds.GameWindowSizeY / 44) + 1;

	if (rangeX < rangeY)
		rangeX = rangeY;
	else
		rangeY = rangeX;

	m_RenderBounds.RealMinRangeX = g_Player->X - rangeX;
	m_RenderBounds.RealMaxRangeX = g_Player->X + rangeX;
	m_RenderBounds.RealMinRangeY = g_Player->Y - rangeY;
	m_RenderBounds.RealMaxRangeY = g_Player->Y + rangeY;

	m_RenderBounds.MinBlockX = (m_RenderBounds.RealMinRangeX / 8) - 1;
	m_RenderBounds.MinBlockY = (m_RenderBounds.RealMinRangeY / 8) - 1;
	m_RenderBounds.MaxBlockX = (m_RenderBounds.RealMaxRangeX / 8) + 1;
	m_RenderBounds.MaxBlockY = (m_RenderBounds.RealMaxRangeY / 8) + 1;

	if (m_RenderBounds.MinBlockX < 0)
		m_RenderBounds.MinBlockX = 0;

	if (m_RenderBounds.MinBlockY < 0)
		m_RenderBounds.MinBlockY = 0;

	if (m_RenderBounds.MaxBlockX >= g_MapBlockX[g_CurrentMap])
		m_RenderBounds.MaxBlockX = g_MapBlockX[g_CurrentMap] - 1;

	if (m_RenderBounds.MaxBlockY >= g_MapBlockY[g_CurrentMap])
		m_RenderBounds.MaxBlockY = g_MapBlockY[g_CurrentMap] - 1;

	int drawOffset = 120;

	m_RenderBounds.MinPixelsX = m_RenderBounds.GameWindowPosX - drawOffset;
	m_RenderBounds.MaxPixelsX = m_RenderBounds.GameWindowPosX + m_RenderBounds.GameWindowSizeX + drawOffset;
	m_RenderBounds.MinPixelsY = m_RenderBounds.GameWindowPosY - drawOffset - playerZOffset;
	m_RenderBounds.MaxPixelsY = m_RenderBounds.GameWindowPosY + m_RenderBounds.GameWindowSizeY + 200 + playerZOffset;

	CreaterRenderList();
}
//---------------------------------------------------------------------------
void TGameScreen::CheckMouseEvents(bool &charSelected)
{
	g_GrayedPixels = g_Player->Dead();

	if (g_StatusbarUnderMouse != g_LastSelectedGump)
		g_StatusbarUnderMouse = 0;
	else
	{
		TGump *gsb = GumpManager->GumpExists(g_StatusbarUnderMouse);

		if (gsb != NULL)
			g_StatusbarUnderMouse = gsb->Serial;
		else
			g_StatusbarUnderMouse = 0;
	}

	if (g_LastObjectType == SOT_GAME_OBJECT && g_LastObjectLeftMouseDown && ((g_MouseX != g_DroppedLeftMouseX || g_MouseY != g_DroppedLeftMouseY) || (g_LastMouseDownTime + DCLICK_DELAY < GetTickCount())))
	{
		TGameItem *selobj = World->FindWorldItem(g_LastObjectLeftMouseDown);

		if (selobj != NULL && ObjectInHand == NULL && !selobj->Locked() && GetDistance(g_Player, selobj) < 3)
		{
			if (selobj->Serial >= 0x40000000 && !g_Player->Dead()) //Item selection
			{
				if (selobj->IsStackable() && selobj->Count > 1)
				{
					TGumpDrag *newgump = new TGumpDrag(g_LastObjectLeftMouseDown, g_MouseX - 80, g_MouseY - 34);
						
					char text_amount[20] = {0};
					sprintf(text_amount, "%d", selobj->Count);

					TGump *gumpEntry = GumpManager->GetTextEntryOwner();
						
					EntryPointer = newgump->TextEntry;
					EntryPointer->SetText(text_amount);

					if (gumpEntry != NULL)
						gumpEntry->UpdateFrame();
	
					GumpManager->AddGump(newgump);
					CurrentScreen->OnLeftMouseDown();
					selobj->Dragged = true;
				}
				else if (!Target.IsTargeting())
				{
					UO->PickupItem(selobj);
					//g_LastGumpLeftMouseDown = 0;
					g_LastObjectLeftMouseDown = 0;
				}
			}
		}
		else if (ObjectInHand == NULL)
		{
			TGameCharacter *selchar = World->FindWorldCharacter(g_LastObjectLeftMouseDown);
			if (selchar!= NULL) //Character selection
			{
				UO->OpenStatus(selchar->Serial);
				CurrentScreen->OnLeftMouseDown();
				g_GeneratedMouseDown = true;
				charSelected = true;
			}
		}
	}
	else if (g_LastObjectType == SOT_GAME_OBJECT) //Character selection
	{
		TGameCharacter *selobj = World->FindWorldCharacter(g_LastSelectedObject);

		if (selobj != NULL && selobj->Serial < 0x40000000)
			charSelected = true;
	}
}
//---------------------------------------------------------------------------
void TGameScreen::AddLight(LIGHT_DATA &light)
{
	if (m_LightCount < MAX_LIGHT_SOURCES)
	{
		memcpy(&m_Light[m_LightCount], &light, sizeof(LIGHT_DATA));

		m_LightCount++;
	}
}
//---------------------------------------------------------------------------
void TGameScreen::RemoveLight(WORD x, WORD y, char z)
{
}
//---------------------------------------------------------------------------
void TGameScreen::CreaterRenderList()
{
	int playerX = g_Player->X;
	int playerY = g_Player->Y;
	int playerZ = g_Player->Z;
	int deltaZ = playerZ * 4;

	int gameWindowPosX = m_RenderBounds.GameWindowPosX;
	int gameWindowPosY = m_RenderBounds.GameWindowPosY;

	int drawModeCount = 2;

	DWORD ticks = GetTickCount();

	if (g_DeathScreenTimer > ticks)
		drawModeCount = 0;
	else
		g_DeathScreenTimer = 0;

	bool noDrawRoof = false;
	int MaxGroundZ = 125;

	int maxDrawZ = GetMaxDrawZ(noDrawRoof, MaxGroundZ);
	m_ListSize = 0;

	IFOR(drawMode, 0, drawModeCount)
	{
		for (int bx = m_RenderBounds.MinBlockX; bx <= m_RenderBounds.MaxBlockX; bx++)
		{
			for (int by = m_RenderBounds.MinBlockY; by <= m_RenderBounds.MaxBlockY; by++)
			{
				int blockIndex = (bx * 512) + by;
				TMapBlock *mb = MapManager->GetBlock(blockIndex);
				if (mb == NULL)
				{
					mb = MapManager->AddBlock(blockIndex);
					mb->X = bx;
					mb->Y = by;
					MapManager->LoadBlock(mb);
				}

				IFOR(x, 0, 8)
				{
					WORD mX = bx * 8 + x;

					if (mX < m_RenderBounds.RealMinRangeX || mX > m_RenderBounds.RealMaxRangeX)
						continue;

					int gx = mX - playerX;

					IFOR(y, 0, 8)
					{
						WORD mY = by * 8 + y;

						if (mY < m_RenderBounds.RealMinRangeY || mY > m_RenderBounds.RealMaxRangeY)
							continue;

						int gy = mY - playerY;

						TRenderWorldObject *ro = mb->GetRender(x, y);

						if (ro == NULL)
							continue;

						int drawPixelsX = m_RenderBounds.GameWindowCenterX + (gx - gy) * 22;
						int drawPixelsY = m_RenderBounds.GameWindowCenterY + (gx + gy) * 22;

						if (drawPixelsX < m_RenderBounds.MinPixelsX || drawPixelsX > m_RenderBounds.MaxPixelsX)
							continue;

						TLandObject *mol = ro->GetLand();

						if (drawPixelsY - deltaZ < m_RenderBounds.MinPixelsY || drawPixelsY - deltaZ > m_RenderBounds.MaxPixelsY)
							continue;

						char checkZ = mol->DrawZ;

						while (ro != NULL)
						{
							TRenderWorldObject *nextRo = ro->m_NextXY;

							bool drawCondition = true;

							if (!drawMode) //��������� ���������, ���� � ���������� ���������� ���������
								drawCondition = (ro->Z < checkZ || ro->RenderType == ROT_LAND_OBJECT);
							else //��������� ��������� ����� ����
								drawCondition = (ro->Z >= checkZ && ro->RenderType != ROT_LAND_OBJECT);

							if (ro->IsInternal())
								drawCondition = false;

							if ((ro->RenderType != ROT_LAND_OBJECT && ro->Z >= maxDrawZ) || !drawCondition)
							{
								ro = nextRo;
								continue;
							}

							bool canDraw = false;

							switch (ro->RenderType)
							{
								case ROT_LAND_OBJECT:
								{
									if (ro->Z > MaxGroundZ)
										break;

									canDraw = true;

									break;
								}
								case ROT_STATIC_OBJECT:
								case ROT_MULTI_OBJECT:
								{
									if (noDrawRoof && ro->IsRoof())
										break;

									canDraw = true;

									break;
								}
								case ROT_GAME_OBJECT:
								{
									if (((TGameObject*)ro)->Container != 0xFFFFFFFF || ro->Graphic >= 0x4000)
										break;

									canDraw = true;

									break;
								}
								case ROT_EFFECT:
								{
									TGameEffect *effect = (TGameEffect*)ro;

									WORD eGraphic = 0;

									if (effect->EffectType == EF_STAY_AT_POS && effect->Duration < ticks)
										EffectManager->RemoveEffect(effect);
									else if (effect->EffectType == EF_DRAG)
									{
										if (effect->Duration < ticks)
											EffectManager->RemoveEffect(effect);
										else
											canDraw = true;
									}
									else
										canDraw = true;

									break;
								}
								default:
									break;
							}

							if (canDraw)
							{
								RENDER_LIST_DATA &data = m_List[m_ListSize];
								data.obj = ro;
								data.DrawX = drawPixelsX;
								data.DrawY = drawPixelsY;

								m_ListSize++;

								if (m_ListSize >= RENDER_LIST_STACK_SIZE)
									return;
							}

							ro = nextRo;
						}
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
int TGameScreen::Render(bool mode)
{
	DWORD ticks = GetTickCount();

	if (g_LastRenderTime > ticks)
	{
		if (mode || !g_SelectGumpObjects)
			return 0;
	}

	int playerX = g_Player->X;
	int playerY = g_Player->Y;
	int playerZ = g_Player->Z;

	int gameWindowPosX = m_RenderBounds.GameWindowPosX;
	int gameWindowPosY = m_RenderBounds.GameWindowPosY;

	int gameWindowSizeX = m_RenderBounds.GameWindowSizeX;
	int gameWindowSizeY = m_RenderBounds.GameWindowSizeY;

	int gameWindowCenterX = m_RenderBounds.GameWindowCenterX;
	int gameWindowCenterY = m_RenderBounds.GameWindowCenterY;
	
	int drawModeCount = 2;

	if (g_DeathScreenTimer > ticks)
		drawModeCount = 0;
	else
		g_DeathScreenTimer = 0;

	//���������� ���������, ������������ � ��������� ������
	TRenderTextObject *rto = WorldTextRenderer->m_Items;

	if (drawModeCount)
	{
		if (mode)
			WorldTextRenderer->ClearRect();

		//���������� ��������� � ������������ ������
		for (; rto != NULL; rto = rto->m_NextDraw)
		{
			if (!rto->IsText())
			{
				if (rto->m_NextDraw == NULL)
					break;

				continue;
			}

			TTextData *td = (TTextData*)rto;
			TTextTexture &tth = td->m_Texture;

			TRenderWorldObject *rwo = NULL;
			int drawX = 0;
			int drawY = 0;

			switch (td->Type)
			{
				case TT_OBJECT:
				{
					TGameObject *go = World->FindWorldObject(td->Serial);

					if (go != NULL && go->Graphic < 0x4000)
					{
						rwo = go;

						int gox = go->X - playerX;
						int goy = go->Y - playerY;

						drawX = gameWindowCenterX + ((gox - goy) * 22);
						drawY = ((gameWindowCenterY + ((gox + goy) * 22)) - (go->Z * 4));

						if (go->NPC)
						{
							TGameCharacter *gc = (TGameCharacter*)go;

							drawX += gc->OffsetX;
							drawY += gc->OffsetY;

							drawY -= 55;

							if (gc->FindLayer(OL_MOUNT) != NULL)
								drawY -= 25;
						}
						else
						{
							WORD gID = go->Graphic;

							int height = UO->m_StaticData[gID / 32].Tiles[gID % 32].Height;

							drawY -= (height + 20);
						}

						drawX -= go->GetTextOffsetX(td);
						drawY -= go->GetTextOffsetY(td);
					}

					break;
				}
				case TT_CLIENT:
				{
					rwo = (TRenderWorldObject*)td->Serial;

					if (rwo != NULL)
					{
						int gox = rwo->X - playerX;
						int goy = rwo->Y - playerY;

						drawX = gameWindowCenterX + ((gox - goy) * 22);
						drawY = ((gameWindowCenterY + ((gox + goy) * 22)) - (rwo->Z * 4));

						WORD gID = rwo->Graphic - 0x4000;

						int height = UO->m_StaticData[gID / 32].Tiles[gID % 32].Height;

						drawY -= (height + 20);

						drawX -= rwo->GetTextOffsetX(td);
						drawY -= rwo->GetTextOffsetY(td);
					}

					break;
				}
				default:
					break;
			}

			if (rwo != NULL)
			{
				td->DrawX = drawX;
				td->DrawY = drawY;

				if (mode)
				{
					TEXT_IMAGE_BOUNDS ib;
					ib.X = drawX;
					ib.Y = drawY;
					ib.Width = drawX + tth.Width;
					ib.Height = drawY + tth.Height;
					ib.m_Text = td;

					td->Transparent = WorldTextRenderer->InRect(ib, rwo);

					WorldTextRenderer->AddRect(ib);
				}
			}

			if (rto->m_NextDraw == NULL)
				break;
		}
	}
	
	int deltaZ = playerZ * 4;

	if (mode)
	{
		static DWORD lastRender = GetTickCount() + 1000;
		static int currentFPS = 0;
		static int FPScount = 0;

		if (lastRender < ticks)
		{
			FPScount = currentFPS;
			currentFPS = 0;
			lastRender = ticks + 1000;
		}
		else
			currentFPS++;

		g_LastRenderTime = ticks + g_FrameDelay;

		bool RMD = false;
		bool LMD = false;
		int LGLMD = 0;
		int LOLMD = 0;

		if (g_GameState == GS_GAME_BLOCKED)
		{
			RMD = g_RightMouseDown;
			g_RightMouseDown = false;
			LMD = g_LeftMouseDown;
			g_LeftMouseDown = false;
			g_LastObjectType = SOT_NO_OBJECT;
			g_LastSelectedObject = 0;

			LGLMD = g_LastGumpLeftMouseDown;
			LOLMD = g_LastObjectLeftMouseDown;

			g_LastObjectLeftMouseDown = 0;
			g_LastObjectRightMouseDown = 0;
			g_LastSelectedGump = 0;
			g_LastGumpLeftMouseDown = 0;
			g_LastGumpRightMouseDown = 0;
		}

		bool multiOnTarget = false;

		if (Target.IsTargeting() && Target.MultiGraphic && g_SelectedObject != NULL)
		{
			multiOnTarget = true;
			int grZ = 0;
			int stZ = 0;
			MapManager->GetMapZ(g_SelectedObject->X, g_SelectedObject->Y, grZ, stZ);
			Target.LoadMulti(g_SelectedObject->X, g_SelectedObject->Y, grZ);
		}

		g_RenderedObjectsCountInGameWindow = 0;
		
		bool charSelected = false;

		CheckMouseEvents(charSelected);
		
		TargetGump.Color = 0;
		AttackTargetGump.Color = 0;

		//bool noDrawRoof = false;
		//int MaxGroundZ = 125;
		m_LightCount = 0;

		//int maxDrawZ = GetMaxDrawZ(noDrawRoof, MaxGroundZ);

		g_GL.BeginDraw();
		
		g_GL.ViewPort(gameWindowPosX, gameWindowPosY, gameWindowSizeX, gameWindowSizeY);
		
		g_GL.Disable(GL_LIGHTING);
		
		g_DrawColor = 1.0f;

		if (!g_UseFrameBuffer && g_PersonalLightLevel < g_LightLevel)
		{
			g_DrawColor = ((32 - g_LightLevel + g_PersonalLightLevel) / 32.0f); // + 0.2f;

			if (!ConfigManager.DarkNights)
				g_DrawColor += 0.2f;
		}

		glColor3f(g_DrawColor, g_DrawColor, g_DrawColor);

		IFOR(i, 0, m_ListSize)
		{
			/*g_OutOfRangeColor = 0;
			POINT testPos = {mX, mY};

			if (GetDistance(g_Player, testPos) > g_UpdateRange)
			g_OutOfRangeColor = 0x0386;*/

			m_List[i].obj->Draw(mode, m_List[i], ticks);

			/*TRenderWorldObject *obj = m_List[i].obj;
			int drawX = m_List[i].DrawX;
			int drawY = m_List[i].DrawY;

			WORD objGraphic = obj->Graphic;
			WORD objColor = obj->Color;
			char objZ = obj->Z;

			switch (obj->RenderType)
			{
				case ROT_LAND_OBJECT:
				{
					objColor = 0;

					if (obj == g_SelectedObject)
						objColor = g_SelectLandColor;

#if UO_DEBUG_INFO!=0
					g_RenderedObjectsCountInGameWindow++;
#endif

					if (obj->Color == 1)
						UO->DrawLandArt(objGraphic, objColor, drawX, drawY, objZ);
					else
					{
						GLfloat tCl = 1.0f;
						glColor3f(tCl, tCl, tCl);

						glEnable(GL_LIGHTING);

						UO->DrawLandTexture(objGraphic, objColor, drawX, drawY, ((TLandObject*)obj)->Rect, ((TLandObject*)obj)->m_Normals);

						glDisable(GL_LIGHTING);

						glColor3f(drawColor, drawColor, drawColor);
					}

					break;
				}
				case ROT_STATIC_OBJECT:
				{
#if UO_DEBUG_INFO!=0
					g_RenderedObjectsCountInGameWindow++;
#endif

					objGraphic -= 0x4000;

					if (obj == g_SelectedObject)
						objColor = g_SelectStaticColor;

					if (obj->IsFoliage() && playerX - 2 < obj->X && playerY - 2 < obj->Y)
					{
						if (!g_GrayedPixels)
						{
							POINT fp = { 0, 0 };
							UO->GetArtDimension(obj->Graphic, fp);

							IMAGE_BOUNDS fib = { drawX - fp.x / 2, drawY - fp.y - (objZ * 4), fp.x, fp.y };

							//g_GL.DrawPolygone(0x7F7F7F7F, fib.X, fib.Y, fib.Width, fib.Height);

							if (fib.InRect(g_PlayerRect))
							{
								glEnable(GL_BLEND);
								glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);

								UO->DrawStaticArtAnimated(objGraphic, objColor, drawX, drawY, objZ);

								glDisable(GL_BLEND);
							}
							else
								UO->DrawStaticArtAnimated(objGraphic, objColor, drawX, drawY, objZ);
						}
					}
					else
						UO->DrawStaticArtAnimated(objGraphic, objColor, drawX, drawY, objZ);

					if (obj->IsLightSource() && !obj->IsWall())
					{
						STATIC_TILES &tile = UO->m_StaticData[objGraphic / 32].Tiles[objGraphic % 32];

						LIGHT_DATA light = { tile.Quality, 0, obj->X, obj->Y, objZ, drawX, drawY - (objZ * 4) };

						if (ConfigManager.ColoredLighting)
							light.Color = UO->GetLightColor(objGraphic);

						AddLight(light);
					}

					break;
				}
				case ROT_GAME_OBJECT:
				{
					TGameObject *go = (TGameObject*)obj;

					if (!go->NPC)
					{
						if (go->Hidden())
							objColor = 0x038A;

#if UO_DEBUG_INFO!=0
						g_RenderedObjectsCountInGameWindow++;
#endif

						if (go->IsCorpse()) //������
							AnimationManager->DrawCorpse((TGameItem*)obj, drawX, drawY, objZ);
						else
						{
							bool doubleDraw = false;
							bool selMode = false;

							if (g_LastObjectType == SOT_GAME_OBJECT && !go->Locked() && g_LastSelectedObject == go->Serial)
							{
								objColor = 0x0035;
								selMode = true;
							}

							int objCount = go->Count;

							if (objCount > 1)
							{
								if (objGraphic == 0x0EED)
								{
									if (objCount > 5)
										objGraphic = 0x0EEF;
									else
										objGraphic = 0x0EEE;
								}
								else if (((TGameItem*)go)->IsStackable())
									doubleDraw = true;
							}

							if (doubleDraw)
							{
								drawX -= 2;
								UO->DrawStaticArt(objGraphic, objColor, drawX, drawY - 5, objZ, selMode);
								UO->DrawStaticArt(objGraphic, objColor, drawX + 5, drawY, objZ, selMode);
							}
							else
								UO->DrawStaticArtAnimated(objGraphic, objColor, drawX, drawY, objZ, selMode);

							if (obj->IsLightSource())
							{
								STATIC_TILES &tile = UO->m_StaticData[objGraphic / 32].Tiles[objGraphic % 32];

								LIGHT_DATA light = { tile.Quality, tile.Hue, obj->X, obj->Y, objZ, drawX, drawY - (objZ * 4) };

								if (ConfigManager.ColoredLighting)
									light.Color = UO->GetLightColor(objGraphic);

								AddLight(light);
							}
						}
					}
					else //if (go->NPC)
					{
						TGameCharacter *goc = (TGameCharacter*)go;
						if (goc->TimeToRandomFidget < ticks)
							goc->SetRandomFidgetAnimation();

#if UO_DEBUG_INFO!=0
						g_RenderedObjectsCountInGameWindow++;
#endif

						DWORD lastSBsel = g_StatusbarUnderMouse;

						if (!go->IsPlayer() && g_Player->Warmode && g_LastSelectedObject == go->Serial)
							g_StatusbarUnderMouse = go->Serial;

						AnimationManager->DrawCharacter(goc, drawX, drawY, objZ); //Draw character

						g_StatusbarUnderMouse = lastSBsel;

						/*if (goc->IsPlayer())
						{
						//g_GL.DrawPolygone(0x7F7F7F7F, g_PlayerRect.X, g_PlayerRect.Y, g_PlayerRect.Width, g_PlayerRect.Height);
						}*/
					/*}

					go->DrawEffects(drawX, drawY, ticks);

					break;
				}
				case ROT_MULTI_OBJECT:
				{
#if UO_DEBUG_INFO!=0
					g_RenderedObjectsCountInGameWindow++;
#endif

					TMultiObject *multi = (TMultiObject*)obj;

					objGraphic -= 0x4000;

					if (multi == g_SelectedObject)
						objColor = g_SelectMultiColor;

					if (multi->MultiFlags == 2) //������ �� �������
					{
						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);

						UO->DrawStaticArt(objGraphic, objColor, drawX, drawY, objZ);

						glDisable(GL_BLEND);
					}
					else
					{
						UO->DrawStaticArt(objGraphic, objColor, drawX, drawY, objZ);

						if (obj->IsLightSource())
						{
							STATIC_TILES &tile = UO->m_StaticData[objGraphic / 32].Tiles[objGraphic % 32];

							LIGHT_DATA light = { tile.Quality, tile.Hue, obj->X, obj->Y, objZ, drawX, drawY - (objZ * 4) };

							if (ConfigManager.ColoredLighting)
								light.Color = UO->GetLightColor(objGraphic);

							AddLight(light);
						}
					}

					break;
				}
				case ROT_EFFECT:
				{
					TGameEffect *effect = (TGameEffect*)obj;

#if UO_DEBUG_INFO!=0
					g_RenderedObjectsCountInGameWindow++;
#endif

					objGraphic = 0;

					if (effect->EffectType == EF_STAY_AT_POS && effect->Duration < ticks)
						EffectManager->RemoveEffect(effect);
					else if (effect->EffectType == EF_DRAG)
					{
						if (effect->Duration < ticks)
							EffectManager->RemoveEffect(effect);
						else
						{
							TGameEffectDrag *ed = (TGameEffectDrag*)effect;

							int deX = drawX - ed->OffsetX;
							int deY = drawY - ed->OffsetY;

							UO->DrawStaticArt(effect->Graphic, objColor, deX, deY, objZ);

							ed->AddOffsetX(10);
							ed->AddOffsetY(10);
						}
					}
					else if (effect->LastChangeFrameTime < ticks)
					{
						effect->LastChangeFrameTime = ticks + effect->Speed;

						objGraphic = effect->CalculateCurrentGraphic();
					}
					else
						objGraphic = effect->GetCurrentGraphic();

					if (objGraphic)
					{
						int deX = drawX;
						int deY = drawY;
						int deZ = 0;

						if (effect->EffectType == EF_MOVING)
						{
							TGameEffectMoving *moving = (TGameEffectMoving*)effect;

							deX += moving->OffsetX;
							deY += moving->OffsetY;
							deZ += moving->OffsetZ;
						}

						effect->ApplyRenderMode();

						UO->DrawStaticArt(objGraphic, objColor, deX, deY, objZ + deZ);

						glDisable(GL_BLEND);
					}

					break;
				}
				default:
					break;
			}*/
		}

		/*IFOR(drawMode, 0, drawModeCount)
		{
			for (int bx = m_RenderBounds.MinBlockX; bx <= m_RenderBounds.MaxBlockX; bx++)
			{
				for (int by = m_RenderBounds.MinBlockY; by <= m_RenderBounds.MaxBlockY; by++)
				{
					int blockIndex = (bx * 512) + by;
					TMapBlock *mb = MapManager->GetBlock(blockIndex);
					if (mb == NULL)
					{
						mb = MapManager->AddBlock(blockIndex);
						mb->X = bx;
						mb->Y = by;
						MapManager->LoadBlock(mb);
					}

					IFOR(x, 0, 8)
					{
						WORD mX = bx * 8 + x;

						if (mX < m_RenderBounds.RealMinRangeX || mX > m_RenderBounds.RealMaxRangeX)
							continue;

						int gx = mX - playerX;

						IFOR(y, 0, 8)
						{
							WORD mY = by * 8 + y;

							if (mY < m_RenderBounds.RealMinRangeY || mY > m_RenderBounds.RealMaxRangeY)
								continue;
						
							int gy = mY - playerY;

							TRenderWorldObject *ro = mb->GetRender(x, y);

							if (ro == NULL)
								continue;

							int DrawPixelsX = gameWindowCenterX + (gx - gy) * 22;
							int DrawPixelsY = gameWindowCenterY + (gx + gy) * 22;

							if (DrawPixelsX < m_RenderBounds.MinPixelsX || DrawPixelsX > m_RenderBounds.MaxPixelsX)
								continue;

							TLandObject *mol = ro->GetLand();

							char TileZ1 = mol->Z;

							if (DrawPixelsY - deltaZ < m_RenderBounds.MinPixelsY || DrawPixelsY - deltaZ > m_RenderBounds.MaxPixelsY)
								continue;

							/*g_OutOfRangeColor = 0;
							POINT testPos = {mX, mY};

							if (GetDistance(g_Player, testPos) > g_UpdateRange)
								g_OutOfRangeColor = 0x0386;*/

							/*char checkZ = mol->DrawZ;
						
							while (ro != NULL)
							{
								TRenderWorldObject *nextRo = ro->m_NextXY;

								bool drawCondition = true;

								if (!drawMode) //��������� ���������, ���� � ���������� ���������� ���������
									drawCondition = (ro->Z < checkZ || ro->RenderType == ROT_LAND_OBJECT);
								else //��������� ��������� ����� ����
									drawCondition = (ro->Z >= checkZ && ro->RenderType != ROT_LAND_OBJECT);

								if (ro->IsInternal())
									drawCondition = false;

								if ((ro->RenderType != ROT_LAND_OBJECT && ro->Z >= maxDrawZ) || !drawCondition)
								{
									ro = nextRo;
									continue;
								}

								switch (ro->RenderType)
								{
									case ROT_LAND_OBJECT:
									{
										if (TileZ1 > MaxGroundZ)
											break;

										WORD color = 0;
										if (ro == g_SelectedObject)
											color = g_SelectLandColor;

#if UO_DEBUG_INFO!=0
g_RenderedObjectsCountInGameWindow++;
#endif

										if (ro->Color == 1)
											UO->DrawLandArt(ro->Graphic, color, DrawPixelsX, DrawPixelsY, TileZ1);
										else
										{
											GLfloat tCl = 1.0f;
											glColor3f(tCl, tCl, tCl);
											
											glEnable(GL_LIGHTING);

											UO->DrawLandTexture(ro->Graphic, color, DrawPixelsX, DrawPixelsY, ((TLandObject*)ro)->Rect, ((TLandObject*)ro)->m_Normals);
											
											glDisable(GL_LIGHTING);

											glColor3f(g_DrawColor, g_DrawColor, g_DrawColor);
										}

										break;
									}
									case ROT_STATIC_OBJECT:
									{
										if (noDrawRoof && ro->IsRoof())
											break;

#if UO_DEBUG_INFO!=0
g_RenderedObjectsCountInGameWindow++;
#endif

										WORD sG = ro->Graphic - 0x4000;
									
										WORD color = ro->Color;
										if (ro == g_SelectedObject)
											color = g_SelectStaticColor;
									
										if (ro->IsFoliage() && playerX - 2 < mX && playerY - 2 < mY)
										{
											if (!g_GrayedPixels)
											{
												POINT fp = {0, 0};
												UO->GetArtDimension(ro->Graphic, fp);

												IMAGE_BOUNDS fib = {DrawPixelsX - fp.x / 2, DrawPixelsY - fp.y - (ro->Z * 4), fp.x, fp.y};

												//g_GL.DrawPolygone(0x7F7F7F7F, fib.X, fib.Y, fib.Width, fib.Height);

												if (fib.InRect(g_PlayerRect))
												{
													glEnable(GL_BLEND);
													glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
										
													UO->DrawStaticArtAnimated(sG, color, DrawPixelsX, DrawPixelsY, ro->Z);

													glDisable(GL_BLEND);
												}
												else
													UO->DrawStaticArtAnimated(sG, color, DrawPixelsX, DrawPixelsY, ro->Z);
											}
										}
										else
											UO->DrawStaticArtAnimated(sG, color, DrawPixelsX, DrawPixelsY, ro->Z);

										if (ro->IsLightSource() && !ro->IsWall())
										{
											STATIC_TILES &tile = UO->m_StaticData[sG / 32].Tiles[sG % 32];

											LIGHT_DATA light = {tile.Quality, 0, mX, mY, ro->Z, DrawPixelsX, DrawPixelsY - (ro->Z * 4)};

											if (ConfigManager.ColoredLighting)
												light.Color = UO->GetLightColor(sG);

											AddLight(light);
										}

										break;
									}
									case ROT_GAME_OBJECT:
									{
										TGameObject *go = (TGameObject*)ro;
									
										WORD goGraphic = go->Graphic;
										char goZ = go->Z;

										if (go->Container != 0xFFFFFFFF || goGraphic >= 0x4000)
											break;
										else if (!go->NPC)
										{
											WORD goColor = go->Color;

											if (go->Hidden())
												goColor = 0x038A;

#if UO_DEBUG_INFO!=0
g_RenderedObjectsCountInGameWindow++;
#endif

											if (go->IsCorpse()) //������
												AnimationManager->DrawCorpse((TGameItem*)go, DrawPixelsX, DrawPixelsY, goZ);
											else
											{
												bool DoubleDraw = false;
												bool sel_mode = false;

												if (g_LastObjectType == SOT_GAME_OBJECT && !go->Locked() && g_LastSelectedObject == go->Serial)
												{
													goColor = 0x0035;
													sel_mode = true;
												}

												int goCount = go->Count;

												if (goCount > 1)
												{
													if (goGraphic == 0x0EED)
													{
														if (goCount > 5)
															goGraphic = 0x0EEF;
														else
															goGraphic = 0x0EEE;
													}
													else if (((TGameItem*)go)->IsStackable())
														DoubleDraw = true;
												}

												if (DoubleDraw)
												{
													DrawPixelsX -= 2;
													UO->DrawStaticArt(goGraphic, goColor, DrawPixelsX, DrawPixelsY - 5, goZ, sel_mode);
													UO->DrawStaticArt(goGraphic, goColor, DrawPixelsX + 5, DrawPixelsY, goZ, sel_mode);
												}
												else
													UO->DrawStaticArtAnimated(goGraphic, goColor, DrawPixelsX, DrawPixelsY, goZ, sel_mode);

												if (ro->IsLightSource())
												{
													STATIC_TILES &tile = UO->m_StaticData[goGraphic / 32].Tiles[goGraphic % 32];

													LIGHT_DATA light = {tile.Quality, tile.Hue, mX, mY, ro->Z, DrawPixelsX, DrawPixelsY - (ro->Z * 4)};

													if (ConfigManager.ColoredLighting)
														light.Color = UO->GetLightColor(goGraphic);

													AddLight(light);
												}
											}
										}
										else //if (go->NPC)
										{
											TGameCharacter *goc = (TGameCharacter*)go;
											if (goc->TimeToRandomFidget < ticks)
												goc->SetRandomFidgetAnimation();

#if UO_DEBUG_INFO!=0
g_RenderedObjectsCountInGameWindow++;
#endif

											DWORD lastSBsel = g_StatusbarUnderMouse;

											if (!go->IsPlayer() && g_Player->Warmode && g_LastSelectedObject == go->Serial)
												g_StatusbarUnderMouse = go->Serial;

											AnimationManager->DrawCharacter(goc, DrawPixelsX, DrawPixelsY, goZ); //Draw character

											g_StatusbarUnderMouse = lastSBsel;

											/*if (goc->IsPlayer())
											{
												//g_GL.DrawPolygone(0x7F7F7F7F, g_PlayerRect.X, g_PlayerRect.Y, g_PlayerRect.Width, g_PlayerRect.Height);
											}*/
										/*}

										go->DrawEffects(DrawPixelsX, DrawPixelsY, ticks);

										break;
									}
									case ROT_MULTI_OBJECT:
									{
										if (noDrawRoof && ro->IsRoof())
											break;

#if UO_DEBUG_INFO!=0
g_RenderedObjectsCountInGameWindow++;
#endif

										TMultiObject *multi = (TMultiObject*)ro;
									
										char sZ = multi->Z;
								
										WORD sG = multi->Graphic - 0x4000;
									
										WORD color = multi->Color;
										if (multi == g_SelectedObject)
											color = g_SelectMultiColor;

										if (multi->MultiFlags == 2) //������ �� �������
										{
											glEnable(GL_BLEND);
											glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);

											UO->DrawStaticArt(sG, color, DrawPixelsX, DrawPixelsY, sZ);

											glDisable(GL_BLEND);
										}
										else
										{
											UO->DrawStaticArt(sG, color, DrawPixelsX, DrawPixelsY, sZ);
											
											if (ro->IsLightSource())
											{
												STATIC_TILES &tile = UO->m_StaticData[sG / 32].Tiles[sG % 32];

												LIGHT_DATA light = {tile.Quality, tile.Hue, mX, mY, ro->Z, DrawPixelsX, DrawPixelsY - (ro->Z * 4)};

												if (ConfigManager.ColoredLighting)
													light.Color = UO->GetLightColor(sG);

												AddLight(light);
											}
										}
									
										break;
									}
									case ROT_EFFECT:
									{
										TGameEffect *effect = (TGameEffect*)ro;

#if UO_DEBUG_INFO!=0
g_RenderedObjectsCountInGameWindow++;
#endif

										char eZ = effect->Z;
								
										WORD eGraphic = 0;

										if (effect->EffectType == EF_STAY_AT_POS && effect->Duration < ticks)
											EffectManager->RemoveEffect(effect);
										else if (effect->EffectType == EF_DRAG)
										{
											if (effect->Duration < ticks)
												EffectManager->RemoveEffect(effect);
											else
											{
												TGameEffectDrag *ed = (TGameEffectDrag*)effect;
												
												int deX = DrawPixelsX - ed->OffsetX;
												int deY = DrawPixelsY - ed->OffsetY;

												UO->DrawStaticArt(effect->Graphic, effect->Color, deX, deY, eZ);

												ed->AddOffsetX(10);
												ed->AddOffsetY(10);
											}
										}
										else if (effect->LastChangeFrameTime < ticks)
										{
											effect->LastChangeFrameTime = ticks + effect->Speed;

											eGraphic = effect->CalculateCurrentGraphic();
										}
										else
											eGraphic = effect->GetCurrentGraphic();

										if (eGraphic)
										{
											int deX = DrawPixelsX;
											int deY = DrawPixelsY;
											int deZ = 0;

											if (effect->EffectType == EF_MOVING)
											{
												TGameEffectMoving *moving = (TGameEffectMoving*)effect;
												
												deX += moving->OffsetX;
												deY += moving->OffsetY;
												deZ += moving->OffsetZ;
											}

											effect->ApplyRenderMode();

											UO->DrawStaticArt(eGraphic, effect->Color, deX, deY, eZ + deZ);

											glDisable(GL_BLEND);
										}

										break;
									}
									default:
										break;
								}

								ro = nextRo;
							}
						}
					}
				}
			}
		}*/

		if (drawModeCount)
		{
			glColor3f(1.0f, 1.0f, 1.0f);

			if (g_UseFrameBuffer && g_PersonalLightLevel < g_LightLevel)
			{
				bool lightReady = m_LightBuffer.Ready(gameWindowSizeX, gameWindowSizeY);

				if (!lightReady)
				{
					m_LightBuffer.Free();
					
					lightReady = m_LightBuffer.Init(gameWindowSizeX, gameWindowSizeY);

					if (!lightReady)
						m_LightBuffer.Free();
				}
				
				float newLightColor = ((32 - g_LightLevel + g_PersonalLightLevel) / 32.0f); // + 0.2f;

				if (!ConfigManager.DarkNights)
					newLightColor += 0.2f;

				if (lightReady)
				{
					if (m_LightBuffer.Use())
					{
						glClearColor(newLightColor, newLightColor, newLightColor, 1.0f);
						glClear(GL_COLOR_BUFFER_BIT);

						glEnable(GL_BLEND);
						glBlendFunc(GL_ONE, GL_ONE);

						IFOR(i, 0, m_LightCount)
						{
							LIGHT_DATA &light = m_Light[i];

							UO->DrawLight(light.ID, light.Color, light.DrawX, light.DrawY);
						}

						m_LightBuffer.Release();

						g_GL.RestorePort();

						glBlendFunc(GL_ZERO, GL_SRC_COLOR);
						
						m_LightBuffer.Draw((float)gameWindowPosX, (float)gameWindowPosY);
				
						glDisable(GL_BLEND);
					}
				}
			}

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			g_GL.ViewPort(gameWindowPosX, gameWindowPosY, gameWindowSizeX, gameWindowSizeY);

			g_OutOfRangeColor = 0;

			g_GrayedPixels = false;

			TargetGump.Draw();
			AttackTargetGump.Draw();

			QuestArrow.Draw(gameWindowCenterX, gameWindowCenterY);

			Weather.Draw(gameWindowPosX, gameWindowPosY);

			//��������� ��������� ������ ����
			int TextOffsY = (gameWindowPosY + gameWindowSizeY) - 41;
			int TextYBounds = gameWindowPosY;

			TTextData *td = SystemChat->m_Head;
			while (td != NULL)
			{
				if (TextOffsY < TextYBounds)
					break;

				//if (td->GetType() == TT_SYSTEM)
				{
					TTextTexture &tth = td->m_Texture;

					TextOffsY -= tth.Height;

					if (td->Timer >= ticks)
						tth.Draw(gameWindowPosX, TextOffsY);
				}

				td = td->m_Prev;
			}

			//��������� ������
			for (; rto != NULL; rto = rto->m_PrevDraw)
			{
				if (!rto->IsText())
					continue;

				TTextData *td = (TTextData*)rto;
				
				if (td->Type != TT_SYSTEM)
				{
					TTextTexture &tth = td->m_Texture;

					if (td->Timer >= ticks)
					{
						//g_GL.DrawPolygone(0x7f7f7f7f, td->DrawX, td->DrawY, tth.Width, tth.Height);

						if (g_LastObjectType == SOT_TEXT_OBJECT && g_SelectedTextObject == rto)
							tth.DrawYellow(td->DrawX, td->DrawY);
						else
						{
							if (td->Transparent)
							{
								glEnable(GL_BLEND);
								glBlendFunc(GL_ONE, GL_DST_COLOR);

								tth.Draw(td->DrawX, td->DrawY);

								glDisable(GL_BLEND);
							}
							else
								tth.Draw(td->DrawX, td->DrawY);
						}
					}
				}
			}
		}
		else
		{
			FontManager->DrawA(3, "You are dead.", 0, gameWindowCenterX - 30, gameWindowCenterY);
		}

		//��������������� ������� �������� �������
		g_GL.RestorePort();

#pragma region GameWindowScope
		//����� �������� ����
		UO->DrawGump(0x0A8D, 0, gameWindowPosX - 4, gameWindowPosY - 4, 0, gameWindowSizeY + 8);
		UO->DrawGump(0x0A8D, 0, gameWindowPosX + gameWindowSizeX, gameWindowPosY - 4, 0, gameWindowSizeY + 8);

		UO->DrawGump(0x0A8C, 0, gameWindowPosX - 4, gameWindowPosY - 4, gameWindowSizeX + 4, 0);
		UO->DrawGump(0x0A8C, 0, gameWindowPosX - 4, gameWindowPosY + gameWindowSizeY, gameWindowSizeX + 8, 0);

		//�������� / �����
		WORD resizeGumpID = 0x0837; //button
		if (ConfigManager.LockResizingGameWindow)
			 resizeGumpID = 0x082C; //lock
		else if (g_LastObjectType == SOT_GAME_GUMP_SCOPE && g_LastObjectLeftMouseDown == 2)
			resizeGumpID++; //lighted button

		UO->DrawGump(resizeGumpID, 0, gameWindowPosX + gameWindowSizeX - 3, gameWindowPosY + gameWindowSizeY - 3);
#pragma endregion
		
		char dbf[150] = {0};
		if (charSelected)
			sprintf(dbf, "FPS=%i Dir=%i Z=%i CS", FPScount, g_Player->Direction, playerZ);
		else
			sprintf(dbf, "FPS=%i Dir=%i Z=%i", FPScount, g_Player->Direction, playerZ);

		FontManager->DrawA(3, dbf, 0x35, 20, 50);

#if UO_DEBUG_INFO!=0
		sprintf(dbf, "Rendered %i object counts:\nLand=%i Statics=%i Game=%i Multi=%i",
			g_RenderedObjectsCountInGameWindow, g_LandObjectsCount, g_StaticsObjectsCount, g_GameObjectsCount, g_MultiObjectsCount);

		FontManager->DrawA(3, dbf, 0x35, 20, 74);

		if (g_SelectedObject != NULL)
		{
			char soName[20] = "UnknownObject";

			switch (g_SelectedObject->RenderType)
			{
				case ROT_LAND_OBJECT:
				{
					if (g_SelectedObject->Color == 1)
						sprintf(soName, "Land");
					else
						sprintf(soName, "LandTex");
					break;
				}
				case ROT_STATIC_OBJECT:
				{
					sprintf(soName, "Static");
					break;
				}
				case ROT_GAME_OBJECT:
				{
					sprintf(soName, "GameObject");
					break;
				}
				case ROT_MULTI_OBJECT:
				{
					sprintf(soName, "Multi");
					break;
				}
				default:
					break;
			}

			sprintf(dbf, "Selected:\n%s: Z=%i RQI=%i (SUM=%i)", soName, g_SelectedObject->Z, g_SelectedObject->RenderQueueIndex, g_SelectedObject->Z + g_SelectedObject->RenderQueueIndex);

			FontManager->DrawA(3, dbf, 0x35, 20, 122);
		}
#endif //UO_DEBUG_INFO!=0
		
		GumpManager->Draw(mode, false);
		
		// ��������� ����� ������
		GameConsole->DrawW((BYTE)ConfigManager.SpeechFont, ConfigManager.SpeechColor, gameWindowPosX, gameWindowPosY + gameWindowSizeY - 18, TS_LEFT, UOFONT_BLACK_BORDER | UOFONT_FIXED);
		
		if (multiOnTarget)
			Target.UnloadMulti();
		
		/*static DWORD times = GetTickCount();
		static int mi = 0;
		g_GL.Draw(g_MapTexture[mi], 0, 0, g_MapSizeX[mi] / 10, g_MapSizeY[mi] / 10);

		if (times < ticks)
		{
			times = ticks + 1000;
			mi = (mi + 1) % 2;
		}*/

		if (g_GameState == GS_GAME_BLOCKED)
		{
			g_RightMouseDown = RMD;
			g_LeftMouseDown = LMD;
			g_LastObjectLeftMouseDown = LOLMD;
			g_LastGumpLeftMouseDown = LGLMD;

			GameBlockedScreen->Render(false);
			GameBlockedScreen->Render(true);
		}
		else
		{
			if (ObjectInHand != NULL)
			{
				WORD ohGraphic = ObjectInHand->Graphic;
				WORD ohColor = ObjectInHand->Color;
				WORD ohCount = ObjectInHand->DragCount;

				if (ObjectInHand->IsGameFigure)
				{
					TTextureObject *to = UO->ExecuteGump(ohGraphic - GAME_FIGURE_GUMP_OFFSET);

					if (to != NULL)
						UO->DrawGump(ohGraphic - GAME_FIGURE_GUMP_OFFSET, ohColor, g_MouseX - (to->Width / 2), g_MouseY - (to->Height / 2));
				}
				else
				{
					bool doubleDraw = false;

					if (ohCount > 1)
					{
						if (ohGraphic == 0x0EED)
						{
							if (ohCount > 5)
								ohGraphic = 0x0EEF;
							else
								ohGraphic = 0x0EEE;
						}
						else if (ObjectInHand->IsStackable())
							doubleDraw = true;
					}

					UO->DrawStaticArtInContainer(ohGraphic, ohColor, g_MouseX, g_MouseY, false, true);

					if (doubleDraw)
						UO->DrawStaticArtInContainer(ohGraphic, ohColor, g_MouseX + 5, g_MouseY + 5, false, true);
				}
			}

			InitTooltip();

			MouseManager.Draw(MouseManager.GetGameCursor()); //Game Gump mouse cursor
		}

		g_GL.EndDraw();
	}
	else //����� ��������
	{
		g_LastSelectedObject = 0;
		g_LastObjectType = SOT_NO_OBJECT;
		g_LastSelectedGump = 0;
		g_StatusbarUnderMouse = 0;
		
		GumpManager->Draw(mode, false);

		if (g_LastSelectedGump != 0)
		{
			if (g_LastObjectType == SOT_TEXT_OBJECT)
				g_SelectedTextObject->ToTop();

			return g_LastSelectedObject;
		}
		
		//���� ������ �� ��������� - ��������� �� �����
		if (UO->GumpPixelsInXY(0x0A8D, gameWindowPosX - 4, gameWindowPosY - 4, 0, gameWindowSizeY + 8))
		{
			g_LastObjectType = SOT_GAME_GUMP_SCOPE;
			g_LastSelectedObject = 1;
		}
		else if (UO->GumpPixelsInXY(0x0A8D, gameWindowPosX + gameWindowSizeX, gameWindowPosY - 4, 0, gameWindowSizeY + 8))
		{
			g_LastObjectType = SOT_GAME_GUMP_SCOPE;
			g_LastSelectedObject = 1;
		}
		else if (UO->GumpPixelsInXY(0x0A8C, gameWindowPosX - 4, gameWindowPosY - 4, gameWindowSizeX + 8, 0))
		{
			g_LastObjectType = SOT_GAME_GUMP_SCOPE;
			g_LastSelectedObject = 1;
		}
		else if (UO->GumpPixelsInXY(0x0A8C, gameWindowPosX - 4, gameWindowPosY + gameWindowSizeY, gameWindowSizeX + 8, 0))
		{
			g_LastObjectType = SOT_GAME_GUMP_SCOPE;
			g_LastSelectedObject = 1;
		}
		
		if (!ConfigManager.LockResizingGameWindow)
		{
			if (UO->GumpPixelsInXY(0x0837, gameWindowPosX + gameWindowSizeX - 3, gameWindowPosY + gameWindowSizeY - 3))
			{
				g_LastObjectType = SOT_GAME_GUMP_SCOPE;
				g_LastSelectedObject = 2; //Button
			}
		}
		
		if (g_LastObjectType == SOT_NO_OBJECT) //���� ������ �� ��������� - ��������� �� ��������
		{
			//���� ������ ���� � ������� ���� - ������������ ���
			if (g_MouseX < gameWindowPosX || g_MouseY < gameWindowPosY || g_MouseX > (gameWindowPosX + gameWindowSizeX) || g_MouseY > (gameWindowPosY + gameWindowSizeY))
				return g_LastSelectedObject;

			IFOR(i, 0, m_ListSize)
				m_List[i].obj->Draw(mode, m_List[i], ticks);

			/*bool noDrawRoof = false;
			int MaxGroundZ = 0xFF;
			int maxDrawZ = GetMaxDrawZ(noDrawRoof, MaxGroundZ);

			IFOR(drawMode, 0, drawModeCount)
			{
				for (int bx = m_RenderBounds.MinBlockX; bx <= m_RenderBounds.MaxBlockX; bx++)
				{
					for (int by = m_RenderBounds.MinBlockY; by <= m_RenderBounds.MaxBlockY; by++)
					{
						int blockIndex = (bx * 512) + by;
						TMapBlock *mb = MapManager->GetBlock(blockIndex);
						if (mb == NULL)
						{
							continue;
						}

						IFOR(x, 0, 8)
						{
							WORD mX = bx * 8 + x;
							if (mX < m_RenderBounds.RealMinRangeX || mX > m_RenderBounds.RealMaxRangeX)
								continue;

							int gx = mX - playerX;

							IFOR(y, 0, 8)
							{
								WORD mY = by * 8 + y;

								if (mY < m_RenderBounds.RealMinRangeY || mY > m_RenderBounds.RealMaxRangeY)
									continue;
						
								int gy = mY - playerY;

								TRenderWorldObject *ro = mb->GetRender(x, y);

								if (ro == NULL)
									continue;

								int DrawPixelsX = gameWindowCenterX + (gx - gy) * 22;
								int DrawPixelsY = gameWindowCenterY + (gx + gy) * 22;
				
								if (DrawPixelsX < m_RenderBounds.MinPixelsX || DrawPixelsX > m_RenderBounds.MaxPixelsX)
									continue;

								TLandObject *mol = ro->GetLand();
						
								char TileZ1 = mol->Z;

								if (DrawPixelsY - deltaZ < m_RenderBounds.MinPixelsY || DrawPixelsY - deltaZ > m_RenderBounds.MaxPixelsY)
									continue;
						
								char checkZ = mol->DrawZ;

								for (; ro != NULL; ro = ro->m_NextXY)
								{
									bool drawCondition = true;

									if (!drawMode)
										drawCondition = (ro->Z < checkZ || ro->RenderType == ROT_LAND_OBJECT);
									else
										drawCondition = (ro->Z >= checkZ && ro->RenderType != ROT_LAND_OBJECT);

									if ((ro->RenderType != ROT_LAND_OBJECT && ro->Z >= maxDrawZ) || !drawCondition)
										continue;
									
									if (ro->IsInternal())
										continue;

									switch (ro->RenderType)
									{
										case ROT_LAND_OBJECT:
										{
											if (TileZ1 > MaxGroundZ)
												break;

											if (UO->LandPixelsInXY(ro->Graphic, DrawPixelsX, DrawPixelsY, TileZ1))
											{
												g_LastObjectType = SOT_LAND_OBJECT;
												g_LastSelectedObject = 2;
												g_SelectedObject = ro;
											}

											break;
										}
										case ROT_STATIC_OBJECT:
										{
											if (noDrawRoof && ro->IsRoof())
												break;

											if (ro->IsFoliage() && playerX - 2 < mX && playerY - 2 < mY)
											{
												if (!g_GrayedPixels)
												{
													POINT fp = {0, 0};
													UO->GetArtDimension(ro->Graphic, fp);

													IMAGE_BOUNDS fib = {DrawPixelsX - fp.x / 2, DrawPixelsY - fp.y - (ro->Z * 4), fp.x, fp.y};

													if (!fib.InRect(g_PlayerRect))
													{
														if (UO->StaticPixelsInXYAnimated(ro->Graphic - 0x4000, DrawPixelsX, DrawPixelsY, ro->Z))
														{
															g_LastObjectType = SOT_STATIC_OBJECT;
															g_LastSelectedObject = 3;
															g_SelectedObject = ro;
														}
													}
												}
											}
											else if (UO->StaticPixelsInXYAnimated(ro->Graphic - 0x4000, DrawPixelsX, DrawPixelsY, ro->Z))
											{
												g_LastObjectType = SOT_STATIC_OBJECT;
												g_LastSelectedObject = 3;
												g_SelectedObject = ro;
											}

											break;
										}
										case ROT_GAME_OBJECT:
										{
											TGameObject *go = (TGameObject*)ro;
									
											WORD goGraphic = go->Graphic;
											char goZ = go->Z;

											if (go->Container != 0xFFFFFFFF || goGraphic >= 0x4000)
												break;
											else if (!go->NPC)
											{
												bool DoubleDraw = false;

												int goCount = go->Count;

												if (goCount > 1)
												{
													if (goGraphic == 0x0EED)
													{
														if (goCount > 5)
															goGraphic = 0x0EEF;
														else
															goGraphic = 0x0EEE;
													}
													else if (((TGameItem*)go)->IsStackable())
														DoubleDraw = true;
												}

												if (go->IsCorpse()) //������
												{
													if (AnimationManager->CorpsePixelsInXY((TGameItem*)go, DrawPixelsX, DrawPixelsY, goZ))
													{
														g_LastObjectType = SOT_GAME_OBJECT;
														g_LastSelectedObject = go->Serial;
														g_SelectedObject = go;
													}
												}
												else
												{
													if (DoubleDraw)
													{
														DrawPixelsX -= 2;

														if (UO->StaticPixelsInXY(goGraphic, DrawPixelsX, DrawPixelsY - 5, goZ))
														{
															g_LastObjectType = SOT_GAME_OBJECT;
															g_LastSelectedObject = go->Serial;
															g_SelectedObject = go;
														}
														else if (UO->StaticPixelsInXY(goGraphic, DrawPixelsX + 5, DrawPixelsY, goZ))
														{
															g_LastObjectType = SOT_GAME_OBJECT;
															g_LastSelectedObject = go->Serial;
															g_SelectedObject = go;
														}
													}
													else if (UO->StaticPixelsInXYAnimated(goGraphic, DrawPixelsX, DrawPixelsY, goZ))
													{
														g_LastObjectType = SOT_GAME_OBJECT;
														g_LastSelectedObject = go->Serial;
														g_SelectedObject = go;
													}
												}
											}
											else //if (go->NPC)
											{
												if (AnimationManager->CharacterPixelsInXY((TGameCharacter*)go, DrawPixelsX, DrawPixelsY, goZ))
												{
													g_LastObjectType = SOT_GAME_OBJECT;
													g_LastSelectedObject = go->Serial;
													g_SelectedObject = go;
												}
											}

											break;
										}
										case ROT_MULTI_OBJECT:
										{
											if (noDrawRoof && ro->IsRoof())
												break;

											if (UO->StaticPixelsInXY(ro->Graphic - 0x4000, DrawPixelsX, DrawPixelsY, ro->Z))
											{
												g_LastObjectType = SOT_STATIC_OBJECT;
												g_LastSelectedObject = 3;
												g_SelectedObject = ro;
											}

											break;
										}
										default:
											break;
									}
								}
							}
						}
					}
				}
			}*/

			if (drawModeCount)
			{
				//�������� ������
				for (; rto != NULL; rto = rto->m_PrevDraw)
				{
					if (!rto->IsText())
						continue;

					TTextData *td = (TTextData*)rto;
				
					if (td->Type != TT_SYSTEM)
					{
						if (td->Timer >= ticks)
						{
							TTextTexture &tth = td->m_Texture;

							if (tth.UnderMouse(td->DrawX, td->DrawY))
							{
								g_LastObjectType = SOT_TEXT_OBJECT;
								g_LastSelectedObject = 4;
								g_SelectedTextObject = rto;
							}
						}
					}
				}

				if (g_LastObjectType == SOT_TEXT_OBJECT)
					WorldTextRenderer->ToTop(g_SelectedTextObject);
			}
		}

		return g_LastSelectedObject;
	}

	return 0;
}
//---------------------------------------------------------------------------
void TGameScreen::OnLeftMouseDown()
{
	g_SelectGumpObjects = true;
	g_LastSelectedGump = 0;

	Render(false);
	
	if (g_LastSelectedGump)
		GumpManager->OnLeftMouseDown(false);
	
	m_GameWindowMoving = false;
	m_GameWindowResizing = false;

	if (g_LastObjectType == SOT_GAME_GUMP_SCOPE && !g_LastSelectedGump)
	{
		if (g_LastSelectedObject == 1)
			m_GameWindowMoving = true;
		else if (g_LastSelectedObject == 2)
			m_GameWindowResizing = true;
	}
	
	g_SelectGumpObjects = false;

	g_LeftMouseDown = true;
	g_DroppedLeftMouseX = g_MouseX;
	g_DroppedLeftMouseY = g_MouseY;
	g_LastObjectLeftMouseDown = g_LastSelectedObject;
	g_LastGumpLeftMouseDown = g_LastSelectedGump;
}
//---------------------------------------------------------------------------
void TGameScreen::OnLeftMouseUp()
{
	Render(false);

	if (g_ResizedGump != NULL)
	{
		g_ResizedGump->UpdateFrame();
		g_ResizedGump->ChangeHeight();

		return;
	}
	else if (g_LeftMouseDown && !g_LastGumpLeftMouseDown && !g_LastGumpRightMouseDown)
	{
		if (m_GameWindowMoving && g_LastObjectLeftMouseDown == 1)
		{
			g_GameWindowPosX = g_GameWindowPosX + (g_MouseX - g_DroppedLeftMouseX);
			g_GameWindowPosY = g_GameWindowPosY + (g_MouseY - g_DroppedLeftMouseY);

			if (g_GameWindowPosX < 1)
				g_GameWindowPosX = 0;

			if (g_GameWindowPosY < 1)
				g_GameWindowPosY = 0;

			if (g_GameWindowPosX + g_GameWindowSizeX > g_ClientWidth)
				g_GameWindowPosX = g_ClientWidth - g_GameWindowSizeX;

			if (g_GameWindowPosY + g_GameWindowSizeY > g_ClientHeight)
				g_GameWindowPosY = g_ClientHeight - g_GameWindowSizeY;

			m_GameWindowMoving = false;

			return;
		}
		else if (m_GameWindowResizing && g_LastObjectLeftMouseDown == 2)
		{
			g_GameWindowSizeX = g_GameWindowSizeX + (g_MouseX - g_DroppedLeftMouseX);
			g_GameWindowSizeY = g_GameWindowSizeY + (g_MouseY - g_DroppedLeftMouseY);
			
			if (g_GameWindowSizeX < 640)
				g_GameWindowSizeX = 640;
			
			if (g_GameWindowSizeY < 480)
				g_GameWindowSizeY = 480;

			TPacketGameWindowSize packet;
			packet.Send();

			m_GameWindowResizing = false;

			return;
		}
	}

	if (g_LeftMouseDown && (g_LastGumpLeftMouseDown || ObjectInHand != NULL))
	{
		if (GumpManager->OnLeftMouseUp(false))
			return;
	}
	
	int GameWindowPosX = g_GameWindowPosX - 4;
	int GameWindowPosY = g_GameWindowPosY - 4;

	if (g_MouseX < GameWindowPosX || g_MouseY < GameWindowPosY || g_MouseX > (GameWindowPosX + g_GameWindowSizeX) || g_MouseY > (GameWindowPosY + g_GameWindowSizeY))
		return;
	
	if (Target.IsTargeting() && g_SelectedObject != NULL)
	{
		if (g_LastObjectType == SOT_GAME_OBJECT && g_LastSelectedObject)
			Target.SendTargetObject(g_LastSelectedObject);
		else if (g_LastObjectType == SOT_LAND_OBJECT)
			Target.SendTargetTile(g_SelectedObject->Graphic, g_SelectedObject->X, g_SelectedObject->Y, g_SelectedObject->Z);
		else if (g_LastObjectType == SOT_STATIC_OBJECT)
			Target.SendTargetTile(g_SelectedObject->Graphic, g_SelectedObject->X, g_SelectedObject->Y, g_SelectedObject->Z);

		g_LastLClickTime = 0;

		return;
	}
	
	DWORD drop_container = 0xFFFFFFFF;
	bool can_drop = false;
	WORD DropX = 0;
	WORD DropY = 0;
	char DropZ = 0;

	if (g_LastObjectType == SOT_GAME_OBJECT && g_LastSelectedObject && ObjectInHand != NULL)
	{
		TGameObject *target = World->FindWorldObject(g_LastSelectedObject);

		can_drop = (GetDistance(g_Player, target) < 3);

		if (can_drop && target != NULL)
		{
			if (target->IsContainer())
				drop_container = target->Serial;
			else if (target->IsStackable() && target->Graphic == ObjectInHand->Graphic)
				drop_container = target->Serial;
			else if (target->NPC)
				drop_container = target->Serial;

			if (drop_container != 0xFFFFFFFF)
			{
				DropX = target->X;
				DropY = target->Y;
				DropZ = target->Z;
			}
		}
	}
	else if ((g_LastObjectType == SOT_LAND_OBJECT || g_LastObjectType == SOT_STATIC_OBJECT) && ObjectInHand != NULL && g_SelectedObject != NULL)
	{
		POINT pt = {g_SelectedObject->X, g_SelectedObject->Y};

		can_drop = (GetDistance(g_Player, pt) < 3);

		if (can_drop)
		{
			DropX = g_SelectedObject->X;
			DropY = g_SelectedObject->Y;
			DropZ = g_SelectedObject->Z;
		}
	}
	
	if (can_drop && ObjectInHand != NULL)
	{
		if (drop_container == 0xFFFFFFFF && !DropX && !DropY)
			can_drop = false;

		if (can_drop)
			UO->DropItem(drop_container, DropX, DropY, DropZ);
	}
	else if (ObjectInHand == NULL)
	{
		if (g_LastObjectType == SOT_GAME_OBJECT)
		{
			if (!g_ClickObjectReq)
			{
				TGameObject *click_target = World->FindWorldObject(g_LastObjectLeftMouseDown);
				if (click_target == NULL)
					return;

				g_ClickObjectReq = true;
				g_ClickObject.Init(COT_GAME_OBJECT);
				g_ClickObject.Serial = g_LastObjectLeftMouseDown;
				g_ClickObject.Timer = GetTickCount() + DCLICK_DELAY;
			}
		}
		else if (g_SelectedObject)
		{
			if (g_LastObjectType == SOT_STATIC_OBJECT)
			{
				WORD ID = g_SelectedObject->Graphic - 0x4000;

				string str(UO->m_StaticData[ID / 32].Tiles[ID % 32].Name);

				if (str.length())
					UO->CreateTextMessage(TT_CLIENT, (DWORD)g_SelectedObject, 3, 0x03B5, str);
			}
			/*else if (g_LastObjectType == SOT_LAND_OBJECT)
			{
				WORD ID = g_SelectedObject->Graphic;
				string str(UO->m_LandData[ID / 32].Tiles[ID % 32].Name);
				if (str.length())
					UO->CreateTextMessage(TT_CLIENT, 0, 3, 0x03B5, str);
			}*/
		}
	}

	if (EntryPointer != GameConsole && EntryPointer != NULL)
	{
		TGump *gumpEntry = GumpManager->GetTextEntryOwner();

		if (ConfigManager.GetConsoleNeedEnter())
			EntryPointer = NULL;
		else
			EntryPointer = GameConsole;

		if (gumpEntry != NULL)
			gumpEntry->UpdateFrame();
	}
}
//---------------------------------------------------------------------------
void TGameScreen::OnRightMouseDown()
{
	Render(false);
	
	g_LastGumpRightMouseDown = g_LastSelectedGump;

	if (g_LastSelectedGump)
		GumpManager->OnRightMouseDown(false);
}
//---------------------------------------------------------------------------
void TGameScreen::OnRightMouseUp()
{
	Render(false);
	
	if (g_RightMouseDown && g_LastGumpRightMouseDown)
		GumpManager->OnRightMouseUp(false);
}
//---------------------------------------------------------------------------
bool TGameScreen::OnLeftMouseDoubleClick()
{
	Render(false);

	bool result = false;

	if (g_LastSelectedGump && GumpManager->OnLeftMouseDoubleClick(false))
		result = true;
	else if (g_LastObjectType == SOT_GAME_OBJECT)
	{
		TGameObject *obj = World->FindWorldObject(g_LastSelectedObject);

		if (obj != NULL)
		{
			if (!obj->NPC)
				UO->DoubleClick(g_LastSelectedObject);
			else
			{
				if (g_Player->Warmode) 
					UO->Attack(g_LastSelectedObject);
				else
					UO->DoubleClick(g_LastSelectedObject);
					//UO->PaperdollReq(g_LastSelectedObject);
			}
		}
		result = true;
	}
	else if (g_LastObjectType == SOT_TEXT_OBJECT && g_SelectedTextObject != NULL)
	{
		TTextData *td = (TTextData*)g_SelectedTextObject;

		if (td->Type == TT_OBJECT)
		{
			DWORD serial = td->Serial;

			TGameObject *obj = World->FindWorldObject(serial);

			if (obj != NULL && obj->NPC)
			{
				if (g_Player->Warmode) 
					UO->Attack(serial);
				else
					UO->DoubleClick(serial);

				result = true;
			}
		}
	}

	return result;
}
//---------------------------------------------------------------------------
bool TGameScreen::OnRightMouseDoubleClick()
{
	return false;
}
//---------------------------------------------------------------------------
void TGameScreen::OnMouseWheel(MOUSE_WHEEL_STATE state)
{
	Render(false);

	if (g_LastSelectedGump)
		GumpManager->OnMouseWheel(state, false);
	else if (g_SelectedObject)
	{
		bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x80000000;

		if (ctrlPressed)
		{
			if (state == MWS_UP)
				g_SelectedObject->IncZ();
			else if (state == MWS_DOWN)
				g_SelectedObject->DecZ();
		}
		else //if (g_SelectedObject->RenderType == ROT_STATIC_OBJECT || g_SelectedObject->RenderType == ROT_LAND_OBJECT)
		{
			if (state == MWS_UP)
			{
				if (g_SelectedObject->m_NextXY != NULL)
				{
					TRenderWorldObject *next = g_SelectedObject->m_NextXY;
					g_SelectedObject->RemoveRender();
					g_SelectedObject->m_NextXY = next->m_NextXY;
					next->m_NextXY = g_SelectedObject;
					g_SelectedObject->m_PrevXY = next;

					if (g_SelectedObject->m_NextXY)
						g_SelectedObject->m_NextXY->m_PrevXY = g_SelectedObject;
				}
			}
			else if (state == MWS_DOWN)
			{
				if (g_SelectedObject->m_PrevXY != NULL)
				{
					TRenderWorldObject *prev = g_SelectedObject->m_PrevXY;
					g_SelectedObject->RemoveRender();
					g_SelectedObject->m_NextXY = prev;
					g_SelectedObject->m_PrevXY = prev->m_PrevXY;
					prev->m_PrevXY = g_SelectedObject;

					if (g_SelectedObject->m_PrevXY)
						g_SelectedObject->m_PrevXY->m_NextXY = g_SelectedObject;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void TGameScreen::OnCharPress(WPARAM wparam, LPARAM lparam)
{
	if (wparam == VK_RETURN || wparam == VK_BACK || wparam == VK_ESCAPE || EntryPointer == NULL)
		return; //Ignore no print keys

	wchar_t wstr[2] = {0};
	wstr[0] = wparam;
	wstring str(wstr);

	if (!str.length())
		return;
	
	if (EntryPointer != GameConsole)
	{
		if (GumpManager->OnCharPress(wparam, lparam, false))
			return;
	}

	bool altPressed = GetAsyncKeyState(VK_MENU) & 0x80000000;
	bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x80000000;
	//bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x80000000;

	if (EntryPointer == GameConsole && !EntryPointer->Length() && lparam == 0x100001 && ctrlPressed)
	{
		EntryPointer->SetText(g_LastConsoleEntry);
		EntryPointer->SetPos(g_LastConsoleEntry.length());
	}
	else if (!altPressed && !ctrlPressed && EntryPointer->Length() < 60)
		EntryPointer->Insert(wparam);
}
//---------------------------------------------------------------------------
void TGameScreen::OnKeyPress(WPARAM wparam, LPARAM lparam)
{
	if (!GumpManager->OnKeyPress(wparam, lparam, false))
	{
		if (EntryPointer == GameConsole)
			EntryPointer->OnKey(NULL, wparam);

		switch (wparam)
		{
			case VK_RETURN:
			{
				if (EntryPointer != NULL)
				{
					if (EntryPointer == GameConsole)
					{
						if (g_ConsolePrompt)
							UO->ConsolePromptSend();
						else if (EntryPointer->Length())
						{
							g_LastConsoleEntry = wstring(EntryPointer->Data());
							GameConsole->Send();
						}

						EntryPointer->Clear();
					}

					if (ConfigManager.GetConsoleNeedEnter())
						EntryPointer = NULL;
					else
						EntryPointer = GameConsole;
				}
				else
					EntryPointer = GameConsole;

				break;
			}
			case VK_PRIOR: //PgUp
			{
				//walk N (0)
				PathFinder->Walk(ConfigManager.AlwaysRun, 0);

				break;
			}
			case VK_NEXT: //PgDown
			{
				//walk E (2)
				PathFinder->Walk(ConfigManager.AlwaysRun, 2);

				break;
			}
			case VK_HOME:
			{
				//walk W (6)
				PathFinder->Walk(ConfigManager.AlwaysRun, 6);

				break;
			}
			case VK_END:
			{
				//walk S (4)
				PathFinder->Walk(ConfigManager.AlwaysRun, 4);

				break;
			}
			case VK_UP:
			{
				//Walk NW (7)
				PathFinder->Walk(ConfigManager.AlwaysRun, 7);

				break;
			}
			case VK_LEFT:
			{
				//Walk SW (5)
				PathFinder->Walk(ConfigManager.AlwaysRun, 5);

				break;
			}
			case VK_DOWN:
			{
				//Walk SE (3)
				PathFinder->Walk(ConfigManager.AlwaysRun, 3);

				break;
			}
			case VK_RIGHT:
			{
				//Walk NE (1)
				PathFinder->Walk(ConfigManager.AlwaysRun, 1);

				break;
			}
			case VK_ESCAPE:
			{
				if (Target.IsTargeting())
					Target.SendCancelTarget();

				if (g_ConsolePrompt)
					UO->ConsolePromptCancel();

				break;
			}
			default:
				break;
		}
	}

	if (wparam == VK_TAB)
	{
		if (ConfigManager.HoldTabForCombat)
			UO->ChangeWarmode(1);
		else
			UO->ChangeWarmode();
	}

	if (MacroPointer == NULL)
	{
		bool altPressed = GetAsyncKeyState(VK_MENU) & 0x80000000;
		bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x80000000;
		bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x80000000;

		TMacro *macro = MacroManager->FindMacro(wparam, altPressed, ctrlPressed, shiftPressed);

		if (macro != NULL)
			MacroPointer = (TMacroObject*)macro->m_Items;

		MacroManager->WaitingBandageTarget = false;
		MacroManager->WaitForTargetTimer = 0;
		MacroManager->Execute();
	}
}
//---------------------------------------------------------------------------