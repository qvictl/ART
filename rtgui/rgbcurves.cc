/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "rgbcurves.h"

using namespace rtengine;
using namespace rtengine::procparams;

RGBCurves::RGBCurves () : FoldableToolPanel(this, "rgbcurves", M("TP_RGBCURVES_LABEL"), false, true)
{
    std::vector<GradientMilestone> milestones;

    curveEditorG = new CurveEditorGroup(options.lastRgbCurvesDir, "");//M("TP_RGBCURVES_CHANNEL"));
    curveEditorG->setCurveListener (this);

    Rshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, M("TP_RGBCURVES_RED")));
    Rshape->setEditID(EUID_RGB_R, BT_SINGLEPLANE_FLOAT);
    milestones.push_back( GradientMilestone(0.0, 0.0, 0.0, 0.0) );
    milestones.push_back( GradientMilestone(1.0, 1.0, 0.0, 0.0) );
    Rshape->setBottomBarBgGradient(milestones);
    Rshape->setLeftBarBgGradient(milestones);

    milestones[1].r = 0.0;
    milestones[1].g = 1.0;
    Gshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, M("TP_RGBCURVES_GREEN")));
    Gshape->setEditID(EUID_RGB_G, BT_SINGLEPLANE_FLOAT);
    Gshape->setBottomBarBgGradient(milestones);
    Gshape->setLeftBarBgGradient(milestones);

    milestones[1].g = 0.0;
    milestones[1].b = 1.0;
    Bshape = static_cast<DiagonalCurveEditor*>(curveEditorG->addCurve(CT_Diagonal, M("TP_RGBCURVES_BLUE")));
    Bshape->setEditID(EUID_RGB_B, BT_SINGLEPLANE_FLOAT);
    Bshape->setBottomBarBgGradient(milestones);
    Bshape->setLeftBarBgGradient(milestones);

    // This will add the reset button at the end of the curveType buttons
    curveEditorG->curveListComplete();

    pack_start (*curveEditorG, Gtk::PACK_SHRINK, 4);

}

RGBCurves::~RGBCurves ()
{
    delete curveEditorG;
}

void RGBCurves::read(const ProcParams* pp)
{
    disableListener ();

    Rshape->setCurve         (pp->rgbCurves.rcurve);
    Gshape->setCurve         (pp->rgbCurves.gcurve);
    Bshape->setCurve         (pp->rgbCurves.bcurve);

    setEnabled(pp->rgbCurves.enabled);

    enableListener ();
}

void RGBCurves::setEditProvider (EditDataProvider *provider)
{
    Rshape->setEditProvider(provider);
    Gshape->setEditProvider(provider);
    Bshape->setEditProvider(provider);
}

void RGBCurves::autoOpenCurve  ()
{
    // Open up the first curve if selected
    bool active = Rshape->openIfNonlinear();

    if (!active) {
        Gshape->openIfNonlinear();
    }

    if (!active) {
        Bshape->openIfNonlinear();
    }
}

void RGBCurves::write(ProcParams* pp)
{
    pp->rgbCurves.enabled = getEnabled();
    pp->rgbCurves.rcurve         = Rshape->getCurve ();
    pp->rgbCurves.gcurve         = Gshape->getCurve ();
    pp->rgbCurves.bcurve         = Bshape->getCurve ();
}


/*
 * Curve listener
 *
 * If more than one curve has been added, the curve listener is automatically
 * set to 'multi=true', and send a pointer of the modified curve in a parameter
 */
void RGBCurves::curveChanged (CurveEditor* ce)
{

    if (listener && getEnabled()) {
        if (ce == Rshape) {
            listener->panelChanged (EvRGBrCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == Gshape) {
            listener->panelChanged (EvRGBgCurve, M("HISTORY_CUSTOMCURVE"));
        }

        if (ce == Bshape) {
            listener->panelChanged (EvRGBbCurve, M("HISTORY_CUSTOMCURVE"));
        }
    }
}


void RGBCurves::updateCurveBackgroundHistogram(
    const LUTu& histToneCurve,
    const LUTu& histLCurve,
    const LUTu& histCCurve,
    const LUTu& histLCAM,
    const LUTu& histCCAM,
    const LUTu& histRed,
    const LUTu& histGreen,
    const LUTu& histBlue,
    const LUTu& histLuma,
    const LUTu& histLRETI
)
{
}


void RGBCurves::enabledChanged()
{
    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged(EvRGBEnabled, M("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged(EvRGBEnabled, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged(EvRGBEnabled, M("GENERAL_DISABLED"));
        }
    }
}
