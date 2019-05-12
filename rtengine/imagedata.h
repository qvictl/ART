/* -*- C++ -*-
 *  
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
#ifndef __IMAGEDATA_H__
#define __IMAGEDATA_H__

#include <cstdio>
#include <memory>
#include "rawimage.h"
#include <string>
#include <glibmm.h>
#include <exiv2/exiv2.hpp>
#include "procparams.h"
#include "rtengine.h"

namespace rtengine
{

Exiv2::Image::AutoPtr open_exiv2(const Glib::ustring &fname);
Exiv2::XmpData read_exiv2_xmp(const Glib::ustring &fname);


class FramesData : public FramesMetaData {
private:
    bool ok_;
    Glib::ustring fname_;
    unsigned int dcrawFrameCount;
    struct tm time;
    time_t timeStamp;
    int iso_speed;
    double aperture;
    double focal_len, focal_len35mm;
    float focus_dist;  // dist: 0=unknown, 10000=infinity
    double shutter;
    double expcomp;
    std::string make, model, serial;
    std::string orientation;
    std::string lens;
    IIOSampleFormat sampleFormat;
    bool isPixelShift;
    bool isHDR;
    
public:
    FramesData (const Glib::ustring& fname);

    void setDCRawFrameCount(unsigned int frameCount);
    unsigned int getFrameCount() const override;
    bool getPixelShift() const override;
    bool getHDR() const override;
    std::string getImageType() const override;
    IIOSampleFormat getSampleFormat() const override;
    bool hasExif() const override;
    tm getDateTime() const override;
    time_t getDateTimeAsTS() const override;
    int getISOSpeed() const override;
    double getFNumber() const override;
    double getFocalLen() const override;
    double getFocalLen35mm() const override;
    float getFocusDist() const override;
    double getShutterSpeed() const override;
    double getExpComp() const override;
    std::string getMake() const override;
    std::string getModel() const override;
    std::string getLens() const override;
    std::string getSerialNumber() const;
    std::string getOrientation() const override;
    Glib::ustring getFileName() const override;
};


}
#endif
