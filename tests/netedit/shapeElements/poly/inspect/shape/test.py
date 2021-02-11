#!/usr/bin/env python
# Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
# Copyright (C) 2009-2021 German Aerospace Center (DLR) and others.
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# https://www.eclipse.org/legal/epl-2.0/
# This Source Code may also be made available under the following Secondary
# Licenses when the conditions for such availability set forth in the Eclipse
# Public License 2.0 are satisfied: GNU General Public License, version 2
# or later which is available at
# https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
# SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later

# @file    test.py
# @author  Pablo Alvarez Lopez
# @date    2016-11-25

# import common functions for netedit tests
import os
import sys

testRoot = os.path.join(os.environ.get('SUMO_HOME', '.'), 'tests')
neteditTestRoot = os.path.join(
    os.environ.get('TEXTTEST_HOME', testRoot), 'netedit')
sys.path.append(neteditTestRoot)
import neteditTestFunctions as netedit  # noqa

# Open netedit
neteditProcess, referencePosition = netedit.setupAndStart(neteditTestRoot, ['--gui-testing-debug-gl'])

# go to shape mode
netedit.shapeMode()

# go to poly mode and select poly
netedit.changeElement("poly")

# create first polygon
netedit.createSquaredTAZ(referencePosition, 182, 130, 200, True)
# go to inspect mode
netedit.inspectMode()

# inspect first polygon
netedit.leftClick(referencePosition, 182, 130)

# Change parameter 1 with a non valid value (dummy)
netedit.modifyAttribute(1, "dummyShape", False)

# Change parameter 1 with a non valid value (empty)
netedit.modifyAttribute(1, "", False)

# Change parameter 1 with a valid value (single point)
netedit.modifyAttribute(1, "12.00,8.00", False)

# Change parameter 1 with a valid value
netedit.modifyAttribute(1, "12.00,8.00 9.00,13.00 12.00,18.00 7.00,15.00 2.00,18.00 " +
                           "5.00,13.00 2.00,8.00 7.00,11.00 12.00,8.00", False)

# Check undos and redos
netedit.undo(referencePosition, 3)
netedit.redo(referencePosition, 3)

# save shapes
netedit.saveAdditionals(referencePosition)

# save network
netedit.saveNetwork(referencePosition)

# quit netedit
netedit.quit(neteditProcess)
