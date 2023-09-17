#include <imgui/imgui.h>
#include <thread>

#include "memory.hpp"
#include "utils.hpp"
#include "offsets.hpp"
#include "xorstr.hpp"

namespace hack
{
	std::shared_ptr<pProcess> process;
	ProcessModule base_module;

	bool watermark = true;
	bool menu, crosshair, showteam, bbox, pname, phealth, snaplines, skeletons = false;

	void loop()
	{
		uintptr_t localPlayer = process->read<uintptr_t>(base_module.base + offsets::dwLocalPlayer);
		if (!localPlayer)
			return;

		int localTeam = process->read<int>(localPlayer + offsets::m_iTeamNum);

		ViewMatrix view_matrix = process->read<ViewMatrix>(base_module.base + offsets::dwViewMatrix);
		Vector localOrigin = process->read<Vector>(localPlayer + offsets::m_vecOrigin);
		uintptr_t entity_list = process->read<uintptr_t>(base_module.base + offsets::dwEntityList);

		for (int i = 1; i < 32; i++)
		{
			uintptr_t list_entry = process->read<uintptr_t>(entity_list + (8 * (i & 0x7FFF) >> 9) + 16);
			if (!list_entry)
				continue;

			uintptr_t player = process->read<uintptr_t>(list_entry + 120 * (i & 0x1FF));
			if (!player)
				continue;

			int playerHealth = process->read<int>(player + offsets::dwPawnHealth);
			if (playerHealth <= 0 || playerHealth > 100)
				continue;

			// https://github.com/UnnamedZ03/CS2-external-base/blob/main/source/CSSPlayer.hpp#L132
			std::uint32_t playerpawn = process->read<std::uint32_t>(player + offsets::dwPlayerPawn);

			uintptr_t list_entry2 = process->read<uintptr_t>(entity_list + 0x8 * ((playerpawn & 0x7FFF) >> 9) + 16);
			if (!list_entry2)
				continue;

			uintptr_t pCSPlayerPawn = process->read<uintptr_t>(list_entry2 + 120 * (playerpawn & 0x1FF));
			if (pCSPlayerPawn == localPlayer)
				continue;

			uintptr_t gameScene = process->read<uintptr_t>(pCSPlayerPawn + offsets::m_hGameSceneNode);
			uintptr_t boneArray = process->read<uintptr_t>(gameScene + offsets::m_hModelState + offsets::m_hBoneArray);

			int playerTeam = process->read<int>(player + offsets::m_iTeamNum);

			std::string playerName = XorStr("Invalid Name");
			uintptr_t playerNameAddress = process->read<uintptr_t>(player + offsets::dwSanitizedName);
			if (playerNameAddress)
			{
				char buf[256];
				process->read_raw(playerNameAddress, buf, sizeof(buf));
				playerName = std::string(buf);
			}

			// https://github.com/UnnamedZ03/CS2-external-base/blob/main/source/CSSPlayer.hpp#L132
			Vector origin = process->read<Vector>(pCSPlayerPawn + offsets::m_vecOrigin);

			//if ((localOrigin - origin).length2d() > 1000) return;

			Vector head;
			head.x = origin.x;
			head.y = origin.y;
			head.z = origin.z + 75.f;

			//float height = screenpos.y - screenhead.y;
			//float width = height / 2.4f;
			//const float w = height * 0.35f;

			//uintptr_t weaponServices = process->read<uintptr_t>(pCSPlayerPawn + offsets::m_pWeaponServices);
			//uintptr_t myWeapons = process->read<uintptr_t>(weaponServices + offsets::m_hMyWeapons);
			//uintptr_t activeWeapon = process->read<uintptr_t>(list_entry + weaponServices + offsets::m_hActiveWeapon);
			//uintptr_t test = process->read<uintptr_t>(pCSPlayerPawn + offsets::m_pClippingWeapon);
			//uint16_t test2 = process->read<uint16_t>(test + offsets::m_AttributeManager + offsets::m_Item + offsets::m_iItemDefinitionIndex);

			// skeleton esp (cancer / shouldnt be legal)			
			// please GOD forgive me
			Vector sHead = process->read<Vector>(boneArray + 6 * 32);
			Vector sCou = process->read<Vector>(boneArray + 5 * 32);
			Vector sShoulderR = process->read<Vector>(boneArray + 8 * 32);
			Vector sShoulderL = process->read<Vector>(boneArray + 13 * 32);
			Vector sBrasR = process->read<Vector>(boneArray + 9 * 32);
			Vector sBrasL = process->read<Vector>(boneArray + 14 * 32);
			Vector sHandR = process->read<Vector>(boneArray + 11 * 32);
			Vector sHandL = process->read<Vector>(boneArray + 16 * 32);
			Vector sCock = process->read<Vector>(boneArray + 0 * 32);
			Vector sKneesR = process->read<Vector>(boneArray + 23 * 32);
			Vector sKneesL = process->read<Vector>(boneArray + 26 * 32);
			Vector sFeetR = process->read<Vector>(boneArray + 24 * 32);
			Vector sFeetL = process->read<Vector>(boneArray + 27 * 32);

			Vector sAhead, sAcou, sAshoulderR, sAshoulderL, sAbrasR, sAbrasL, sAhandR, sAhandL, sAcock, sAkneesR, sAkneesL, sAfeetR, sAfeetL;

			if (showteam ? (1 == 1) : (playerTeam != localTeam))
			{
				Vector top;
				Vector bottom;
				if (world_to_screen(head, top, view_matrix) && world_to_screen(origin, bottom, view_matrix))
				{
					const float h = bottom.y - top.y;
					const float w = h * 0.2f;

					const float hbH = bottom.y - top.y;
					const float hbW = hbH / 2.4f;

					// bounding box
					if (hack::bbox)
					{
						ImGui::GetBackgroundDrawList()->AddRect({ top.x - w - 1, top.y - 1 }, { top.x + w + 1 , bottom.y + 1 }, ImColor(0, 0, 0, 255));
						ImGui::GetBackgroundDrawList()->AddRect({ top.x - w + 1, top.y + 1 }, { top.x + w - 1 , bottom.y - 1 }, ImColor(0, 0, 0, 255));
						ImGui::GetBackgroundDrawList()->AddRect({ top.x - w, top.y }, { top.x + w , bottom.y }, ImColor(255, 255, 255, 255));
					}

					// snaplines
					if (hack::snaplines)
						ImGui::GetBackgroundDrawList()->AddLine(ImVec2(1920 / 2, 1080), ImVec2(bottom.x, bottom.y), ImColor(255, 255, 255, 255));

					// player name
					// 
					//        horizontal vertical			horizontal vertical
					// ImVec2(X posicao, Y posicao), ImVec2(X tamanho, Y tamanho)
					//ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(top.x - (hbW / 2), top.y - 18), ImVec2(top.x + (hbW / 2), top.y - 2), ImColor(0, 0, 0, 60));
					//ImGui::GetBackgroundDrawList()->AddText(ImVec2(top.x - (hbW / 2) + 2, top.y - 15), ImColor(1.f, 1.f, 1.f), playerName.c_str());
					if (hack::pname)
						ImGui::GetBackgroundDrawList()->AddText(ImVec2(top.x + (hbW / 2) + 1, top.y), ImColor(255, 255, 255, 255), playerName.c_str());

					// player health bar
					if (hack::phealth)
					{
						ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(top.x - (hbW / 2 + 3), top.y + (hbH * (100 - playerHealth) / 100)), ImVec2(top.x - (hbW / 2 + 1), bottom.y), ImColor(0, 255, 0, 255));

						// player health number
						ImGui::GetBackgroundDrawList()->AddText(ImVec2(top.x - (hbW / 2 + 12), top.y + (hbH * (100 - playerHealth) / 100) - 12), ImColor(255, 255, 255, 255), std::to_string(playerHealth).c_str());
					}

					//ImGui::GetBackgroundDrawList()->AddText(ImVec2(top.x + (hbW / 2) + 1, top.y + 15), ImColor(255, 255, 255, 255), std::to_string(activeWeapon).c_str());
				}
				
				if (hack::skeletons)
				{
					if (!world_to_screen(sHead, sAhead, view_matrix)) continue;
					if (!world_to_screen(sCou, sAcou, view_matrix)) continue;
					if (!world_to_screen(sShoulderR, sAshoulderR, view_matrix)) continue;
					if (!world_to_screen(sShoulderL, sAshoulderL, view_matrix)) continue;
					if (!world_to_screen(sBrasR, sAbrasR, view_matrix)) continue;
					if (!world_to_screen(sBrasL, sAbrasL, view_matrix)) continue;
					if (!world_to_screen(sHandR, sAhandR, view_matrix)) continue;
					if (!world_to_screen(sHandL, sAhandL, view_matrix)) continue;
					if (!world_to_screen(sCock, sAcock, view_matrix)) continue;
					if (!world_to_screen(sKneesR, sAkneesR, view_matrix)) continue;
					if (!world_to_screen(sKneesL, sAkneesL, view_matrix)) continue;
					if (!world_to_screen(sFeetR, sAfeetR, view_matrix)) continue;
					if (!world_to_screen(sFeetL, sAfeetL, view_matrix)) continue;

					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAcou.x, sAcou.y), ImVec2(sAhead.x, sAhead.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAcou.x, sAcou.y), ImVec2(sAshoulderR.x, sAshoulderR.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAcou.x, sAcou.y), ImVec2(sAshoulderL.x, sAshoulderL.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAbrasL.x, sAbrasL.y), ImVec2(sAshoulderL.x, sAshoulderL.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAbrasR.x, sAbrasR.y), ImVec2(sAshoulderR.x, sAshoulderR.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAbrasR.x, sAbrasR.y), ImVec2(sAhandR.x, sAhandR.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAbrasL.x, sAbrasL.y), ImVec2(sAhandL.x, sAhandL.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAcou.x, sAcou.y), ImVec2(sAcock.x, sAcock.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAkneesR.x, sAkneesR.y), ImVec2(sAcock.x, sAcock.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAkneesL.x, sAkneesL.y), ImVec2(sAcock.x, sAcock.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAkneesL.x, sAkneesL.y), ImVec2(sAfeetL.x, sAfeetL.y), ImColor(255, 255, 255, 255), 1.25);
					ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sAkneesR.x, sAkneesR.y), ImVec2(sAfeetR.x, sAfeetR.y), ImColor(255, 255, 255, 255), 1.25);
				}
			}
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}