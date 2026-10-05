////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_object_handler_inline.h
//	Created 	: 07.10.2005
//  Modified 	: 07.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human object handler class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CALifeHumanObjectHandler::CALifeHumanObjectHandler(object_type* object)
    : m_object(object), m_food_keep_days(1.0f), m_ammo_keep_factor(1.0f)
{
    VERIFY(object);

    // N.5: trade tuning, configs/alife.ltx [alife] (defaults above if absent)
    pSettings->read_if_exists(m_food_keep_days, "alife", "food_keep_days");
    pSettings->read_if_exists(m_ammo_keep_factor, "alife", "ammo_keep_factor");
}

IC CALifeHumanObjectHandler::object_type& CALifeHumanObjectHandler::object() const
{
    VERIFY(m_object);
    return (*m_object);
}
