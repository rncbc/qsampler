// qsamplerDeviceForm.ui.h
//
// ui.h extension file, included from the uic-generated form implementation.
/****************************************************************************
   Copyright (C) 2004-2005, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#include <qvalidator.h>
#include <qmessagebox.h>

#include "qsamplerMainForm.h"

#include "config.h"


// Kind of constructor.
void qsamplerDeviceForm::init (void)
{
    // Initialize locals.
    m_pMainForm   = NULL;
	m_iDirtySetup = 0;
    m_iDirtyCount = 0;

    // Try to restore normal window positioning.
    adjustSize();
}


// Kind of destructor.
void qsamplerDeviceForm::destroy (void)
{
}


// Device configuration dialog setup formal initializer.
void qsamplerDeviceForm::setup ( qsamplerMainForm *pMainForm )
{
    m_pMainForm   = pMainForm;
	m_iDirtySetup = 0;
    m_iDirtyCount = 0;

    if (m_pMainForm == NULL)
        return;
    if (m_pMainForm->client() == NULL)
        return;

    qsamplerOptions *pOptions = m_pMainForm->options();
    if (pOptions == NULL)
        return;

	// Set our main client reference.
    DeviceParameterTable->setClient(pMainForm->client());

    // Avoid nested changes.
    m_iDirtySetup++;

	//
    // TODO: Load initial device configuration data ...
    //

    // Done.
    m_iDirtySetup--;
    stabilizeForm();
}


// Dirty up settings.
void qsamplerDeviceForm::contentsChanged (void)
{
    if (m_iDirtySetup > 0)
        return;

    m_iDirtyCount++;
    stabilizeForm();
}


// Stabilize current form state.
void qsamplerDeviceForm::stabilizeForm (void)
{
	// TODO: Enable/disable available command buttons.
}


// end of qsamplerDeviceForm.ui.h

