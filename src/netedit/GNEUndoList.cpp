/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2021 German Aerospace Center (DLR) and others.
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// https://www.eclipse.org/legal/epl-2.0/
// This Source Code may also be made available under the following Secondary
// Licenses when the conditions for such availability set forth in the Eclipse
// Public License 2.0 are satisfied: GNU General Public License, version 2
// or later which is available at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
/****************************************************************************/
/// @file    GNEUndoList.cpp
/// @author  Jakob Erdmann
/// @date    Mar 2011
///
// FXUndoList2 is pretty dandy but some features are missing:
//   - we cannot find out wether we have currently begun an undo-group and
//     thus abort() is hard to use.
//   - onUpd-methods do not disable undo/redo while in an undo-group
//
// GNEUndoList inherits from FXUndoList2 and patches some methods. these are
// prefixed with p_
/****************************************************************************/
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/GNEViewNet.h>
#include <netedit/GNEViewParent.h>
#include <netedit/frames/common/GNESelectorFrame.h>

#include "GNEApplicationWindow.h"
#include "GNEUndoList.h"


// ===========================================================================
// FOX callback mapping
// ===========================================================================
FXDEFMAP(GNEUndoList) GNEUndoListMap[] = {
    //FXMAPFUNC(SEL_COMMAND, FXUndoList2::ID_REVERT,     FXUndoList2::onCmdRevert),
    //FXMAPFUNC(SEL_COMMAND, FXUndoList2::ID_UNDO,       FXUndoList2::onCmdUndo),
    //FXMAPFUNC(SEL_COMMAND, FXUndoList2::ID_REDO,       FXUndoList2::onCmdRedo),
    //FXMAPFUNC(SEL_COMMAND, FXUndoList2::ID_UNDO_ALL,   FXUndoList2::onCmdUndoAll),
    //FXMAPFUNC(SEL_COMMAND, FXUndoList2::ID_REDO_ALL,   FXUndoList2::onCmdRedoAll),
    //
    //FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_UNDO_COUNT, FXUndoList2::onUpdUndoCount),
    //FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_REDO_COUNT, FXUndoList2::onUpdRedoCount),
    //FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_CLEAR,      FXUndoList2::onUpdClear),
    //FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_REVERT,     FXUndoList2::onUpdRevert),
    FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_UNDO_ALL,   GNEUndoList::p_onUpdUndo),
    FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_REDO_ALL,   GNEUndoList::p_onUpdRedo),
    FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_UNDO,       GNEUndoList::p_onUpdUndo),
    FXMAPFUNC(SEL_UPDATE,  FXUndoList2::ID_REDO,       GNEUndoList::p_onUpdRedo)
};


// ===========================================================================
// FOX-declarations
// ===========================================================================
FXIMPLEMENT_ABSTRACT(GNEUndoList, FXUndoList2, GNEUndoListMap, ARRAYNUMBER(GNEUndoListMap))


// ===========================================================================
// member method definitions
// ===========================================================================

GNEUndoList::GNEUndoList(GNEApplicationWindow* parent) :
    FXUndoList2(),
    myGNEApplicationWindowParent(parent) {
}


void
GNEUndoList::p_begin(const std::string& description) {
    myCommandGroups.push(new GNEChangeGroup(description));
    begin(myCommandGroups.top());
}


void
GNEUndoList::p_end() {
    myCommandGroups.pop();
    // check if net has to be updated
    if (myCommandGroups.empty() && myGNEApplicationWindowParent->getViewNet()) {
        myGNEApplicationWindowParent->getViewNet()->updateViewNet();
        // check if we have to update selector frame
        const auto &editModes = myGNEApplicationWindowParent->getViewNet()->getEditModes();
        if ((editModes.isCurrentSupermodeNetwork() && editModes.networkEditMode == NetworkEditMode::NETWORK_SELECT) ||
            (editModes.isCurrentSupermodeDemand() && editModes.demandEditMode == DemandEditMode::DEMAND_SELECT) ||
            (editModes.isCurrentSupermodeData() && editModes.dataEditMode == DataEditMode::DATA_SELECT)) {
            myGNEApplicationWindowParent->getViewNet()->getViewParent()->getSelectorFrame()->getSelectionInformation()->updateInformationLabel();
        }
    }
    end();
}


void
GNEUndoList::p_clear() {
    // disable updating of interval bar (check viewNet due #7252)
    if (myGNEApplicationWindowParent->getViewNet()) {
        myGNEApplicationWindowParent->getViewNet()->getIntervalBar().disableIntervalBarUpdate();
    }
    p_abort();
    clear();
    // enable updating of interval bar again (check viewNet due #7252)
    if (myGNEApplicationWindowParent->getViewNet()) {
        myGNEApplicationWindowParent->getViewNet()->getIntervalBar().enableIntervalBarUpdate();
    }
}


void
GNEUndoList::p_abort() {
    while (hasCommandGroup()) {
        myCommandGroups.top()->undo();
        myCommandGroups.pop();
        abort();
    }
}


void
GNEUndoList::p_abortLastCommandGroup() {
    if (myCommandGroups.size() > 0) {
        myCommandGroups.top()->undo();
        myCommandGroups.pop();
        abort();
    }
}


void
GNEUndoList::undo() {
    WRITE_DEBUG("Calling GNEUndoList::undo()");
    FXUndoList2::undo();
    // update specific controls
    myGNEApplicationWindowParent->updateControls();
}


void
GNEUndoList::redo() {
    WRITE_DEBUG("Calling GNEUndoList::redo()");
    FXUndoList2::redo();
    // update specific controls
    myGNEApplicationWindowParent->updateControls();
}


void
GNEUndoList::p_add(GNEChange_Attribute* cmd) {
    if (cmd->trueChange()) {
        add(cmd, true);
    } else {
        delete cmd;
    }
}


int
GNEUndoList::currentCommandGroupSize() const {
    if (myCommandGroups.size() > 0) {
        return myCommandGroups.top()->size();
    } else {
        return 0;
    }
}


const GNEChange* 
GNEUndoList::getlastChange() const {
    if (myCommandGroups.empty()) {
        return nullptr;
    } else {
        const GNEChange* change = dynamic_cast<GNEChange*>(myCommandGroups.top());
        if (change) {
            return change;
        } else {
            return nullptr;
        }
    }
}


long
GNEUndoList::p_onUpdUndo(FXObject* sender, FXSelector, void*) {
    // first check if Undo Menu command or button has to be disabled
    bool enable = canUndo() && !hasCommandGroup() && myGNEApplicationWindowParent->isUndoRedoEnabled().empty();
    // cast button (see #6209)
    FXButton* button = dynamic_cast<FXButton*>(sender);
    // enable or disable depending of "enable" flag
    if (button) {
        // avoid unnnecesary enables/disables (due flickering)
        if (enable && !button->isEnabled()) {
            sender->handle(this, FXSEL(SEL_COMMAND, FXWindow::ID_ENABLE), nullptr);
            button->update();
        } else if (!enable && button->isEnabled()) {
            sender->handle(this, FXSEL(SEL_COMMAND, FXWindow::ID_DISABLE), nullptr);
            button->update();
        }
    } else {
        sender->handle(this, enable ? FXSEL(SEL_COMMAND, FXWindow::ID_ENABLE) : FXSEL(SEL_COMMAND, FXWindow::ID_DISABLE), nullptr);
    }
    // cast menu command
    FXMenuCommand* menuCommand = dynamic_cast<FXMenuCommand*>(sender);
    // only set caption on menu command item
    if (menuCommand) {
        // change caption of FXMenuCommand
        FXString caption = undoName();
        // set caption of FXmenuCommand edit/undo
        if (myGNEApplicationWindowParent->isUndoRedoEnabled().size() > 0) {
            caption = ("Cannot Undo in the middle of " + myGNEApplicationWindowParent->isUndoRedoEnabled()).c_str();
        } else if (hasCommandGroup()) {
            caption = ("Cannot Undo in the middle of " + myCommandGroups.top()->getDescription()).c_str();
        } else if (!canUndo()) {
            caption = "Undo";
        }
        menuCommand->handle(this, FXSEL(SEL_COMMAND, FXMenuCaption::ID_SETSTRINGVALUE), (void*)&caption);
        menuCommand->update();
    }
    return 1;
}


long
GNEUndoList::p_onUpdRedo(FXObject* sender, FXSelector, void*) {
    // first check if Redo Menu command or button has to be disabled
    bool enable = canRedo() && !hasCommandGroup() && myGNEApplicationWindowParent->isUndoRedoEnabled().empty();
    // cast button (see #6209)
    FXButton* button = dynamic_cast<FXButton*>(sender);
    // enable or disable depending of "enable" flag
    if (button) {
        // avoid unnnecesary enables/disables (due flickering)
        if (enable && !button->isEnabled()) {
            sender->handle(this, FXSEL(SEL_COMMAND, FXWindow::ID_ENABLE), nullptr);
            button->update();
        } else if (!enable && button->isEnabled()) {
            sender->handle(this, FXSEL(SEL_COMMAND, FXWindow::ID_DISABLE), nullptr);
            button->update();
        }
    } else {
        sender->handle(this, enable ? FXSEL(SEL_COMMAND, FXWindow::ID_ENABLE) : FXSEL(SEL_COMMAND, FXWindow::ID_DISABLE), nullptr);
    }
    // cast menu command
    FXMenuCommand* menuCommand = dynamic_cast<FXMenuCommand*>(sender);
    // only set caption on menu command item
    if (menuCommand) {
        // change caption of FXMenuCommand
        FXString caption = redoName();
        // set caption of FXmenuCommand edit/undo
        if (myGNEApplicationWindowParent->isUndoRedoEnabled().size() > 0) {
            caption = ("Cannot Redo in the middle of " + myGNEApplicationWindowParent->isUndoRedoEnabled()).c_str();
        } else if (hasCommandGroup()) {
            caption = ("Cannot Redo in the middle of " + myCommandGroups.top()->getDescription()).c_str();
        } else if (!canRedo()) {
            caption = "Redo";
        }
        menuCommand->handle(this, FXSEL(SEL_COMMAND, FXMenuCaption::ID_SETSTRINGVALUE), (void*)&caption);
        menuCommand->update();
    }
    return 1;
}


bool
GNEUndoList::hasCommandGroup() const {
    return myCommandGroups.size() != 0;
}