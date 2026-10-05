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
}

IC CALifeHumanObjectHandler::object_type& CALifeHumanObjectHandler::object() const
{
    VERIFY(m_object);
    return (*m_object);
}
