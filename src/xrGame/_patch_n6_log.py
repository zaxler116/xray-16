import io

path = r"C:\DEV\code\personal\xray-16\src\xrGame\alife_communication_manager.cpp"
with io.open(path, "rb") as f:
    src = f.read().decode("utf-8")

# 1. offline human<->human trade (vfPerformTrading): log the keepable pool
old1 = (
    "  // N.4: only keepable (non-personal, above minimum stock) items are offered\r\n"
    "  append_keepable_items(tpALifeHumanAbstract1, m_tpItems1);\r\n"
    "  append_keepable_items(tpALifeHumanAbstract2, m_tpItems2);\r\n"
)
new1 = (
    "  // N.4: only keepable (non-personal, above minimum stock) items are offered\r\n"
    "  append_keepable_items(tpALifeHumanAbstract1, m_tpItems1);\r\n"
    "  append_keepable_items(tpALifeHumanAbstract2, m_tpItems2);\r\n"
    "\r\n"
    "#if defined(DEBUG) || defined(DEBUG_ALIFE)\r\n"
    "  if (ALIFE_LOG_ON)\r\n"
    '    Msg("N.4 trade pool: %s offers %d of %d, %s offers %d of %d children",\r\n'
    "        tpALifeHumanAbstract1->name_replace(), (int)m_tpItems1.size(),\r\n"
    "        (int)tpALifeHumanAbstract1->children.size(),\r\n"
    "        tpALifeHumanAbstract2->name_replace(), (int)m_tpItems2.size(),\r\n"
    "        (int)tpALifeHumanAbstract2->children.size());\r\n"
    "#endif\r\n"
)
assert old1 in src, "anchor 1 not found"
src = src.replace(old1, new1, 1)

# 2. offline human->trader (communicate_with_customer): count kept items
old2 = (
    "  CSE_ALifeItemPDA *original_pda = 0;\r\n"
    "  tpALifeHumanAbstract->brain().m_dwTotalMoney = tpALifeHumanAbstract->m_dwMoney;\r\n"
    "  {\r\n"
    "    ALife::OBJECT_IT I = tpALifeHumanAbstract->children.begin();\r\n"
)
new2 = (
    "  CSE_ALifeItemPDA *original_pda = 0;\r\n"
    "  tpALifeHumanAbstract->brain().m_dwTotalMoney = tpALifeHumanAbstract->m_dwMoney;\r\n"
    "  int l_iKeptItems = 0; // N.6: how many children stay with the human\r\n"
    "  {\r\n"
    "    ALife::OBJECT_IT I = tpALifeHumanAbstract->children.begin();\r\n"
)
assert old2 in src, "anchor 2 not found"
src = src.replace(old2, new2, 1)

old3 = (
    "      if (!tpALifeHumanAbstract->brain().objects().item_is_keepable(l_tpALifeInventoryItem, tpALifeHumanAbstract))\r\n"
    "        continue;\r\n"
    "      tpALifeHumanAbstract->detach(l_tpALifeInventoryItem, 0, true, false);\r\n"
)
new3 = (
    "      if (!tpALifeHumanAbstract->brain().objects().item_is_keepable(l_tpALifeInventoryItem, tpALifeHumanAbstract))\r\n"
    "        continue;\r\n"
    "      ++l_iKeptItems;\r\n"
    "      tpALifeHumanAbstract->detach(l_tpALifeInventoryItem, 0, true, false);\r\n"
)
assert old3 in src, "anchor 3 not found"
src = src.replace(old3, new3, 1)

# 3. log at the end of communicate_with_customer
old4 = (
    "    VERIFY(I != tpALifeTrader->children.end());\r\n"
    "    smart_cast<CSE_ALifeDynamicObject *>(tpALifeTrader->base())->detach(original_pda);\r\n"
    "    tpALifeHumanAbstract->attach(original_pda, true);\r\n"
    "  }\r\n"
    "}\r\n"
)
new4 = (
    "    VERIFY(I != tpALifeTrader->children.end());\r\n"
    "    smart_cast<CSE_ALifeDynamicObject *>(tpALifeTrader->base())->detach(original_pda);\r\n"
    "    tpALifeHumanAbstract->attach(original_pda, true);\r\n"
    "  }\r\n"
    "\r\n"
    "#if defined(DEBUG) || defined(DEBUG_ALIFE)\r\n"
    "  if (ALIFE_LOG_ON)\r\n"
    '    Msg("N.6 trade with trader %s: %d items sold, %d kept (personal/minimum stock)",\r\n'
    "        tpALifeTrader->name_replace(), l_iKeptItems,\r\n"
    "        (int)tpALifeHumanAbstract->children.size() - l_iKeptItems);\r\n"
    "#endif\r\n"
    "}\r\n"
)
assert old4 in src, "anchor 4 not found"
src = src.replace(old4, new4, 1)

with io.open(path, "wb") as f:
    f.write(src.encode("utf-8"))
print("OK")
