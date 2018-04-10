#include "craft.inc"

main() {
	new CraftSet:craftset = DefineItemCraftSet(ItemType:0);
	IsValidCraftSet(craftset);
	new output[CRAFT_MAX_CRAFT_SET_ITEMS][e_craft_item_data];
	new count;
	GetCraftSetIngredients(craftset, output, count);
	new ItemType:itemtype;
	GetCraftSetItemType(craftset, 0, itemtype);
	new bool:keep;
	GetCraftSetItemKeep(craftset, 0, keep);
	GetCraftSetItemCount(craftset, count);
	new ItemType:result;
	GetCraftSetResult(craftset, result);
	GetCraftSetTotal();
	new playerid;
	new output2[CRAFT_MAX_CRAFT_SET_ITEMS][e_selected_item_data];
	GetPlayerSelectedCraftItems(playerid, output2);
	new itemid;
	GetPlayerSelectedCraftItemID(playerid, 0, itemid);
	GetPlayerSelectedCraftItemCount(playerid, count);
	new E_CRAFT_ENVIRONMENT:env;
	GetPlayerCraftEnvironment(playerid, env);
	ItemTypeResultForCraftingSet(itemtype, craftset);
}

public OnPlayerCraft(playerid, CraftSet:craftset) {
	//
}

public OnPlayerCrafted(playerid, CraftSet:craftset, result) {
	//
}
