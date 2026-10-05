////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.cpp
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "purchase_list.h"
#include "InventoryOwner.h"
#include "GameObject.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Level.h"
#include "ai_debug.h"

static float min_deficit_factor = .3f;

void CPurchaseList::process(CInifile& ini_file, LPCSTR section, CInventoryOwner& owner, bool return_item)
{
#if defined(DEBUG_ALIFE)
    if (ALIFE_LOG_ON)
        Msg("V5 restock: section=%s return_item=%d", section, (int)return_item);
#endif
    owner.sell_useless_items();

    m_deficits.clear();

    const CGameObject& game_object = smart_cast<const CGameObject&>(owner);
    CInifile::Sect& S = ini_file.r_section(section);
    auto I = S.Data.cbegin();
    auto E = S.Data.cend();
    for (; I != E; ++I)
    {
        if (I->second.empty())
            continue;

        if (!pSettings->section_exist(I->first))
            continue;

        //VERIFY3(I->second.size(), "PurchaseList : cannot handle lines in section without values", section);

        string256 temp0, temp1;
        //THROW3(_GetItemCount(I->second.c_str()) == 2, "Invalid parameters in section", section);

		cpcstr count = _GetItem(I->second.c_str(), 0, temp0);
		cpcstr prob = _GetItemCount(I->second.c_str()) >= 2 ? _GetItem(I->second.c_str(), 1, temp1) : "1.0f";

        process(game_object, I->first, atoi(count), (float)atof(prob), return_item);
    }
}

void CPurchaseList::process(
    const CGameObject& owner, const shared_str& name, const u32& count, const float& probability, bool return_item)
{
#if defined(DEBUG_ALIFE)
    if (ALIFE_LOG_ON)
        Msg("V5 restock item: section=%s count=%u prob=%.2f return_item=%d", name.c_str(), count, probability, (int)return_item);
#endif
    VERIFY3(count, "Invalid count for section in the purchase list", name.c_str());
    VERIFY3(!fis_zero(probability, EPS_S), "Invalid probability for section in the purchase list", name.c_str());

    const Fvector& position = owner.Position();
    const u32& level_vertex_id = owner.ai_location().level_vertex_id();
    const ALife::_OBJECT_ID& id = owner.ID();
    CRandom random((u32)(CPU::QPC() & u32(-1)));
    u32 j = 0;
    for (u32 i = 0; i < count; ++i)
    {
        if (random.randF() > probability)
            continue;

        ++j;
        if (return_item)
        {
            // V2.4 - keep the ALife object: create it, send the spawn to the
            // network, and do NOT destroy it, so it stays in the ALife registry
            // and gets attached to the owner's children.
            CSE_Abstract* abstract = Level().spawn_item(name.c_str(), position, level_vertex_id, id, true);
            R_ASSERT3(abstract, "Cannot find item with section", name.c_str());
            NET_Packet P;
            abstract->Spawn_Write(P, TRUE);
            Level().Send(P, net_flags(TRUE));
#if defined(DEBUG_ALIFE)
            if (ALIFE_LOG_ON)
                Msg("V5 restock spawned: section=%s id=%08X", name.c_str(), (unsigned)abstract->ID);
#endif
        }
        else
        {
            Level().spawn_item(name.c_str(), position, level_vertex_id, id, false);
        }
    }

    VERIFY3(m_deficits.find(name) == m_deficits.end(), "Duplicate section in the purchase list", name.c_str());
    m_deficits.emplace(name, (float)count * probability / _max((float)j, min_deficit_factor));
#if defined(DEBUG_ALIFE)
    if (ALIFE_LOG_ON)
        Msg("V5 restock done: section=%s spawned=%u/%u deficit=%.2f", name.c_str(), j, count, (float)count * probability / _max((float)j, min_deficit_factor));
#endif
}
