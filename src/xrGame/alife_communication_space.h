////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_communication_space.h
//	Created 	: 14.05.2004
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife communication space
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrServer_Objects_ALife_Items.h"

// 2003: removed from the item list the items already attached to the
// object (used by the choose_* methods of the human object handler and by
// the communication manager while trading).
struct CRemoveAttachedItemsPredicate {
  IC bool operator()(const CSE_ALifeInventoryItem *item) {
    return (item->attached());
  };
};
