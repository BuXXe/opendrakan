/*
 * Actions.h
 *
 *  Created on: 28 Dec 2018
 *      Author: zal
 */

#ifndef INCLUDE_DRAGONRFL_ACTIONS_H_
#define INCLUDE_DRAGONRFL_ACTIONS_H_

#include <map>

#include <odCore/SrscFile.h>

#include <odCore/input/Action.h>

namespace dragonRfl
{

    enum class Action
    {
        Unknown = 0, // TODO: document somewhere that the action with zero representation can't be bound

        Forward,
        Backward,
        Strafe_Left,
        Strafe_Right,
        Jump,
        Duck,
        Turn_Left,
        Turn_Right,
        Tilt_Up,
        Tilt_Down,
        Free_Look,
        Attack_Primary,
        Attack_Secondary,
        Interact,
        Inventory,
        Dragon_Action,
        Crouch,
        Map,
        Use_Potion_Health,
        Use_Potion_Invisibility,
        Use_Potion_Invulnerability,
        Use_Potion_Speed,
        Weapon_Next,
        Weapon_Previous,
        Weapon_Stow,
        Save_Quick,
        Load_Quick,
        Options,
        Save,
        Load,
        Chat,
        Change_Teams,
        Fogdepth_Up,
        Fogdepth_Down,
        Screenshot,
        Resolution_Up,
        Resolution_Down,
        Fullscreen_Toggle,
        Enginedata_Toggle,

        // these ones do not appear in Drakan.cfg, but have strings in Dragon.rrc
        Main_Menu,
        Pause,
        Demo_Record,
        Demo_Playback,

        // these are our own, additional actions
        PhysicsDebug_Toggle
    };

    using ActionHandle = odInput::ActionHandle<Action>;

    // TODO: move this out of global scope
    const extern std::map<Action, od::RecordId> sActionNameMap;
}

#endif /* INCLUDE_DRAGONRFL_ACTIONS_H_ */
