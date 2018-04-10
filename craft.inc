// built-in include guard removal
// just in case the user has a local dependency with the same file name
#if defined _inc_library
	#undef _inc_library
#endif
// custom include-guard to ensure we don't duplicate
#if defined _library_included
	#endinput
#endif
#define _library_included


#include <a_samp>
#include <logger>
#include <item>
#include <inventory>
#include <container>
#include <inventory-dialog>
#include <container-dialog>

#include <YSI\y_hooks>


// CRAFT_MAX_CRAFT_SET controls the total amount of "craft-sets" which are individual recipes for
// transforming n items into 1 item.
#if !defined CRAFT_MAX_CRAFT_SET
	#define CRAFT_MAX_CRAFT_SET (CraftSet:64)
#endif

// CRAFT_MAX_CRAFT_SET_ITEMS controls the maximum amount of "ingredients" per craft-set
#if !defined CRAFT_MAX_CRAFT_SET_ITEMS
	#define CRAFT_MAX_CRAFT_SET_ITEMS (8)
#endif

#define INVALID_CRAFTSET (CraftSet:-1)

enum e_craft_item_data {
	ItemType:craft_itemType,
	bool:craft_keepItem
}

enum e_selected_item_data {
	ItemType:craft_selectedItemType,
	craft_selectedItemID
}

enum E_CRAFT_ENVIRONMENT {
	CRAFT_ENVIRONMENT_NONE,
	CRAFT_ENVIRONMENT_INVENTORY,
	CRAFT_ENVIRONMENT_CONTAINER
}


// -
// API
// -


forward CraftSet:DefineItemCraftSet(ItemType:result, {ItemType, _}:...);
/*
# Description:
Defines a recipe with one target item and 1..n ingredients, where n < CRAFT_MAX_CRAFT_SET_ITEMS.
*/

forward bool:IsValidCraftSet(CraftSet:craftset);
/*
# Description:
Checks if a craftset ID is valid.
*/

forward GetCraftSetIngredients(CraftSet:craftset, output[CRAFT_MAX_CRAFT_SET_ITEMS][e_craft_item_data], &count);
/*
# Description:
Stores the ingredient data for `craftset` into `output`.
*/

forward GetCraftSetItemType(CraftSet:craftset, index, &ItemType:itemtype);
/*
# Description:
Returns an ingredient item type at `index` for `craftset`.
*/

forward GetCraftSetItemKeep(CraftSet:craftset, index, &bool:keep);
/*
# Description:
Stores the keep state for the item at the given index in `craftset`.
*/

forward GetCraftSetItemCount(CraftSet:craftset, &count);
/*
# Description:
Returns the amount of ingredient items in `craftset`.
*/

forward GetCraftSetResult(CraftSet:craftset, &ItemType:result);
/*
# Description:
Stores the target result item type for `craftset` into `result`.
*/

forward GetCraftSetTotal();
/*
# Description:
Returns the total amount of defined craftsets.
*/

forward GetPlayerSelectedCraftItems(playerid, output[CRAFT_MAX_CRAFT_SET_ITEMS][e_selected_item_data]);
/*
# Description:
If `playerid` is currently crafting, stores their currently selected items into `output`.
*/

forward GetPlayerSelectedCraftItemID(playerid, index, &itemid);
/*
# Description:
If `playerid` is currently crafting, stores the selected item specified at `index` into `itemid`.
*/

forward GetPlayerSelectedCraftItemCount(playerid, &count);
/*
# Description:
Stores into `count` the amount of items the player has selected to craft.
*/

forward GetPlayerCraftEnvironment(playerid, &E_CRAFT_ENVIRONMENT:env);
/*
# Description:
Stores into `env` the craft environment the player is using.
*/

forward ItemTypeResultForCraftingSet(ItemType:itemtype, &CraftSet:craftset);
/*
# Description:
Stores into `craftset` the craft-set ID that `itemtype` is a result of, if any.
*/


// -
// Events
// -


forward OnPlayerCraft(playerid, CraftSet:craftset);
/*
# Called:
Before a player crafts a set of items in the given `craftset`. `return 1` will cancel the craft.
*/

forward OnPlayerCrafted(playerid, CraftSet:craftset, result);
/*
# Called:
After a player has successfully crafted a craftset. `result` holds the result item ID.
*/


// -
// Setup
// -


static
	craft_Ingredients[CRAFT_MAX_CRAFT_SET][CRAFT_MAX_CRAFT_SET_ITEMS][e_craft_item_data],
	craft_ItemCount[CRAFT_MAX_CRAFT_SET],
	ItemType:craft_Result[CRAFT_MAX_CRAFT_SET],
	craft_Total,
	CraftSet:craft_ItemTypeResultFor[MAX_ITEM_TYPE] = {INVALID_CRAFTSET, ...},
	craft_SelectedItems[MAX_PLAYERS][CRAFT_MAX_CRAFT_SET_ITEMS][e_selected_item_data],
	craft_SelectedItemCount[MAX_PLAYERS],
	craft_SelectedSlot[MAX_PLAYERS],
	E_CRAFT_ENVIRONMENT:craft_SelectionEnvironment[MAX_PLAYERS],
	craft_MenuOptionID[MAX_PLAYERS];


// -
// Implementation
// -


stock CraftSet:DefineItemCraftSet(ItemType:result, {ItemType, bool}:...) {
	if(CraftSet:craft_Total >= CRAFT_MAX_CRAFT_SET) {
		return INVALID_CRAFTSET;
	}

	new
		CraftSet:id = CraftSet:craft_Total++,
		args = numargs();

	if(!(args & 1)) {
		err("DefineItemCraftSet has invalid parameter structure. Should be: result, (item, keep) * n",
			_i("result", _:result));
		return INVALID_CRAFTSET;
	}

	craft_Result[id] = result;
	craft_ItemTypeResultFor[result] = id;

	for(new i, j = 1; j < args; i++) {
		craft_Ingredients[id][i][craft_itemType] = ItemType:getarg(j++);
		craft_Ingredients[id][i][craft_keepItem] = bool:getarg(j++);
		craft_ItemCount[id]++;
	}

	_btn_SortCraftSet(craft_Ingredients[id], 0, craft_ItemCount[id]);

	return id;
}

// craft_Total
stock bool:IsValidCraftSet(CraftSet:craftset) {
	if(!(CraftSet:0 <= craftset < CraftSet:craft_Total)) {
		return false;
	}

	return true;
}

// craft_Ingredients
stock GetCraftSetIngredients(CraftSet:craftset, output[CRAFT_MAX_CRAFT_SET_ITEMS][e_craft_item_data], &count) {
	if(!IsValidCraftSet(craftset)) {
		return 1;
	}

	for(new i; i < craft_ItemCount[craftset]; i++) {
		output[i][craft_itemType] = craft_Ingredients[craftset][i][craft_itemType];
		output[i][craft_keepItem] = craft_Ingredients[craftset][i][craft_keepItem];
	}

	count = craft_ItemCount[craftset];
	return 0;
}

// craft_Ingredients/craft_itemType
stock GetCraftSetItemType(CraftSet:craftset, index, &ItemType:itemtype) {
	if(!IsValidCraftSet(craftset)) {
		return 1;
	}

	if(!(0 <= index < craft_ItemCount[craftset])) {
		return 2;
	}

	itemtype = craft_Ingredients[craftset][index][craft_itemType];
	return 0;
}

// craft_Ingredients/craft_keepItem
stock GetCraftSetItemKeep(CraftSet:craftset, index, &bool:keep) {
	if(!IsValidCraftSet(craftset)) {
		return 1;
	}

	if(!(0 <= index < craft_ItemCount[craftset])) {
		return 2;
	}

	keep = craft_Ingredients[craftset][index][craft_keepItem];
	return 0;
}

// craft_ItemCount
stock GetCraftSetItemCount(CraftSet:craftset, &count) {
	if(!IsValidCraftSet(craftset)) {
		return 1;
	}

	count = craft_ItemCount[craftset];
	return 0;
}

// craft_Result
stock GetCraftSetResult(CraftSet:craftset, &ItemType:result) {
	if(!IsValidCraftSet(craftset)) {
		return 1;
	}

	result = craft_Result[craftset];
	return 0;
}

// craft_Total
stock GetCraftSetTotal()
	return craft_Total;

// craft_SelectedItems
stock GetPlayerSelectedCraftItems(playerid, output[CRAFT_MAX_CRAFT_SET_ITEMS][e_selected_item_data]) {
	if(!IsPlayerConnected(playerid)) {
		return 1;
	}
	output = craft_SelectedItems[playerid];
	return 0;
}

// craft_SelectedItems/craft_selectedItemID
stock GetPlayerSelectedCraftItemID(playerid, index, &itemid) {
	if(!IsPlayerConnected(playerid)) {
		return 1;
	}

	if(!(0 <= index < craft_SelectedItemCount[playerid])) {
		return 2;
	}

	itemid = craft_SelectedItems[playerid][index][craft_selectedItemID];
	return 0;
}

// craft_SelectedItemCount
stock GetPlayerSelectedCraftItemCount(playerid, &count) {
	if(!IsPlayerConnected(playerid)) {
		return 1;
	}

	count = craft_SelectedItemCount[playerid];
	return 0;
}

// craft_SelectionEnvironment
stock GetPlayerCraftEnvironment(playerid, &E_CRAFT_ENVIRONMENT:env) {
	if(!IsPlayerConnected(playerid)) {
		return 1;
	}

	env = craft_SelectionEnvironment[playerid];
	return 0;
}

// craft_ItemTypeResultFor
stock ItemTypeResultForCraftingSet(ItemType:itemtype, &CraftSet:craftset) {
	if(!IsValidItemType(itemtype)) {
		return 1;
	}

	craftset = craft_ItemTypeResultFor[itemtype];
	return 0;
}


/*==============================================================================

	Internal Functions and Hooks

==============================================================================*/


hook OnPlayerViewInvOpt(playerid) {
	if(craft_SelectedItemCount[playerid] == 0) {
		craft_MenuOptionID[playerid] = AddInventoryOption(playerid, "Combine...");
	} else {
		new
			string[128], // TODO: Improve option length
			itemname[MAX_ITEM_NAME];

		string = "Combine with ";

		for(new i; i < craft_SelectedItemCount[playerid]; i++) {
			GetItemTypeName(craft_SelectedItems[playerid][i][craft_selectedItemType], itemname);

			if(i == 0) {
				format(string, sizeof(string), "%s%s", string, itemname);
			} else {
				format(string, sizeof(string), "%s+%s", string, itemname);
			}
		}
		craft_MenuOptionID[playerid] = AddInventoryOption(playerid, string);
	}

	return Y_HOOKS_CONTINUE_RETURN_0;
}

hook OnPlayerSelectInvOpt(playerid, option) {
	if(option == craft_MenuOptionID[playerid]) {
		// Get selected item ID, store it.
		// make the item yellow.
		// If there are more than 1 items selected, perform craft table check.
		new
			slot,
			itemid;

		slot = GetPlayerSelectedInventorySlot(playerid);
		GetInventorySlotItem(playerid, slot, itemid);

		craft_SelectedSlot[playerid] = slot;
		craft_SelectionEnvironment[playerid] = CRAFT_ENVIRONMENT_INVENTORY;

		_craft_AddItemToCraftList(playerid, itemid);

		DisplayPlayerInventory(playerid);
	}

	return 0;
}

hook OnPlayerViewCntOpt(playerid, containerid) {
	if(craft_SelectedItemCount[playerid] == 0) {
		craft_MenuOptionID[playerid] = AddContainerOption(playerid, "Combine...");
	} else {
		new
			string[128], // TODO: Improve option length
			itemname[MAX_ITEM_NAME];

		string = "Combine with ";

		for(new i; i < craft_SelectedItemCount[playerid]; i++) {
			GetItemTypeName(craft_SelectedItems[playerid][i][craft_selectedItemType], itemname);

			if(i == 0) {
				format(string, sizeof(string), "%s%s", string, itemname);
			} else {
				format(string, sizeof(string), "%s+%s", string, itemname);
			}
		}
		craft_MenuOptionID[playerid] = AddContainerOption(playerid, string);
	}

	return 0;
}

hook OnPlayerSelectCntOpt(playerid, containerid, option) {
	if(option == craft_MenuOptionID[playerid]) {
		// Get selected item ID, store it.
		// make the item yellow.
		// If there are more than 1 items selected, perform craft table check.
		new
			slot,
			itemid;

		GetPlayerContainerSlot(playerid, slot);
		GetContainerSlotItem(playerid, slot, itemid);

		craft_SelectedSlot[playerid] = slot;
		craft_SelectionEnvironment[playerid] = CRAFT_ENVIRONMENT_INVENTORY;

		_craft_AddItemToCraftList(playerid, itemid);
		craft_SelectionEnvironment[playerid] = CRAFT_ENVIRONMENT_CONTAINER;

		DisplayContainerInventory(playerid, containerid);
	}

	return 0;
}

hook OnPlayerCloseInventory(playerid) {
	_craft_ClearCraftList(playerid);
	return Y_HOOKS_CONTINUE_RETURN_0;
}

hook OnPlayerCloseContainer(playerid, containerid) {
	_craft_ClearCraftList(playerid);
	return Y_HOOKS_CONTINUE_RETURN_0;
}

CraftSet:_craft_FindCraftset(list[CRAFT_MAX_CRAFT_SET_ITEMS][e_selected_item_data], size) {
	_btn_SortSelectedItems(list, 0, size);

	for(new CraftSet:i; i < CraftSet:craft_Total; i++) {
		if(craft_ItemCount[i] != size) {
			continue;
		}

		if(_craft_CompareListToCraftSet(list, i)) {
			return i;
		}
	}

	return INVALID_CRAFTSET;
}

_craft_AddItemToCraftList(playerid, itemid) {
	new itemname[MAX_ITEM_NAME];
	GetItemTypeName(GetItemType(itemid), itemname);

	dbg("craft", "_craft_AddItemToCraftList",
		_i("playerid", playerid),
		_i("itemid", itemid),
		_i("itemtype", _:GetItemType(itemid)),
		_s("itemname", itemname));

	craft_SelectedItems[playerid][craft_SelectedItemCount[playerid]][craft_selectedItemType] = GetItemType(itemid);
	craft_SelectedItems[playerid][craft_SelectedItemCount[playerid]][craft_selectedItemID] = itemid;
	craft_SelectedItemCount[playerid]++;

	if(craft_SelectedItemCount[playerid] >= 2) {
		new CraftSet:craftset = _craft_FindCraftset(craft_SelectedItems[playerid], craft_SelectedItemCount[playerid]);

		if(craftset != INVALID_CRAFTSET) {
			_craft_CraftSelected(playerid, craftset);
		} else {
			SendClientMessage(playerid, -1, "That combination doesn't seem to work... Try adding more items or close the menu to cancel.");
		}
	}

	return 1;
}

_craft_ClearCraftList(playerid) {
	dbg("craft", "_craft_ClearCraftList",
		_i("playerid", playerid));

	craft_SelectionEnvironment[playerid] = CRAFT_ENVIRONMENT_NONE;
	craft_SelectedItemCount[playerid] = 0;
}

_craft_CraftSelected(playerid, CraftSet:craftset) {
	dbg("craft", "_craft_CraftSelected",
		_i("playerid", playerid),
		_i("craftset", _:craftset));

	new
		resultslots,
		tmpslots,
		totalslots,
		freeslots,
		slot;
	
	GetItemTypeSize(craft_Result[craftset], resultslots);

	for(new i; i < craft_ItemCount[craftset]; i++) {
		if(!craft_Ingredients[craftset][i][craft_keepItem]) {
			GetItemTypeSize(craft_Ingredients[craftset][i][craft_itemType], tmpslots);
			resultslots -= tmpslots;
		}
	}

	if(craft_SelectionEnvironment[playerid] == CRAFT_ENVIRONMENT_INVENTORY) {
		GetInventoryFreeSlots(playerid, freeslots);
		GetPlayerInventorySize(playerid, totalslots);

		if(freeslots + resultslots > totalslots) {
			return 1;
		}

		if(CallLocalFunction("OnPlayerCraft", "dd", playerid, _:craftset)) {
			return 2;
		}

		for(new i; i < craft_SelectedItemCount[playerid]; i++) {
			dbg("craft", "_craft_CraftSelected looping ingredient item",
				_i("itemid", craft_SelectedItems[playerid][i][craft_selectedItemID]),
				_i("itemtype", _:GetItemType(craft_SelectedItems[playerid][i][craft_selectedItemID])));

			if(!craft_Ingredients[craftset][i][craft_keepItem]) {
				dbg("craft", "_craft_CraftSelected destroying ingredient item",
					_i("itemid", craft_SelectedItems[playerid][i][craft_selectedItemID]),
					_i("itemtype", _:GetItemType(craft_SelectedItems[playerid][i][craft_selectedItemID])));

				GetItemPlayerInventorySlot(craft_SelectedItems[playerid][i][craft_selectedItemID], slot);
				RemoveItemFromInventory(playerid, slot, 0);
				DestroyItem(craft_SelectedItems[playerid][i][craft_selectedItemID]);
			}
		}

		new itemid = CreateItem(craft_Result[craftset]);
		dbg("craft", "_craft_CraftSelected created result item",
			_i("itemid", itemid),
			_i("result", _:craft_Result[craftset]));

		AddItemToInventory(playerid, itemid, false);
		dbg("craft", "_craft_CraftSelected added result item to player",
			_i("itemid", itemid),
			_i("playerid", playerid));

		CallLocalFunction("OnPlayerCrafted", "ddd", playerid, _:craftset, itemid);
	} else {
		new containerid;
		GetPlayerCurrentContainer(playerid, containerid);

		if(!IsValidContainer(containerid)) {
			return 3;
		}

		GetContainerFreeSlots(containerid, freeslots);
		GetContainerSize(containerid, totalslots);

		if(freeslots + resultslots > totalslots) {
			return 4;
		}

		if(CallLocalFunction("OnPlayerCraft", "dd", playerid, _:craftset)) {
			return 5;
		}

		for(new i; i < craft_SelectedItemCount[playerid]; i++) {
			dbg("craft", "_craft_CraftSelected looping ingredient item",
				_i("itemid", craft_SelectedItems[playerid][i][craft_selectedItemID]),
				_i("itemtype", _:GetItemType(craft_SelectedItems[playerid][i][craft_selectedItemID])));

			if(!craft_Ingredients[craftset][i][craft_keepItem]) {
				dbg("craft", "_craft_CraftSelected destroying ingredient item",
					_i("itemid", craft_SelectedItems[playerid][i][craft_selectedItemID]),
					_i("itemtype", _:GetItemType(craft_SelectedItems[playerid][i][craft_selectedItemID])));

				GetItemContainerSlot(craft_SelectedItems[playerid][i][craft_selectedItemID], slot);
				RemoveItemFromContainer(containerid, slot, .call = 0);
				DestroyItem(craft_SelectedItems[playerid][i][craft_selectedItemID]);
			}
		}

		new itemid = CreateItem(craft_Result[craftset]);
		dbg("craft", "_craft_CraftSelected created result item",
			_i("itemid", itemid),
			_i("result", _:craft_Result[craftset]));

		AddItemToContainer(containerid, itemid);
		dbg("craft", "_craft_CraftSelected added result item to container",
			_i("itemid", itemid),
			_i("containerid", containerid));

		CallLocalFunction("OnPlayerCrafted", "ddd", playerid, _:craftset, itemid);
	}

	new
		str[8 + MAX_ITEM_NAME],
		itemname[MAX_ITEM_NAME];

	GetItemTypeName(craft_Result[craftset], itemname);
	format(str, sizeof(str), "Crafted %s", itemname);
	ShowActionText(playerid, str, 3000);

	_craft_ClearCraftList(playerid);

	return 1;
}

_craft_CompareListToCraftSet(list[][e_selected_item_data], CraftSet:craftset) {
	dbg("craft", "_craft_CompareListToCraftSet",
		_i("craftset", _:craftset),
		_i("size", craft_ItemCount[craftset]));

	for(new i; i < craft_ItemCount[craftset]; i++) {
		dbg("craft", "_craft_CompareListToCraftSet itemtype in craftset",
			_i("index", i),
			_i("itemtype", _:craft_Ingredients[craftset][i][craft_itemType]),
			_i("selectedtype", _:list[i][craft_selectedItemType]));

		if(craft_Ingredients[craftset][i][craft_itemType] != list[i][craft_selectedItemType]) {
			return 0;
		}
	}

	return 1;
}

_btn_SortCraftSet(array[CRAFT_MAX_CRAFT_SET_ITEMS][e_craft_item_data], left, right) {
	new
		tmp_left = left,
		tmp_right = right,
		ItemType:pivot = array[(left + right) / 2][craft_itemType],
		ItemType:itemtype,
		bool:keepitem;

	while(tmp_left <= tmp_right) {
		while(array[tmp_left][craft_itemType] > pivot) {
			tmp_left++;
		}

		while(array[tmp_right][craft_itemType] < pivot) {
			tmp_right--;
		}

		if(tmp_left <= tmp_right) {
			itemtype = array[tmp_left][craft_itemType];
			array[tmp_left][craft_itemType] = array[tmp_right][craft_itemType];
			array[tmp_right][craft_itemType] = itemtype;

			keepitem = array[tmp_left][craft_keepItem];
			array[tmp_left][craft_keepItem] = array[tmp_right][craft_keepItem];
			array[tmp_right][craft_keepItem] = keepitem;

			tmp_left++;
			tmp_right--;
		}
	}

	if(left < tmp_right) {
		_btn_SortCraftSet(array, left, tmp_right);
	}

	if(tmp_left < right) {
		_btn_SortCraftSet(array, tmp_left, right);
	}
}

_btn_SortSelectedItems(array[CRAFT_MAX_CRAFT_SET_ITEMS][e_selected_item_data], left, right) {
	new
		tmp_left = left,
		tmp_right = right,
		ItemType:pivot = array[(left + right) / 2][craft_selectedItemType],
		ItemType:itemtype,
		itemid;

	while(tmp_left <= tmp_right) {
		while(array[tmp_left][craft_selectedItemType] > pivot) {
			tmp_left++;
		}

		while(array[tmp_right][craft_selectedItemType] < pivot) {
			tmp_right--;
		}

		if(tmp_left <= tmp_right) {
			itemtype = array[tmp_left][craft_selectedItemType];
			array[tmp_left][craft_selectedItemType] = array[tmp_right][craft_selectedItemType];
			array[tmp_right][craft_selectedItemType] = itemtype;

			itemid = array[tmp_left][craft_selectedItemID];
			array[tmp_left][craft_selectedItemID] = array[tmp_right][craft_selectedItemID];
			array[tmp_right][craft_selectedItemID] = itemid;

			tmp_left++;
			tmp_right--;
		}
	}

	if(left < tmp_right) {
		_btn_SortSelectedItems(array, left, tmp_right);
	}

	if(tmp_left < right) {
		_btn_SortSelectedItems(array, tmp_left, right);
	}
}
